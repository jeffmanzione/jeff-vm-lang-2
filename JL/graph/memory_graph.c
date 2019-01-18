/*
 * heap.c
 *
 *  Created on: Sep 30, 2016
 *      Author: Jeff
 */

#include "memory_graph.h"

#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

#include "../arena/arena.h"
#include "../arena/strings.h"
#include "../datastructure/array.h"
#include "../datastructure/expando.h"
#include "../datastructure/map.h"
#include "../datastructure/tuple.h"
#include "../error.h"
#include "../ltable/ltable.h"
#include "../shared.h"
#include "../threads/thread_interface.h"
#include "memory.h"

typedef struct MemoryGraph_ {
  // ID-related bools
  bool rand_seeded, use_rand;
  uint32_t id_counter;
  Set/*<Node>*/nodes;
  Set/*<Node>*/roots;

//  Set/*<Thread>*/threads;
  ThreadHandle access_mutex;
} MemoryGraph;

NodeID new_id(MemoryGraph* graph) {
  ASSERT_NOT_NULL(graph);
  uint32_t int_id;
  if (graph->use_rand) {
    if (!graph->rand_seeded) {
      srand((uint32_t) time(NULL));
      graph->rand_seeded = true;
    }
    int_id = rand();
  } else {
    int_id = graph->id_counter++;
  }
  NodeID id = { int_id };
  return id;
}

uint32_t node_hasher(const void *node) {
  ASSERT_NOT_NULL(node);
  return ((Node *) node)->id.int_id;
}

int32_t node_comparator(const void *n1, const void *n2) {
  ASSERT_NOT_NULL(n1);
  ASSERT_NOT_NULL(n2);
  return ((Node *) n1)->id.int_id - ((Node *) n2)->id.int_id;
}

void ltable_fill() {
  CKey_set(CKey_resval, RESULT_VAL);
  CKey_set(CKey_name, NAME_KEY);
  CKey_set(CKey_class, CLASS_KEY);
  CKey_set(CKey_parent, PARENT);
  CKey_set(CKey_parents, PARENTS_KEY);
  CKey_set(CKey_parent_class, PARENT_CLASS);
}

MemoryGraph *memory_graph_create() {
  MemoryGraph *graph = ALLOC2(MemoryGraph);
  graph->access_mutex = mutex_create(NULL);
  set_init_default(&graph->nodes);
  set_init(&graph->roots, DEFAULT_TABLE_SZ, default_hasher, default_comparator);
  graph->rand_seeded = false;
  graph->use_rand = false;
  graph->id_counter = 0;
  CKey_init();
  ltable_fill();
  return graph;
}

uint32_t node_edge_hasher(const void *ptr) {
  ASSERT_NOT_NULL(ptr);
  NodeEdge *edge = (NodeEdge *) ptr;
  return node_hasher(edge->node);
}

int32_t node_edge_comparator(const void *ptr1, const void *ptr2) {
  ASSERT_NOT_NULL(ptr1);
  ASSERT_NOT_NULL(ptr2);
  NodeEdge *edge1 = (NodeEdge *) ptr1;
  NodeEdge *edge2 = (NodeEdge *) ptr2;
  return node_comparator(edge1->node, edge2->node);
}

Node *node_create(MemoryGraph *graph) {
  ASSERT_NOT_NULL(graph);
  Node *node = ARENA_ALLOC(Node);
  // TODO: Instead of locking here, give each thread its own arena and merge
  // them when threads finish.
  mutex_await(graph->access_mutex, INFINITE);
  node->id = new_id(graph);
  set_insert(&graph->nodes, node);
  mutex_release(graph->access_mutex);
  set_init(&node->parents, DEFAULT_TABLE_SZ, node_edge_hasher,
      node_edge_comparator);
  set_init(&node->children, DEFAULT_TABLE_SZ, node_edge_hasher,
      node_edge_comparator);
  node->access_mutex = mutex_create(NULL);
  return node;
}

