/*
 * 03_threadpool.c - Thread Pool with Work Queue
 *
 * Demonstrates a complete thread pool implementation with a fixed number
 * of worker threads processing tasks from a shared queue.
 * This is a realistic, production-style concurrent programming pattern.
 */

#define _POSIX_C_SOURCE 200112L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#define MAX_PATH 256
#define MAX_PATTERN 64
#define DEFAULT_POOL_SIZE 4

/* Task operation types */
typedef enum {
    TASK_COUNT_LINES,
    TASK_COUNT_WORDS,
    TASK_FIND_PATTERN
} TaskType;

/* Task structure - represents work to be done */
typedef struct {
    TaskType type;
    char filepath[MAX_PATH];
    char pattern[MAX_PATTERN];
    int task_id;
} Task;

/* Queue node - linked list element */
typedef struct QueueNode {
    Task task;
    struct QueueNode *next;
} QueueNode;

/* Thread pool structure */
typedef struct {
    pthread_t *threads;           // Array of worker threads
    int num_threads;              // Number of worker threads

    QueueNode *queue_head;        // Front of task queue
    QueueNode *queue_tail;        // Back of task queue
    int queue_size;               // Number of tasks in queue

    pthread_mutex_t mutex;        // Protects queue and shutdown flag
    pthread_cond_t task_available; // Signals when tasks are available
    pthread_cond_t queue_empty;   // Signals when queue becomes empty

    int shutdown;                 // Flag to signal shutdown
    int active_tasks;             // Number of tasks being processed
} ThreadPool;

/* Global task counter */
int g_task_counter = 0;
pthread_mutex_t g_counter_mutex = PTHREAD_MUTEX_INITIALIZER;

/* ===== Task Queue Operations (Internal) ===== */

/* Add task to queue (assumes mutex is held) */
static void queue_push(ThreadPool *pool, Task task) {
    QueueNode *node = malloc(sizeof(QueueNode));
    if (!node) {
        perror("Failed to allocate queue node");
        return;
    }

    node->task = task;
    node->next = NULL;

    if (pool->queue_tail) {
        pool->queue_tail->next = node;
    } else {
        pool->queue_head = node;
    }
    pool->queue_tail = node;
    pool->queue_size++;
}

/* Remove task from queue (assumes mutex is held) */
static int queue_pop(ThreadPool *pool, Task *task) {
    if (!pool->queue_head) {
        return -1;  // Queue is empty
    }

    QueueNode *node = pool->queue_head;
    *task = node->task;

    pool->queue_head = node->next;
    if (!pool->queue_head) {
        pool->queue_tail = NULL;
    }
    pool->queue_size--;

    free(node);
    return 0;
}

/* ===== Task Execution Functions ===== */

/* Count lines in a file */
static void execute_count_lines(const char *filepath) {
    FILE *file = fopen(filepath, "r");
    if (!file) {
        printf("      [ERROR] Could not open: %s\n", filepath);
        return;
    }

    int lines = 0;
    int ch;
    while ((ch = fgetc(file)) != EOF) {
        if (ch == '\n') lines++;
    }

    fclose(file);
    printf("      [RESULT] %s: %d lines\n", filepath, lines);
}

