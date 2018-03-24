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

#include "element.h"
#include "set.h"

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
void memory_graph_set_field(MemoryGraph *memory_graph, const Element parent,
    const char field_name[], const Element field_val);

void memory_graph_array_push(MemoryGraph *graph, const Element parent,
    const Element element);

void memory_graph_array_set(MemoryGraph *graph, const Element parent,
    int64_t index, const Element element);

Element memory_graph_array_pop(MemoryGraph *graph, const Element parent);

void memory_graph_array_enqueue(MemoryGraph *graph, const Element parent,
    const Element element);
Element memory_graph_array_dequeue(MemoryGraph *graph, const Element parent);

Element memory_graph_array_join(MemoryGraph *graph, const Element a1,
    const Element a2);

void memory_graph_tuple_add(MemoryGraph *graph, const Element tuple,
    const Element elt);
// Removes all unreachable nodes in the graph
void memory_graph_free_space(MemoryGraph *memory_graph);

void memory_graph_print(const MemoryGraph *graph, FILE *file);

#endif /* MEMORY_GRAPH_H_ */
