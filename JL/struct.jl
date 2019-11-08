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
  def hash__(k) {
    hval = hash(k)
    pos = hval % self.sz
    if pos < 0 pos = -pos
    return pos
  }
  def __set__(const k, const v) {
    pos = self.hash__(k)
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
    pos = self.hash__(k)
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
  def get(k, factory, args, default) {
    self.mutex.acquire()
    if k in self.map {
      v = self.map[k]
      self.mutex.release()
      return v
    }
    try {
      v = factory(args)
    } catch e {
      v = default
    }
    self.map[k] = v
    self.mutex.release()
    return v
  }
}