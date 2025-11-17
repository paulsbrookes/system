/*
 * 02_threadsafe_queue.c - Thread-safe FIFO Task Queue
 *
 * Demonstrates a thread-safe task queue using mutexes and condition variables.
 * Multiple producer and consumer threads can safely access the queue concurrently.
 * Implements the classic producer-consumer pattern.
 */

#define _POSIX_C_SOURCE 200112L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#define MAX_PATH 256
#define MAX_PATTERN 64
#define NUM_PRODUCERS 2
#define NUM_CONSUMERS 3

/* Task operation types */
typedef enum {
    TASK_COUNT_LINES,
    TASK_COUNT_WORDS,
    TASK_FIND_PATTERN,
    TASK_SHUTDOWN  // Special task to signal shutdown
} TaskType;

/* Task structure - represents work to be done */
typedef struct {
    TaskType type;
    char filepath[MAX_PATH];
    char pattern[MAX_PATTERN];
    int task_id;  // For tracking which task this is
} Task;

/* Queue node - linked list element */
typedef struct QueueNode {
    Task task;
    struct QueueNode *next;
} QueueNode;

/* Thread-safe queue structure */
typedef struct {
    QueueNode *head;           // Front of queue (dequeue from here)
    QueueNode *tail;           // Back of queue (enqueue here)
    int count;                 // Number of tasks in queue
    pthread_mutex_t mutex;     // Protects queue data
    pthread_cond_t not_empty;  // Signals when queue has items
} TaskQueue;

/* Global queue shared by all threads */
TaskQueue *g_queue = NULL;

/* Global task counter for tracking */
int g_task_counter = 0;
pthread_mutex_t g_counter_mutex = PTHREAD_MUTEX_INITIALIZER;

/* ===== Queue Operations (Thread-Safe) ===== */

/* Create and initialize an empty thread-safe queue */
TaskQueue* queue_create(void) {
    TaskQueue *queue = malloc(sizeof(TaskQueue));
    if (!queue) {
        perror("Failed to allocate queue");
        return NULL;
    }

    queue->head = NULL;
    queue->tail = NULL;
    queue->count = 0;

    // Initialize synchronization primitives
    if (pthread_mutex_init(&queue->mutex, NULL) != 0) {
        perror("Failed to initialize mutex");
        free(queue);
        return NULL;
    }

    if (pthread_cond_init(&queue->not_empty, NULL) != 0) {
        perror("Failed to initialize condition variable");
        pthread_mutex_destroy(&queue->mutex);
        free(queue);
        return NULL;
    }

    return queue;
}

/* Add a task to the back of the queue (thread-safe) */
int queue_enqueue(TaskQueue *queue, Task task) {
    if (!queue) return -1;

    QueueNode *node = malloc(sizeof(QueueNode));
    if (!node) {
        perror("Failed to allocate queue node");
        return -1;
    }

    node->task = task;
    node->next = NULL;

    // Lock the queue for exclusive access
    pthread_mutex_lock(&queue->mutex);

    if (queue->tail) {
        queue->tail->next = node;
    } else {
        queue->head = node;  // First element
    }
    queue->tail = node;
    queue->count++;

    // Signal waiting consumers that queue is not empty
    pthread_cond_signal(&queue->not_empty);

    pthread_mutex_unlock(&queue->mutex);

    return 0;
}

/* Remove and return a task from the front of the queue (thread-safe, blocking) */
int queue_dequeue(TaskQueue *queue, Task *task) {
    if (!queue || !task) return -1;

    pthread_mutex_lock(&queue->mutex);

    // Wait until queue is not empty
    while (queue->head == NULL) {
        pthread_cond_wait(&queue->not_empty, &queue->mutex);
    }

    // Dequeue the task
    QueueNode *node = queue->head;
    *task = node->task;

    queue->head = node->next;
    if (!queue->head) {
        queue->tail = NULL;  // Queue is now empty
    }
    queue->count--;

    pthread_mutex_unlock(&queue->mutex);

    free(node);
    return 0;
}

