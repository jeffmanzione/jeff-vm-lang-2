/*
 * queue.c
 *
 *  Created on: Jan 6, 2016
 *      Author: Jeff
 */


#include "queue.h"

#include <stdbool.h>

#include "error.h"
#include "memory.h"

size_t queue_size(const Queue const * q) {
  return q->size;
}

void queue_init(Queue *queue) {
  queue->size = 0;
  queue->head = NULL;
  queue->tail = NULL;
}

void queue_deep_delete(Queue *queue, Deleter del) {
  ASSERT_NOT_NULL(queue);
  while (queue->size > 0) {
    del(queue_remove(queue));
  }
}

void queue_shallow_delete(Queue *queue) {
  ASSERT_NOT_NULL(queue);
  while (queue->size > 0) {
    queue_remove(queue);
  }
}

void queue_iterate(Queue *queue, Q_Action act) {
  ASSERT_NOT_NULL(queue);
  QueueElement *elt = queue->head;

  while (NULL != elt) {
    act(elt->value);
    elt = elt->next;
  }
}

void queue_iterate_until(Queue *queue, Q_ActionUntil act) {
  ASSERT_NOT_NULL(queue);
  QueueElement *elt = queue->head;

  while (NULL != elt) {
    if (act(elt->value)) {
      return;
    }
    elt = elt->next;
  }
}

void queue_iterate_mutable(Queue *queue, Q_MutableAction act) {
  ASSERT_NOT_NULL(queue);
  QueueElement *elt = queue->head;

  while (NULL != elt) {
    act((void *) elt->value);
    elt = elt->next;
  }
}

void *queue_peek(const Queue *queue) {
  ASSERT_NOT_NULL(queue);
  if (NULL == queue->head) {
    return NULL;
  }

  //printf("QUEUE PEEK: %s\n", ((Token *) queue->head->value)->text);

  return (void *) queue->head->value;
}

void *queue_last(const Queue *queue) {
  ASSERT_NOT_NULL(queue);
  if (NULL == queue->tail) {
    return NULL;
  }

  return (void *) queue->tail->value;
}

void *queue_remove(Queue *queue) {
  ASSERT_NOT_NULL(queue);
  void *to_remove;
  if (NULL == queue->head) {
    return NULL;
  }

  to_remove = (void *) queue->head->value;
  QueueElement *old_ptr = queue->head;
  queue->head = queue->head->next;

  if (NULL == queue->head) {
    queue->tail = NULL;
  } else {
    queue->head->prev = NULL;
  }

  DEALLOC(old_ptr);
  queue->size--;

  return to_remove;
}

void *queue_remove_last(Queue *queue) {
  ASSERT_NOT_NULL(queue);
  if (NULL == queue->tail) {
    return NULL;
  }
  void *to_remove = (void *) queue->tail->value;
  if (queue->tail->prev != NULL) {
    queue->tail->prev->next = NULL;
  }
  QueueElement *old_ptr = queue->tail;
  queue->tail = queue->tail->prev;

  DEALLOC(old_ptr);
  queue->size--;

  return to_remove;
}

bool queue_remove_elt(Queue *queue, const void *to_remove) {
  ASSERT_NOT_NULL(queue);
  if (NULL == queue->head) {
    return false;
  }

  QueueElement *prev = NULL;
  QueueElement *elt = queue->head;

  while (elt != NULL && elt->value != to_remove) {
    prev = elt;
    elt = elt->next;
  }

  if (NULL == elt) {
    return false;
  }

  if (queue->head == elt) {
    queue->head = elt->next;
  }
  if (queue->tail == elt) {
    queue->tail = prev;
  }
  if (prev != NULL) {
    prev->next = elt->next;
  }
  if (elt->next != NULL) {
    elt->next->prev = prev;
  }

  DEALLOC(elt);

  queue->size--;

  return true;
}

void queue_add(Queue *queue, const void *elt) {
  ASSERT_NOT_NULL(queue);
  QueueElement *new_elt;

  new_elt = ALLOC2(QueueElement);
  new_elt->value = elt;
  new_elt->next = NULL;
  new_elt->prev = queue->tail;

  if (NULL == queue->tail) {
    queue->head = queue->tail = new_elt;
    queue->size = 1;
  } else {
    queue->tail->next = new_elt;
    queue->tail = new_elt;
    queue->size++;
  }
  //printf("QUEUE SIZE = %d\n", queue->size);
}

void queue_add_front(Queue *queue, const void *elt) {
  ASSERT_NOT_NULL(queue);
  QueueElement *new_elt;

  new_elt = ALLOC2(QueueElement);
  new_elt->value = elt;
  new_elt->next = queue->head;
  new_elt->prev = NULL;

  if (NULL == queue->tail) {
    queue->head = queue->tail = new_elt;
    queue->size = 1;
  } else {
    queue->head = new_elt;
    queue->size++;
  }
}

void queue_steal(Queue *thief, Queue *sap) {
  ASSERT(NOT_NULL(thief), NOT_NULL(sap));
  if (sap->size <= 0) {
    return;
  }
  thief->tail->next = sap->head;
  thief->tail = sap->tail;
  thief->size += sap->size;
  sap->size = 0;
  sap->head = sap->tail = NULL;
}
