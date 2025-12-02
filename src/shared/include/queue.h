#ifndef __QUEUE_H__
#define __QUEUE_H__

#include <stddef.h>
#include <stdint.h>

typedef struct QueueCDT * Queue;
typedef int (*QueueElemCmpFn)(void *, void *);
typedef void (*QueueElemFreeFn)(void *);

Queue createQueue(QueueElemFreeFn freeFn, size_t elemSize, size_t max_capacity);
Queue enqueue(Queue queue, void * data);
void * dequeue(Queue queue, void * buffer);
void * queuePeek(Queue queue, void * buffer);
size_t queueSize(Queue queue);
void freeQueue(Queue queue);

void queueBeginIter(Queue queue);
void * queueIterNext(Queue queue, void * buffer);
int queueHasNextIter(Queue queue);

#endif