void node_delete(MemoryGraph *graph, Node *node, bool free_mem) {
  ASSERT_NOT_NULL(graph);
  ASSERT_NOT_NULL(node);
  if (free_mem) {
    void delete_node_edge(void *ptr) {
      ARENA_DEALLOC(NodeEdge, (NodeEdge* ) ptr);
    }
    set_iterate(&node->children, delete_node_edge);
    set_iterate(&node->children, delete_node_edge);
  }
  set_finalize(&node->children);
  set_finalize(&node->parents);
  obj_delete_ptr(&node->obj, /*free_mem=*/free_mem);
  if (free_mem) {
    set_remove(&graph->nodes, node);
    ARENA_DEALLOC(Node, node);
  }
  mutex_close(node->access_mutex);
}

void memory_graph_delete(MemoryGraph *graph) {
  CKey_finalize();
  ASSERT_NOT_NULL(graph);
#ifdef DEBUG
  int node_count = 0;
  int field_count = 0;
  int children_count = 0;
#endif
  void delete_node_and_obj(void *p) {
    ASSERT_NOT_NULL(p);
    Node *node = (Node *) p;
#ifdef DEBUG
    node_count++;
    field_count += map_size(&node->obj.fields);
    Set *set = &node->children;
    if (set) {
      children_count += set_size(set);
    }
#endif
    node_delete(graph, node, /*free_mem=*/false);
  }
  // Do not adjust order of deletes
  set_finalize(&graph->roots);
  set_iterate(&graph->nodes, delete_node_and_obj);

#ifdef DEBUG
  fprintf(stdout, "There are %d members of the graph.\n"
      "Total/Avg # fields: %d/%.02f\n"
      "Total/Avg # children %d/%.02f\n", node_count, field_count,
      (field_count * 1.0) / node_count, children_count,
      (children_count * 1.0) / node_count);
  fflush(stdout);
#endif
  set_finalize(&graph->nodes);
  mutex_close(graph->access_mutex);
  DEALLOC(graph);
}

Element memory_graph_new_node(MemoryGraph *graph) {
  ASSERT_NOT_NULL(graph);
  Node *node = node_create(graph);
  node->obj.node = node;
  node->obj.type = OBJ;
  node->obj.is_external = false;
//  node->obj.rwlock = create_rwlock();
  // Set to none.
//  memset(node->obj.ltable, 0x0, sizeof(Element) * CKey_END);
  map_init_default(&node->obj.fields);
  map_init_default(&node->obj.consts);
  node->obj.parent_objs = expando(Object *, 4);
  Element e = { .type = OBJECT, .is_const = false, .obj = &node->obj, };
  memory_graph_set_field(graph, e, ADDRESS_KEY, create_int((int32_t) e.obj));
  return e;
}

NodeEdge *node_edge_create(Node *to) {
  NodeEdge *ne = ARENA_ALLOC(NodeEdge);
  ne->ref_count = 1;
  ne->node = to;
  return ne;
}

void acquire_all_mutex(const Node * const n1, const Node * const n2) {
  if (n1 == NULL && n2 == NULL) {
    return;
  }
  if (n1 != NULL && n2 == NULL) {
    mutex_await(n1->access_mutex, INFINITE);
    return;
  }
  if (n1 == NULL && n2 != NULL) {
    mutex_await(n2->access_mutex, INFINITE);
    return;
  }

  ThreadHandle first =
      n1->id.int_id > n2->id.int_id ? n1->access_mutex : n2->access_mutex;
  ThreadHandle second =
      n1->id.int_id > n2->id.int_id ? n2->access_mutex : n1->access_mutex;
  mutex_await(first, INFINITE);
  mutex_await(second, INFINITE);
}

void release_all_mutex(const Node * const n1, const Node * const n2) {
  if (n1 == NULL && n2 == NULL) {
    return;
  }
  if (n1 != NULL && n2 == NULL) {
    mutex_release(n1->access_mutex);
    return;
  }
  if (n1 == NULL && n2 != NULL) {
    mutex_release(n2->access_mutex);
    return;
  }
  ThreadHandle first =
      n1->id.int_id > n2->id.int_id ? n1->access_mutex : n2->access_mutex;
  ThreadHandle second =
      n1->id.int_id > n2->id.int_id ? n2->access_mutex : n1->access_mutex;
  mutex_release(second);
  mutex_release(first);
}

