/*
 * memory_graph.h
 *
 *  Created on: Sep 30, 2016
 *      Author: Jeff
 */

#ifndef MEMORY_GRAPH_H_
#define MEMORY_GRAPH_H_

#include <stdint.h>
#include <stdio.h>

#include "../datastructure/set.h"
#include "../element.h"
#include "../threads/thread_interface.h"

typedef struct MemoryGraph_ MemoryGraph;

// Needed in header for arenas.
typedef struct NodeID_ {
  uint32_t int_id;
} NodeID;
// Needed in header for arenas.
typedef struct Node_ {
  NodeID id;
  Object obj;
  Set parents;
  Set children;
#ifdef ENABLE_MEMORY_LOCK
  ThreadHandle access_mutex;
#endif
} Node;
// Needed in header for arenas.
typedef struct {
  Node *node;
  uint32_t ref_count;
} NodeEdge;

// Creates a memory graph
MemoryGraph *memory_graph_create();
// Deletes a memory graph
void memory_graph_delete(MemoryGraph *memory_graph);
// Creates a root element
Element memory_graph_create_root_element(MemoryGraph *graph);
// Creates a new object.
Element memory_graph_new_node(MemoryGraph *graph);
// Creates an edge from one node to another node
DEB_FN(void, memory_graph_set_field, MemoryGraph *memory_graph, const Element parent,
                            const char field_name[], const Element field_val);
#define memory_graph_set_field(...) CALL_FN(memory_graph_set_field__, __VA_ARGS__)

void memory_graph_set_var(MemoryGraph *graph, const Element block,
                          const char field_name[], const Element field_val);

void memory_graph_array_push(MemoryGraph *graph, const Element parent,
                             const Element element);

void memory_graph_array_set(MemoryGraph *graph, const Element parent,
                            int64_t index, const Element element);

Element memory_graph_array_pop(MemoryGraph *graph, const Element parent);

void memory_graph_array_enqueue(MemoryGraph *graph, const Element parent,
                                const Element element);
Element memory_graph_array_dequeue(MemoryGraph *graph, const Element parent);

Element memory_graph_array_remove(MemoryGraph *graph, const Element parent,
                                  int index);

void memory_graph_array_shift(MemoryGraph *graph, const Element parent,
                              int start, int count, int shift);

Element memory_graph_array_join(MemoryGraph *graph, const Element a1,
                                const Element a2);

void memory_graph_tuple_add(MemoryGraph *graph, const Element tuple,
                            const Element elt);
// Removes all unreachable nodes in the graph
int memory_graph_free_space(MemoryGraph *memory_graph);

void memory_graph_print(const MemoryGraph *graph, FILE *file);

Array *extract_array(Element element);

Mutex memory_graph_mutex(const MemoryGraph *graph);

// Element memory_graph_new_thread();

const Node *node_for(const Element *e);

#ifdef ENABLE_MEMORY_LOCK
void acquire_all_mutex(const Node *const n1, const Node *const n2);
void release_all_mutex(const Node *const n1, const Node *const n2);
#endif

#endif /* MEMORY_GRAPH_H_ */
