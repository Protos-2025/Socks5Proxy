#include "queue.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

struct Node {
	struct Node *next;
	uint8_t *data;
};

typedef struct QueueCDT {
    struct Node *first;
    struct Node *last;
    size_t elemSize;
    /** Zero if unlimited */
    size_t max_capacity;
    size_t size;
    QueueElemFreeFn freeFn;
    struct Node * iterNode;
} QueueCDT;

/**
 * @brief Create a Queue object
 * 
 * @param cmp 
 * @param elemSize 
 * @param max_capacity Zero if unlimited. Drops if full.
 * @return Queue 
 */
Queue create_queue(QueueElemFreeFn free_fn, size_t elem_size, size_t max_capacity) {
    Queue queue = (Queue)malloc(sizeof(QueueCDT));
    if (!queue) {
        return NULL;
    }
    queue->first = NULL;
    queue->last = NULL;
    queue->iterNode = NULL;
    queue->elemSize = elem_size;
    queue->size = 0;
    queue->freeFn = free_fn;
    queue->max_capacity = max_capacity;
    return queue;
}

Queue enqueue(Queue queue, void * data) {
    if (!queue || !data) {
        return NULL;
    }
    struct Node *newNode = (struct Node *)malloc(sizeof(struct Node));
    if (!newNode) {
        return NULL;
    }
    newNode->data = (uint8_t *)malloc(queue->elemSize);
    memcpy(newNode->data, data, queue->elemSize);
    newNode->next = NULL;

    if (queue->max_capacity != 0 && queue->size >= queue->max_capacity) {
        if (queue->freeFn) {
            queue->freeFn(queue->first->data);
        }
        dequeue(queue, NULL);
    }

    if (queue->last) {
        queue->last->next = newNode;
    } else {
        queue->first = newNode;
    }
    queue->last = newNode;
    queue->size++;
    return queue;
}

void * dequeue(Queue queue, void * buffer) {
    if (!queue || queue->size == 0) {
        return NULL;
    }
    struct Node *temp = queue->first;
    if (buffer != NULL)
        memcpy(buffer, temp->data, queue->elemSize);
    queue->first = queue->first->next;
    if (queue->first == NULL) {
        queue->last = NULL;
    }
    free(temp->data);
    free(temp);
    queue->size--;
    return buffer;
}

void * queue_peek(Queue queue, void * buffer) {
    if (!queue || queue->size == 0) {
        return NULL;
    }
    memcpy(buffer, queue->first->data, queue->elemSize);
    return buffer;
}

size_t queue_size(Queue queue) {
    if (!queue) return 0;
    return queue->size;
}

void free_queue(Queue queue) {
    if (!queue) return;
    struct Node *current = queue->first;
    while (current) {
        struct Node *temp = current;
        current = current->next;
        free(temp->data);
        free(temp);
    }
    free(queue);
}

void queue_begin_iter(Queue queue) {
    if (!queue) return;
    queue->iterNode = queue->first;
}

int queue_has_next_iter(Queue queue) {
    if (!queue) return 0;
    return queue->iterNode != NULL;
}

void * queue_iter_next(Queue queue, void * buffer) {
    if (!queue_has_next_iter(queue)) {
        return NULL;
    }
    memcpy(buffer, queue->iterNode->data, queue->elemSize);
    queue->iterNode = queue->iterNode->next;
    return buffer;
}
