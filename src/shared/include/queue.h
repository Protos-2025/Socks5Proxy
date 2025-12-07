#ifndef __QUEUE_H__
#define __QUEUE_H__

#include <stddef.h>
#include <stdint.h>

typedef struct QueueCDT * Queue;
typedef int (*QueueElemCmpFn)(void *, void *);
typedef void (*QueueElemFreeFn)(void *);

Queue create_queue(QueueElemFreeFn free_fn, size_t elem_size, size_t max_capacity);
Queue enqueue(Queue queue, void * data);
void * dequeue(Queue queue, void * buffer);
void * queue_peek(Queue queue, void * buffer);
size_t queue_size(Queue queue);
void free_queue(Queue queue);

void queue_begin_iter(Queue queue);
void * queue_iter_next(Queue queue, void * buffer);
int queue_has_next_iter(Queue queue);

#endif