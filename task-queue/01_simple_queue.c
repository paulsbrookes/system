/*
 * 01_simple_queue.c - Single-threaded FIFO Task Queue
 *
 * Demonstrates a basic task queue using a linked list.
 * Tasks are file processing operations executed sequentially.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_PATH 256
#define MAX_PATTERN 64

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
    char pattern[MAX_PATTERN];  // Used for FIND_PATTERN tasks
} Task;

/* Queue node - linked list element */
typedef struct QueueNode {
    Task task;
    struct QueueNode *next;
} QueueNode;

/* Queue structure - manages the linked list */
typedef struct {
    QueueNode *head;  // Front of queue (dequeue from here)
    QueueNode *tail;  // Back of queue (enqueue here)
    int count;        // Number of tasks in queue
} TaskQueue;

/* ===== Queue Operations ===== */

/* Create and initialize an empty queue */
TaskQueue* queue_create(void) {
    TaskQueue *queue = malloc(sizeof(TaskQueue));
    if (!queue) {
        perror("Failed to allocate queue");
        return NULL;
    }
    queue->head = NULL;
    queue->tail = NULL;
    queue->count = 0;
    return queue;
}

/* Add a task to the back of the queue */
int queue_enqueue(TaskQueue *queue, Task task) {
    if (!queue) return -1;

    QueueNode *node = malloc(sizeof(QueueNode));
    if (!node) {
        perror("Failed to allocate queue node");
        return -1;
    }

    node->task = task;
    node->next = NULL;

    if (queue->tail) {
        queue->tail->next = node;
    } else {
        queue->head = node;  // First element
    }
    queue->tail = node;
    queue->count++;

    return 0;
}

/* Remove and return a task from the front of the queue */
int queue_dequeue(TaskQueue *queue, Task *task) {
    if (!queue || !task) return -1;
    if (!queue->head) return -1;  // Queue is empty

    QueueNode *node = queue->head;
    *task = node->task;

    queue->head = node->next;
    if (!queue->head) {
        queue->tail = NULL;  // Queue is now empty
    }
    queue->count--;

    free(node);
    return 0;
}

/* Check if queue is empty */
int queue_is_empty(TaskQueue *queue) {
    return queue == NULL || queue->head == NULL;
}

/* Get the number of tasks in the queue */
int queue_size(TaskQueue *queue) {
    return queue ? queue->count : 0;
}

/* Destroy the queue and free all memory */
void queue_destroy(TaskQueue *queue) {
    if (!queue) return;

    Task task;
    while (queue_dequeue(queue, &task) == 0) {
        // Just dequeue and discard all remaining tasks
    }

    free(queue);
}

/* ===== Task Execution Functions ===== */

/* Count lines in a file */
void execute_count_lines(const char *filepath) {
    FILE *file = fopen(filepath, "r");
    if (!file) {
        perror("Failed to open file");
        printf("  [ERROR] Could not open: %s\n", filepath);
        return;
    }

    int lines = 0;
    int ch;
    while ((ch = fgetc(file)) != EOF) {
        if (ch == '\n') {
            lines++;
        }
    }

    fclose(file);
    printf("  [RESULT] %s: %d lines\n", filepath, lines);
}

/* Count words in a file (simple whitespace-based) */
void execute_count_words(const char *filepath) {
    FILE *file = fopen(filepath, "r");
    if (!file) {
        perror("Failed to open file");
        printf("  [ERROR] Could not open: %s\n", filepath);
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
    printf("  [RESULT] %s: %d words\n", filepath, words);
}

/* Find pattern in a file (simple substring search) */
void execute_find_pattern(const char *filepath, const char *pattern) {
    FILE *file = fopen(filepath, "r");
    if (!file) {
        perror("Failed to open file");
        printf("  [ERROR] Could not open: %s\n", filepath);
        return;
    }

    char line[1024];
    int line_num = 0;
    int matches = 0;

    while (fgets(line, sizeof(line), file)) {
        line_num++;
        if (strstr(line, pattern)) {
            matches++;
        }
    }

    fclose(file);
    printf("  [RESULT] %s: Found '%s' on %d line(s)\n", filepath, pattern, matches);
}

/* Execute a single task based on its type */
void execute_task(Task *task) {
    printf("Executing task: ");

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

/* Create a task for counting lines */
Task create_count_lines_task(const char *filepath) {
    Task task;
    task.type = TASK_COUNT_LINES;
    strncpy(task.filepath, filepath, MAX_PATH - 1);
    task.filepath[MAX_PATH - 1] = '\0';
    task.pattern[0] = '\0';
    return task;
}

/* Create a task for counting words */
Task create_count_words_task(const char *filepath) {
    Task task;
    task.type = TASK_COUNT_WORDS;
    strncpy(task.filepath, filepath, MAX_PATH - 1);
    task.filepath[MAX_PATH - 1] = '\0';
    task.pattern[0] = '\0';
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
    return task;
}

/* ===== Main Program ===== */

int main(void) {
    printf("=== Simple Task Queue Demo ===\n\n");

    // Create the queue
    TaskQueue *queue = queue_create();
    if (!queue) {
        fprintf(stderr, "Failed to create queue\n");
        return 1;
    }

    // Add various tasks to the queue
    printf("Enqueueing tasks...\n");

    queue_enqueue(queue, create_count_lines_task("test_file.txt"));
    queue_enqueue(queue, create_count_words_task("test_file.txt"));
    queue_enqueue(queue, create_find_pattern_task("test_file.txt", "task"));
    queue_enqueue(queue, create_count_lines_task("01_simple_queue.c"));
    queue_enqueue(queue, create_find_pattern_task("01_simple_queue.c", "queue"));

    printf("Queue size: %d tasks\n\n", queue_size(queue));

    // Process all tasks in order (FIFO)
    printf("Processing tasks...\n");
    Task task;
    while (!queue_is_empty(queue)) {
        if (queue_dequeue(queue, &task) == 0) {
            execute_task(&task);
            printf("  Queue size: %d remaining\n\n", queue_size(queue));
        }
    }

    printf("All tasks completed!\n");

    // Clean up
    queue_destroy(queue);

    return 0;
}
