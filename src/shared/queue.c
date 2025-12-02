#include "queue.h"

#include <stdlib.h>
#include <string.h>

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
    QueueElemCmpFn cmpFn;
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
Queue createQueue(QueueElemCmpFn cmp, size_t elemSize, size_t max_capacity) {
    Queue queue = (Queue)malloc(sizeof(QueueCDT));
    queue->first = NULL;
    queue->last = NULL;
    queue->iterNode = NULL;
    queue->elemSize = elemSize;
    queue->size = 0;
    queue->cmpFn = cmp;
    queue->max_capacity = max_capacity;
    return queue;
}

Queue enqueue(Queue queue, void * data) {
    struct Node *newNode = (struct Node *)malloc(sizeof(struct Node));
    newNode->data = (uint8_t *)malloc(queue->elemSize);
    memcpy(newNode->data, data, queue->elemSize);
    newNode->next = NULL;

    if (queue->max_capacity != 0 && queue->size >= queue->max_capacity) {
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
    if (queue->size == 0) {
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

void * queuePeek(Queue queue, void * buffer) {
    if (queue->size == 0) {
        return NULL;
    }
    memcpy(buffer, queue->first->data, queue->elemSize);
    return buffer;
}

void * queueRemove(Queue queue, void * data) {
    struct Node *current = queue->first;
    struct Node *previous = NULL;

    while (current) {
        if (queue->cmpFn(current->data, data) == 0) {
            if (previous) {
                previous->next = current->next;
            } else {
                queue->first = current->next;
            }
            if (current == queue->last) {
                queue->last = previous;
            }
            void *removedData = malloc(queue->elemSize);
            memcpy(removedData, current->data, queue->elemSize);
            free(current->data);
            free(current);
            queue->size--;
            return removedData;
        }
        previous = current;
        current = current->next;
    }
    return NULL;
}

size_t queueSize(Queue queue) {
    if (!queue) return 0;
    return queue->size;
}

void freeQueue(Queue queue) {
    struct Node *current = queue->first;
    while (current) {
        struct Node *temp = current;
        current = current->next;
        free(temp->data);
        free(temp);
    }
    free(queue);
}

void queueBeginIter(Queue queue) {
    queue->iterNode = queue->first;
}

int queueHasNextIter(Queue queue) {
    return queue->iterNode != NULL;
}

void * queueIterNext(Queue queue, void * buffer) {
    if (!queueHasNextIter(queue)) {
        return NULL;
    }
    memcpy(buffer, queue->iterNode->data, queue->elemSize);
    queue->iterNode = queue->iterNode->next;
    return buffer;
}