DEB_FN(void, memory_graph_inc_edge, MemoryGraph *graph,
    const Object * const parent, const Object * const child) {
  ASSERT_NOT_NULL(graph);
  Node *parent_node = parent->node;
  ASSERT_NOT_NULL(parent_node);
  Node *child_node = child->node;
  ASSERT_NOT_NULL(child_node);

  Set *children_of_parent = &parent_node->children;
  ASSERT_NOT_NULL(children_of_parent);
  NodeEdge tmp_child_edge = { child_node, -1 };
  NodeEdge *child_edge;

  Set *parents_of_child = &child_node->parents;
  ASSERT_NOT_NULL(parents_of_child);
  NodeEdge tmp_parent_edge = { parent_node, -1 };
  NodeEdge *parent_edge;

  acquire_all_mutex(parent_node, child_node);

  // if There is already an edge, increase the edge count
  if (NULL != (child_edge = set_lookup(children_of_parent, &tmp_child_edge))) {
    child_edge->ref_count++;
  } else {
    // Create edge from parent to child
    set_insert(children_of_parent, node_edge_create(child_node));
  }
  // if There is already an edge, increase the edge count
  if (NULL != (parent_edge = set_lookup(parents_of_child, &tmp_parent_edge))) {
    parent_edge->ref_count++;
  } else {
    // Create edge from child to parent
    set_insert(parents_of_child, node_edge_create(parent_node));
  }

  release_all_mutex(parent_node, child_node);
}
#define memory_graph_inc_edge(...) CALL_FN(memory_graph_inc_edge__, __VA_ARGS__)

Element memory_graph_create_root_element(MemoryGraph *graph) {
  ASSERT_NOT_NULL(graph);
  Element elt = memory_graph_new_node(graph);
  set_insert(&graph->roots, elt.obj->node);
  return elt;
}

DEB_FN(void, memory_graph_dec_edge, MemoryGraph *graph,
    const Object * const parent, const Object * const child) {
  ASSERT_NOT_NULL(graph);
  Node *parent_node = parent->node;
  ASSERT_NOT_NULL(parent_node);
  Node *child_node = child->node;
  ASSERT_NOT_NULL(child_node);

  Set *children_of_parent = &parent_node->children;
  ASSERT_NOT_NULL(children_of_parent);
  NodeEdge tmp_child_edge = { child_node, -1 };

  Set *parents_of_child = &child_node->parents;
  ASSERT_NOT_NULL(parents_of_child);
  NodeEdge tmp_parent_edge = { parent_node, -1 };

  acquire_all_mutex(parent_node, child_node);

  // Remove edge from parent to child
  NodeEdge *child_edge = set_lookup(children_of_parent, &tmp_child_edge);
  ASSERT_NOT_NULL(child_edge);
  --child_edge->ref_count;
  // Remove edge from child to parent
  NodeEdge *parent_edge = set_lookup(parents_of_child, &tmp_parent_edge);
  ASSERT_NOT_NULL(parent_edge);
  --parent_edge->ref_count;

  release_all_mutex(parent_node, child_node);
}
#define memory_graph_dec_edge(...) CALL_FN(memory_graph_dec_edge__, __VA_ARGS__)

const Node *node_for(const Element *e) {
  return (e->type == OBJECT) ? e->obj->node : NULL;
}

void memory_graph_set_field(MemoryGraph *graph, const Element parent,
    const char field_name[], const Element field_val) {

  ASSERT_NOT_NULL(graph);
  ASSERT(OBJECT == parent.type);
  ASSERT_NOT_NULL(parent.obj);
  Element existing = obj_get_field(parent, field_name);
  // If the field is already set to an object, remove the edge to the old child
  // node.
  if (OBJECT == existing.type) {
    memory_graph_dec_edge(graph, parent.obj, existing.obj);
  }
  if (OBJECT == field_val.type) {
    memory_graph_inc_edge(graph, parent.obj, field_val.obj);
  }

//  wait_for_mutex(parent.obj->node->access_mutex, INFINITE);
  obj_set_field(parent, field_name, field_val);
//  release_mutex(parent.obj->node->access_mutex);
}