/* Get the number of tasks in the queue (thread-safe) */
int queue_size(TaskQueue *queue) {
    if (!queue) return 0;

    pthread_mutex_lock(&queue->mutex);
    int size = queue->count;
    pthread_mutex_unlock(&queue->mutex);

    return size;
}

/* Destroy the queue and free all memory */
void queue_destroy(TaskQueue *queue) {
    if (!queue) return;

    pthread_mutex_lock(&queue->mutex);

    QueueNode *node = queue->head;
    while (node) {
        QueueNode *next = node->next;
        free(node);
        node = next;
    }

    pthread_mutex_unlock(&queue->mutex);

    pthread_mutex_destroy(&queue->mutex);
    pthread_cond_destroy(&queue->not_empty);
    free(queue);
}

/* ===== Task Execution Functions ===== */

/* Count lines in a file */
void execute_count_lines(const char *filepath) {
    FILE *file = fopen(filepath, "r");
    if (!file) {
        printf("    [ERROR] Could not open: %s\n", filepath);
        return;
    }

    int lines = 0;
    int ch;
    while ((ch = fgetc(file)) != EOF) {
        if (ch == '\n') lines++;
    }

    fclose(file);
    printf("    [RESULT] %s: %d lines\n", filepath, lines);
}

/* Count words in a file */
void execute_count_words(const char *filepath) {
    FILE *file = fopen(filepath, "r");
    if (!file) {
        printf("    [ERROR] Could not open: %s\n", filepath);
        return;
    }

    int words = 0;
    int in_word = 0;
    int ch;

    while ((ch = fgetc(file)) != EOF) {
        if (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r') {
            in_word = 0;
        } else if (!in_word) {
            in_word = 1;
            words++;
        }
    }

    fclose(file);
    printf("    [RESULT] %s: %d words\n", filepath, words);
}

/* Find pattern in a file */
void execute_find_pattern(const char *filepath, const char *pattern) {
    FILE *file = fopen(filepath, "r");
    if (!file) {
        printf("    [ERROR] Could not open: %s\n", filepath);
        return;
    }

    char line[1024];
    int matches = 0;

    while (fgets(line, sizeof(line), file)) {
        if (strstr(line, pattern)) matches++;
    }

    fclose(file);
    printf("    [RESULT] %s: Found '%s' on %d line(s)\n", filepath, pattern, matches);
}

/* Execute a single task based on its type */
void execute_task(Task *task, int thread_id) {
    if (task->type == TASK_SHUTDOWN) {
        return;  // Don't execute shutdown tasks
    }

    printf("  [Thread %d] Task #%d: ", thread_id, task->task_id);

    switch (task->type) {
        case TASK_COUNT_LINES:
            printf("COUNT_LINES on %s\n", task->filepath);
            execute_count_lines(task->filepath);
            break;

        case TASK_COUNT_WORDS:
            printf("COUNT_WORDS on %s\n", task->filepath);
            execute_count_words(task->filepath);
            break;

        case TASK_FIND_PATTERN:
            printf("FIND_PATTERN '%s' in %s\n", task->pattern, task->filepath);
            execute_find_pattern(task->filepath, task->pattern);
            break;

        default:
            printf("Unknown task type\n");
    }
}

/* ===== Helper Functions ===== */

/* Get next task ID (thread-safe) */
int get_next_task_id(void) {
    pthread_mutex_lock(&g_counter_mutex);
    int id = ++g_task_counter;
    pthread_mutex_unlock(&g_counter_mutex);
    return id;
}

/* Create a task for counting lines */
Task create_count_lines_task(const char *filepath) {
    Task task;
    task.type = TASK_COUNT_LINES;
    strncpy(task.filepath, filepath, MAX_PATH - 1);
    task.filepath[MAX_PATH - 1] = '\0';
    task.pattern[0] = '\0';
    task.task_id = get_next_task_id();
    return task;
}

/* Create a task for counting words */
Task create_count_words_task(const char *filepath) {
    Task task;
    task.type = TASK_COUNT_WORDS;
    strncpy(task.filepath, filepath, MAX_PATH - 1);
    task.filepath[MAX_PATH - 1] = '\0';
    task.pattern[0] = '\0';
    task.task_id = get_next_task_id();
    return task;
}

