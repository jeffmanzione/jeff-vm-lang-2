module struct

import io

class Map {
  def new(sz) {
    self.sz = sz
    self.table = []
    self.table[self.sz] = None
  }
  def put(k, v) {
    hval = hash(k)
    pos = hval % self.sz
    if ~self.table[pos] {
      self.table[pos] = [(k,v)]
      return None
    }
    entries = self.table[pos]
    io.println('>>>',entries)
    for i=0, i<entries.len, i=i+1 {
      if eq(k, entries[i][0]) {
        old_v = entries[i][1]
        entries[i] = (k,v)
        return old_v
      }
    }
    entries.append((k,v))
    return None
  }
  def get(k) {
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
}

class Set {
  def new(sz) {
    self.map = Map(sz)
  }
  def insert(k) {
    self.map.put(k, k)
  }
  def contains(k) {
    self.map.get(k)
  }
}