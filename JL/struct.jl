module struct

import io
import sync

class Map {
  def new(sz) {
    self.sz = sz
    self.table = []
    self.table[self.sz] = None
    self.keys = []
  }
  def __set__(const k, const v) {
    hval = hash(k)
    if hval < 0 hval = -hval
    pos = hval % self.sz
    if ~self.table[pos] {
      self.table[pos] = [(k,v)]
      self.keys.append(k)
      return None
    }
    entries = self.table[pos]
    for i=0, i<entries.len, i=i+1 {
      if k == entries[i][0] {
        old_v = entries[i][1]
        entries[i] = (k,v)
        return old_v
      }
    }
    entries.append((k,v))
    self.keys.append(k)
    return None
  }
  def __index__(const k) {
    hval = hash(k)
    pos = hval % self.sz
    if ~self.table[pos] {
      return None
    }
    entries = self.table[pos]
    for i=0, i<entries.len, i=i+1 {
      if eq(k, entries[i][0]) {
        return entries[i][1]
      }
    }
    return None
  }
  def __in__(const k) {
    self.__index__(k) != None
  }
  
  def iter() const {
    KVIterator(self.keys.iter(), self)
  }
}

class Set {
  def new(sz) {
    self.map = Map(sz)
  }
  def insert(k) {
    self.map[k] = k
  }
  def __in__(k) {
    self.map[k]
  }
  def iter() {
    self.keys.iter()
  }
}

class Cache {
  def new() {
    self.map = Map(255)
    self.mutex = sync.Mutex()
  }
  def get(k, factory, args=None) {
    self.mutex.acquire()
    if k in self.map {
      return self.map[k]
    }
    v = factory(args)
    self.map[k] = v
    self.mutex.release()
    return v
  }
}

class Node {
  def new() {
    self.children = Map(255)
    self.value = None
  }
  def to_s() {
    tos = 'Node('
    if (self.value) {
      tos.extend('value=').extend(self.value)
    }
    for k, v in self.children {
      tos.extend(concat('(', k, '->', v, '),'))
    }
    tos.extend(')')
  }
}

class Trie {
  def new() {
    self.root = Node()
  }  
  def insert(key, value) {
    node, index_last_char = self.find_node(self.root, key)
    if ~index_last_char {
      index_last_char = 0
    }
    for (i, char) in key.substr(index_last_char, key.len) {
      node.children[char] = Node()
      node = node.children[char]
    }
    node.value = value
  }
  def find_node(node, key) {
    index_last_char = None
    for (i, char) in key {
     if char in node.children {
        node = node.children[char]
      } else {
        index_last_char = i
        return (node, index_last_char)
      }
    }
    (node, index_last_char)
  }
  def find(key) {
    node, i = self.find_node(self.root, key)
    if ~node return None
    node.value
  }
  def collect_child_values(node) {
    if node.value return [node.value]
    result = []
    for (k, child) in node.children {
      if child {
        result.extend(self.collect_child_values(child))
      }
    }
    result
  }
  def find_by_prefix(key) {
    node, i = self.find_node(self.root, key)
    if (~node) return []
    if i {
      if i < key.len return []
    }
    self.collect_child_values(node)
  }
  def to_s() {
    concat('Trie(', self.root, ')')
  }
}