void traverse_subtree(MemoryGraph *graph, Set *marked, Node *node) {
  ASSERT_NOT_NULL(graph);
  if (set_lookup(marked, node)) {
    return;
  }
  set_insert(marked, node);
  void traverse_subtree_helper(void *ptr) {
    NodeEdge *child_edge = (NodeEdge *) ptr;
    ASSERT_NOT_NULL(child_edge);
    ASSERT_NOT_NULL(child_edge->node);
    // Don't traverse edges which no longer are present
    if (child_edge->ref_count < 1) {
      set_remove(&node->children, child_edge);
      ARENA_DEALLOC(NodeEdge, child_edge);
      return;
    }
    traverse_subtree(graph, marked, child_edge->node);
  }
  set_iterate(&node->children, traverse_subtree_helper);
}

void memory_graph_free_space(MemoryGraph *graph) {
  ASSERT_NOT_NULL(graph);
  Set *marked = set_create_default();
  void traverse_graph(void *ptr) {
    Node *node = (Node *) ptr;
    ASSERT_NOT_NULL(node);
    traverse_subtree(graph, marked, node);
  }

  set_iterate(&graph->roots, traverse_graph);

  void delete_node_if_not_marked(void *p) {
    ASSERT_NOT_NULL(p);
    Node *node = (Node *) p;
    if (set_lookup(marked, node)) {
      return;
    }
    node_delete(graph, node, /*free_mem=*/true);
    ASSERT_NULL(set_lookup(&graph->nodes, node));
  }
  set_iterate(&graph->nodes, delete_node_if_not_marked);
  set_delete(marked);
}

Array *extract_array(Element element) {
  ASSERT(OBJECT == element.type);
  ASSERT(ARRAY == element.obj->type);
  ASSERT_NOT_NULL(element.obj->array);
  return element.obj->array;
}

void memory_graph_array_push(MemoryGraph *graph, const Element parent,
    const Element element) {
  ASSERT_NOT_NULL(graph);
  Array *arr = extract_array(parent);
  Array_push(arr, element);
  if (OBJECT == element.type) {
    memory_graph_inc_edge(graph, parent.obj, element.obj);
  }
  memory_graph_set_field(graph, parent, LENGTH_KEY,
      create_int(Array_size(arr)));
}

void memory_graph_array_set(MemoryGraph *graph, const Element parent,
    int64_t index, const Element element) {
  ASSERT(NOT_NULL(graph), index >= 0);
  Array *arr = extract_array(parent);
  if (index < Array_size(arr)) {
    Element old = Array_get(arr, index);
    if (old.type == OBJECT) {
      memory_graph_dec_edge(graph, parent.obj, old.obj);
    }
  }
  Array_set(arr, index, element);
  if (OBJECT == element.type) {
    memory_graph_inc_edge(graph, parent.obj, element.obj);
  }
  memory_graph_set_field(graph, parent, LENGTH_KEY,
      create_int(Array_size(arr)));
}

Element memory_graph_array_pop(MemoryGraph *graph, const Element parent) {
  ASSERT_NOT_NULL(graph);
  ASSERT(OBJECT == parent.type, ARRAY == parent.obj->type);
  Array *arr = extract_array(parent);
  ASSERT(Array_size(arr) > 0);
  Element element = Array_pop(arr);
  if (OBJECT == element.type) {
    memory_graph_dec_edge(graph, parent.obj, element.obj);
  }
  memory_graph_set_field(graph, parent, LENGTH_KEY,
      create_int(Array_size(arr)));
  return element;
}

void memory_graph_array_enqueue(MemoryGraph *graph, const Element parent,
    const Element element) {
  ASSERT_NOT_NULL(graph);
  Array *arr = extract_array(parent);
  Array_enqueue(arr, element);
  if (OBJECT == element.type) {
    memory_graph_inc_edge(graph, parent.obj, element.obj);
  }
  memory_graph_set_field(graph, parent, LENGTH_KEY,
      create_int(Array_size(arr)));
}

