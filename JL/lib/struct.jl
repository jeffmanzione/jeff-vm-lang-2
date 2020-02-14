module struct

import builtin
import io
import sync

class Map {
  new(field sz) {
    self.table = []
    self.table[sz] = None
    self.keys = []
  }
  method hash__(k) {
    hval = builtin.hash(k)
    pos = hval % sz
    if pos < 0 pos = -pos
    return pos
  }
  method __set__(const k, const v) {
    pos = hash__(k)
    entries = table[pos]
    if ~entries {
      table[pos] = [(k,v)]
      keys.append(k)
      return None
    }
    for i=0, i<entries.len, i=i+1 {
      if k == entries[i][0] {
        old_v = entries[i][1]
        entries[i] = (k,v)
        return old_v
      }
    }
    entries.append((k,v))
    keys.append(k)
    return None
  }
  method __index__(const k) {
    pos = hash__(k)
    entries = table[pos]
    if ~entries {
      return None
    }
    for i=0, i<entries.len, i=i+1 {
      if eq(k, entries[i][0]) {
        return entries[i][1]
      }
    }
    return None
  }
  method __in__(const k) {
    __index__(k) != None
  }
  
  method iter() const {
    KVIterator(keys.iter(), self)
  }
}

class Set {
  field map
  new(sz) {
    map = Map(sz)
  }
  method insert(k) {
    map[k] = k
  }
  method __in__(k) {
    map[k]
  }
  method iter() {
    map.keys.iter()
  }
}

class Cache {
  field map, mutex
  new() {
    map = Map(255)
    mutex = sync.Mutex()
  }
  method get(k, factory, args, default=None) {
    mutex.acquire()
    v = map[k]
    if v {
      io.println('Hit')
      mutex.release()
      return v
    }
    try {
      v = factory(args)
    } catch e {
      v = default
    }
    map[k] = v
    mutex.release()
    return v
  }
}