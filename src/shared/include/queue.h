#ifndef __QUEUE_H__
#define __QUEUE_H__

#include <stddef.h>
#include <stdint.h>

typedef struct QueueCDT * Queue;
typedef int (*QueueElemCmpFn)(void *, void *);

Queue createQueue(QueueElemCmpFn cmp, size_t elemSize, size_t max_capacity);
Queue enqueue(Queue queue, void * data);
void * dequeue(Queue queue, void * buffer);
void * queuePeek(Queue queue, void * buffer);
void * queueRemove(Queue queue, void * data);
size_t queueSize(Queue queue);
void freeQueue(Queue queue);

void queueBeginIter(Queue queue);
void * queueIterNext(Queue queue, void * buffer);
int queueHasNextIter(Queue queue);

#endif