Element memory_graph_array_join(MemoryGraph *graph, const Element a1,
    const Element a2) {
  Element joined = create_array(graph);
  Array_append(joined.obj->array, a1.obj->array);
  Array_append(joined.obj->array, a2.obj->array);
  void append_child_edges(void *p) {
    NodeEdge *ne = (NodeEdge *) p;
    int i;
    for (i = 0; i < ne->ref_count; i++) {
      memory_graph_inc_edge(graph, joined.obj, &ne->node->obj);
    }
  }
  set_iterate(&a1.obj->node->children, append_child_edges);
  set_iterate(&a2.obj->node->children, append_child_edges);
  memory_graph_set_field(graph, joined, LENGTH_KEY,
      create_int(Array_size(joined.obj->array)));
  return joined;
}

Element memory_graph_array_dequeue(MemoryGraph *graph, const Element parent) {
  ASSERT_NOT_NULL(graph);
  Array *arr = extract_array(parent);
  Element element = Array_dequeue(arr);
  if (OBJECT == element.type) {
    memory_graph_dec_edge(graph, parent.obj, element.obj);
  }
  memory_graph_set_field(graph, parent, LENGTH_KEY,
      create_int(Array_size(arr)));
  return element;
}

Element memory_graph_array_remove(MemoryGraph *graph, const Element parent,
    int index) {
  ASSERT_NOT_NULL(graph);
  Array *arr = extract_array(parent);
  Element element = Array_remove(arr, index);
  if (OBJECT == element.type) {
    memory_graph_dec_edge(graph, parent.obj, element.obj);
  }
  memory_graph_set_field(graph, parent, LENGTH_KEY,
      create_int(Array_size(arr)));
  return element;
}

void memory_graph_tuple_add(MemoryGraph *graph, const Element tuple,
    const Element elt) {
  ASSERT(NOT_NULL(graph), OBJECT == tuple.type, TUPLE == tuple.obj->type);
  tuple_add(tuple.obj->tuple, elt);
  if (OBJECT == elt.type) {
    memory_graph_inc_edge(graph, tuple.obj, elt.obj);
  }
  memory_graph_set_field(graph, tuple, LENGTH_KEY,
      create_int(tuple_size(tuple.obj->tuple)));
}

void memory_graph_print(const MemoryGraph *graph, FILE *file) {
  ASSERT_NOT_NULL(graph);
  void print_node_id(void *ptr) {
    ASSERT_NOT_NULL(ptr);
    fprintf(file, "%u ", ((Node *) ptr)->id.int_id);
  }
  fprintf(file, "roots={ ");
  set_iterate(&graph->roots, print_node_id);
  fprintf(file, "}\n");
  void print_node_id2(void *ptr) {
    ASSERT_NOT_NULL(ptr);
    fprintf(file, "%u ", ((Node *) ptr)->id.int_id);
  }
  fprintf(file, "nodes(%d)={ ", set_size(&graph->nodes));
  set_iterate(&graph->nodes, print_node_id2);
  fprintf(file, "}\n");
  void print_edges_for_child(void *ptr) {
    ASSERT_NOT_NULL(ptr);
    Node *parent = (Node *) ptr;
    Set *children = (Set *) &parent->children;
    void print_edge(void *ptr) {
      ASSERT_NOT_NULL(ptr);
      NodeEdge *edge = (NodeEdge *) ptr;
      int i;
      for (i = 0; i < edge->ref_count; i++) {
        fprintf(file, "%u-->%u ", parent->id.int_id, edge->node->id.int_id);
      }
    }
    set_iterate(children, print_edge);
  }
  fprintf(file, "edges={ ");
  set_iterate(&graph->nodes, print_edges_for_child);
  fprintf(file, "}\n\n");
}


Mutex memory_graph_mutex(const MemoryGraph *graph) {
  return graph->access_mutex;
}
