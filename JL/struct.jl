module struct

import io

class Map {
  def new(sz) {
    self.sz = sz
    self.table = []
    self.table[self.sz] = None
    self.keys = []
  }
  def __set__(k, v) {
    hval = hash(k)
    pos = hval % self.sz
    if ~self.table[pos] {
      self.table[pos] = [(k,v)]
      self.keys.append(k)
      return None
    }
    entries = self.table[pos]
    for i=0, i<entries.len, i=i+1 {
      if eq(k, entries[i][0]) {
        old_v = entries[i][1]
        entries[i] = (k,v)
        return old_v
      }
    }
    entries.append((k,v))
    self.keys.append(k)
    return None
  }
  def __index__(k) {
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
  def __in__(k) {
    self.__index__(k) != None
  }
  
  def iter() {
    KVIterator(self.keys.iter(), self)
  }
}

class Set {
  def new(sz) {
    self.map = Map(sz)
  }
  def insert(k) {
    self.map.put(k, k)
  }
  def __in__(k) {
    self.map.get(k)
  }
  def iter() {
    self.keys.iter()
  }
}