/*
 * queue.h
 *
 *  Created on: Jan 6, 2016
 *      Author: Jeff
 */

#ifndef QUEUE_H_
#define QUEUE_H_

#include <stddef.h>
#include <stdbool.h>

typedef struct Queue_ Queue;
typedef struct QueueElement_ QueueElement;

typedef void (*Q_Action)(const void const *);
typedef bool (*Q_ActionUntil)(const void const *);
typedef void (*Q_MutableAction)(void *);
typedef Q_MutableAction Deleter;

typedef struct QueueElement_ {
  QueueElement *next, *prev;
  const void *value;
} QueueElement;


typedef struct Queue_ {
  QueueElement *head;
  QueueElement *tail;
  size_t size;
} Queue;

void queue_init(Queue *queue);
void queue_shallow_delete(Queue *queue);
void queue_deep_delete(Queue *queue, Deleter del);
void queue_iterate(Queue *queue, Q_Action act);
void queue_iterate_until(Queue *queue, Q_ActionUntil act);
void queue_iterate_mutable(Queue *queue, Q_MutableAction act);
void *queue_peek(const Queue *queue);
void *queue_peek_last(const Queue *queue);
void *queue_last(const Queue *queue);
void *queue_remove(Queue *queue);
void *queue_remove_last(Queue *queue);
bool queue_remove_elt(Queue *queue, const void *elt);
void queue_add(Queue *queue, const void *elt);
void queue_add_front(Queue *queue, const void *elt);
void queue_steal(Queue *theif, Queue *sap);
size_t queue_size(const Queue const * q);
#endif /* QUEUE_H_ */