/* Count words in a file */
static void execute_count_words(const char *filepath) {
    FILE *file = fopen(filepath, "r");
    if (!file) {
        printf("      [ERROR] Could not open: %s\n", filepath);
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
    printf("      [RESULT] %s: %d words\n", filepath, words);
}

/* Find pattern in a file */
static void execute_find_pattern(const char *filepath, const char *pattern) {
    FILE *file = fopen(filepath, "r");
    if (!file) {
        printf("      [ERROR] Could not open: %s\n", filepath);
        return;
    }

    char line[1024];
    int matches = 0;

    while (fgets(line, sizeof(line), file)) {
        if (strstr(line, pattern)) matches++;
    }

    fclose(file);
    printf("      [RESULT] %s: Found '%s' on %d line(s)\n", filepath, pattern, matches);
}

/* Execute a task */
static void execute_task(Task *task, int worker_id) {
    printf("    [Worker %d] Executing task #%d: ", worker_id, task->task_id);

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

/* ===== Worker Thread ===== */

/* Worker thread function - continuously processes tasks */
static void* worker_thread(void *arg) {
    ThreadPool *pool = (ThreadPool*)arg;
    int worker_id = (int)pthread_self() % 1000;  // Simple ID for display

    printf("  [Worker %d] Started\n", worker_id);

    while (1) {
        pthread_mutex_lock(&pool->mutex);

        // Wait for a task or shutdown signal
        while (pool->queue_size == 0 && !pool->shutdown) {
            pthread_cond_wait(&pool->task_available, &pool->mutex);
        }

        // Check for shutdown
        if (pool->shutdown && pool->queue_size == 0) {
            pthread_mutex_unlock(&pool->mutex);
            break;
        }

        // Get a task from the queue
        Task task;
        if (queue_pop(pool, &task) == 0) {
            pool->active_tasks++;
            pthread_mutex_unlock(&pool->mutex);

            // Execute the task (without holding the mutex)
            execute_task(&task, worker_id);
            usleep(100000);  // Simulate some processing time

            // Mark task as complete
            pthread_mutex_lock(&pool->mutex);
            pool->active_tasks--;

            // Signal if queue is now empty and no tasks are active
            if (pool->queue_size == 0 && pool->active_tasks == 0) {
                pthread_cond_signal(&pool->queue_empty);
            }
            pthread_mutex_unlock(&pool->mutex);
        } else {
            pthread_mutex_unlock(&pool->mutex);
        }
    }

    printf("  [Worker %d] Shutting down\n", worker_id);
    return NULL;
}

/* ===== Thread Pool API ===== */

/* Create and initialize a thread pool */
ThreadPool* threadpool_create(int num_threads) {
    if (num_threads <= 0) {
        num_threads = DEFAULT_POOL_SIZE;
    }

    ThreadPool *pool = malloc(sizeof(ThreadPool));
    if (!pool) {
        perror("Failed to allocate thread pool");
        return NULL;
    }

    // Initialize pool fields
    pool->threads = malloc(sizeof(pthread_t) * num_threads);
    if (!pool->threads) {
        perror("Failed to allocate thread array");
        free(pool);
        return NULL;
    }

    pool->num_threads = num_threads;
    pool->queue_head = NULL;
    pool->queue_tail = NULL;
    pool->queue_size = 0;
    pool->shutdown = 0;
    pool->active_tasks = 0;

    // Initialize synchronization primitives
    if (pthread_mutex_init(&pool->mutex, NULL) != 0) {
        perror("Failed to initialize mutex");
        free(pool->threads);
        free(pool);
        return NULL;
    }

    if (pthread_cond_init(&pool->task_available, NULL) != 0) {
        perror("Failed to initialize condition variable");
        pthread_mutex_destroy(&pool->mutex);
        free(pool->threads);
        free(pool);
        return NULL;
    }

    if (pthread_cond_init(&pool->queue_empty, NULL) != 0) {
        perror("Failed to initialize condition variable");
        pthread_cond_destroy(&pool->task_available);
        pthread_mutex_destroy(&pool->mutex);
        free(pool->threads);
        free(pool);
        return NULL;
    }

    // Create worker threads
    for (int i = 0; i < num_threads; i++) {
        if (pthread_create(&pool->threads[i], NULL, worker_thread, pool) != 0) {
            perror("Failed to create worker thread");
            // Clean up already created threads
            pool->shutdown = 1;
            pthread_cond_broadcast(&pool->task_available);
            for (int j = 0; j < i; j++) {
                pthread_join(pool->threads[j], NULL);
            }
            pthread_cond_destroy(&pool->queue_empty);
            pthread_cond_destroy(&pool->task_available);
            pthread_mutex_destroy(&pool->mutex);
            free(pool->threads);
            free(pool);
            return NULL;
        }
    }

    return pool;
}

/* Submit a task to the thread pool */
int threadpool_submit(ThreadPool *pool, Task task) {
    if (!pool || pool->shutdown) {
        return -1;
    }

    pthread_mutex_lock(&pool->mutex);

    queue_push(pool, task);
    pthread_cond_signal(&pool->task_available);

    pthread_mutex_unlock(&pool->mutex);

    return 0;
}

/* Wait for all tasks to complete */
void threadpool_wait(ThreadPool *pool) {
    if (!pool) return;

    pthread_mutex_lock(&pool->mutex);

    while (pool->queue_size > 0 || pool->active_tasks > 0) {
        pthread_cond_wait(&pool->queue_empty, &pool->mutex);
    }

    pthread_mutex_unlock(&pool->mutex);
}

/* Shutdown the thread pool and free resources */
void threadpool_destroy(ThreadPool *pool) {
    if (!pool) return;

    pthread_mutex_lock(&pool->mutex);
    pool->shutdown = 1;
    pthread_cond_broadcast(&pool->task_available);
    pthread_mutex_unlock(&pool->mutex);

    // Wait for all worker threads to finish
    for (int i = 0; i < pool->num_threads; i++) {
        pthread_join(pool->threads[i], NULL);
    }

    // Clean up remaining tasks in queue
    pthread_mutex_lock(&pool->mutex);
    QueueNode *node = pool->queue_head;
    while (node) {
        QueueNode *next = node->next;
        free(node);
        node = next;
    }
    pthread_mutex_unlock(&pool->mutex);

    // Destroy synchronization primitives
    pthread_cond_destroy(&pool->queue_empty);
    pthread_cond_destroy(&pool->task_available);
    pthread_mutex_destroy(&pool->mutex);

    // Free memory
    free(pool->threads);
    free(pool);
}

/* Get the current queue size */
int threadpool_queue_size(ThreadPool *pool) {
    if (!pool) return 0;

    pthread_mutex_lock(&pool->mutex);
    int size = pool->queue_size;
    pthread_mutex_unlock(&pool->mutex);

    return size;
}

/* ===== Helper Functions ===== */

/* Get next task ID (thread-safe) */
static int get_next_task_id(void) {
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

/* ===== Main Program ===== */

int main(void) {
    printf("=== Thread Pool Demo ===\n\n");

    // Create a thread pool with 4 workers
    int num_workers = 4;
    printf("Creating thread pool with %d workers...\n", num_workers);
    ThreadPool *pool = threadpool_create(num_workers);
    if (!pool) {
        fprintf(stderr, "Failed to create thread pool\n");
        return 1;
    }

    sleep(1);  // Give workers time to start
    printf("\n");

    // Submit various tasks to the pool
    printf("Submitting tasks to the pool...\n");

    threadpool_submit(pool, create_count_lines_task("test_file.txt"));
    threadpool_submit(pool, create_count_words_task("test_file.txt"));
    threadpool_submit(pool, create_find_pattern_task("test_file.txt", "task"));
    threadpool_submit(pool, create_count_lines_task("03_threadpool.c"));
    threadpool_submit(pool, create_count_words_task("03_threadpool.c"));
    threadpool_submit(pool, create_find_pattern_task("03_threadpool.c", "pool"));
    threadpool_submit(pool, create_find_pattern_task("03_threadpool.c", "thread"));
    threadpool_submit(pool, create_count_lines_task("01_simple_queue.c"));
    threadpool_submit(pool, create_count_words_task("02_threadsafe_queue.c"));
    threadpool_submit(pool, create_find_pattern_task("01_simple_queue.c", "queue"));

    printf("Submitted 10 tasks (queue size: %d)\n\n", threadpool_queue_size(pool));

    // Wait for all tasks to complete
    printf("Waiting for all tasks to complete...\n\n");
    threadpool_wait(pool);

    printf("\nAll tasks completed!\n");
    printf("Queue size: %d\n", threadpool_queue_size(pool));

    // Shutdown the thread pool
    printf("\nShutting down thread pool...\n");
    threadpool_destroy(pool);

    printf("Thread pool destroyed. Program exiting.\n");

    return 0;
}