/* Create a task for finding a pattern */
Task create_find_pattern_task(const char *filepath, const char *pattern) {
    Task task;
    task.type = TASK_FIND_PATTERN;
    strncpy(task.filepath, filepath, MAX_PATH - 1);
    task.filepath[MAX_PATH - 1] = '\0';
    strncpy(task.pattern, pattern, MAX_PATTERN - 1);
    task.pattern[MAX_PATTERN - 1] = '\0';
    task.task_id = get_next_task_id();
    return task;
}

/* Create a shutdown task */
Task create_shutdown_task(void) {
    Task task;
    task.type = TASK_SHUTDOWN;
    task.filepath[0] = '\0';
    task.pattern[0] = '\0';
    task.task_id = -1;
    return task;
}

/* ===== Thread Functions ===== */

/* Producer thread - adds tasks to the queue */
void* producer_thread(void *arg) {
    int thread_id = *(int*)arg;
    free(arg);

    printf("[Producer %d] Started\n", thread_id);

    // Each producer creates a few tasks
    for (int i = 0; i < 3; i++) {
        sleep(1);  // Simulate some work before creating task

        Task task;
        switch (i % 3) {
            case 0:
                task = create_count_lines_task("test_file.txt");
                break;
            case 1:
                task = create_count_words_task("02_threadsafe_queue.c");
                break;
            case 2:
                task = create_find_pattern_task("test_file.txt", "queue");
                break;
        }

        queue_enqueue(g_queue, task);
        printf("[Producer %d] Enqueued task #%d (queue size: %d)\n",
               thread_id, task.task_id, queue_size(g_queue));
    }

    printf("[Producer %d] Finished\n", thread_id);
    return NULL;
}

/* Consumer thread - processes tasks from the queue */
void* consumer_thread(void *arg) {
    int thread_id = *(int*)arg;
    free(arg);

    printf("[Consumer %d] Started\n", thread_id);

    while (1) {
        Task task;
        queue_dequeue(g_queue, &task);

        if (task.type == TASK_SHUTDOWN) {
            printf("[Consumer %d] Received shutdown signal\n", thread_id);
            break;
        }

        execute_task(&task, thread_id);
        usleep(100000);  // Small delay to see interleaving
    }

    printf("[Consumer %d] Finished\n", thread_id);
    return NULL;
}

/* ===== Main Program ===== */

int main(void) {
    printf("=== Thread-Safe Task Queue Demo ===\n");
    printf("Producers: %d, Consumers: %d\n\n", NUM_PRODUCERS, NUM_CONSUMERS);

    // Create the shared queue
    g_queue = queue_create();
    if (!g_queue) {
        fprintf(stderr, "Failed to create queue\n");
        return 1;
    }

    // Create producer threads
    pthread_t producers[NUM_PRODUCERS];
    for (int i = 0; i < NUM_PRODUCERS; i++) {
        int *id = malloc(sizeof(int));
        *id = i + 1;
        pthread_create(&producers[i], NULL, producer_thread, id);
    }

    // Create consumer threads
    pthread_t consumers[NUM_CONSUMERS];
    for (int i = 0; i < NUM_CONSUMERS; i++) {
        int *id = malloc(sizeof(int));
        *id = i + 1;
        pthread_create(&consumers[i], NULL, consumer_thread, id);
    }

    // Wait for all producers to finish
    for (int i = 0; i < NUM_PRODUCERS; i++) {
        pthread_join(producers[i], NULL);
    }

    printf("\nAll producers finished. Sending shutdown signals...\n\n");

    // Send shutdown signal to each consumer
    for (int i = 0; i < NUM_CONSUMERS; i++) {
        queue_enqueue(g_queue, create_shutdown_task());
    }

    // Wait for all consumers to finish
    for (int i = 0; i < NUM_CONSUMERS; i++) {
        pthread_join(consumers[i], NULL);
    }

    printf("\nAll tasks completed!\n");

    // Clean up
    queue_destroy(g_queue);

    return 0;
}
