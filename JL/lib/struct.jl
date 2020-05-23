module struct

import builtin
import io
import sync

class Map {
  field table, keys
  new(field sz=31) {
    table = []
    table[sz] = None
    keys = []
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
      if k == entries[i][0] {
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
  method each(f) {
    for (k, v) in self {
      f(k, v)
    }
  }
  method to_s() {
    kvs = []
    each((k, v) -> kvs.append(concat(k, ': ', v)))
    return concat('{', ', '.join(kvs), '}')
  }
}

class RobinHoodMap {
  field table, keys, entry_count, entries_threshold, last, first
  new(field sz=31) {
    table = []
    table[sz-1] = None
    keys = []
    entry_count = 0
    entries_threshold = _calc_threshold(sz)
  }
  method _calc_threshold(table_size) {
    return 3 * table_size / 4
  }
  method _pos(hval, num_probes, table_sz) {
    (hval + num_probes * num_probes) % table_sz
  }
  method _set_helper(k, v, hval, table, table_sz) {
    num_probes = 0
    first_empty = None
    first_empty_index = -1
    num_probes_at_first_empty = -1
    while True {
      table_index = _pos(hval, num_probes, table_sz)
      num_probes = num_probes + 1
      me = table[table_index]
      ; Position is vacant.
      if ~me {
        if first_empty {
          ; Use the previously-empty slot if we don't find our element.
          me = first_empty
          num_probes = num_probes_at_first_empty
        } else {
          me = _MapEntry()
          table[table_index] = me
        }
        ; Take the vacant spot
        me.key = k
        me.value = v
        me.hval = hval
        me.num_probes = num_probes
        me.prev = last
        if last {
          me.prev.next = me
        }
        last = me
        if ~first {
          first = me
        }
        return True
      }
      ; Spot is vacant but previously used, mark it so we can use it later.
      if me.num_probes < 0 {
        if ~first_empty {
          first_empty = me
          num_probes_at_first_empty = num_probes
        }
        continue
      }
      ; Pair is already present in the table, so the mission is accomplished.
      if hval == me.hval and k == me.key {
        return False
      }
      ; Rob this entry if it did fewer probes.
      if me.num_probes < num_probes {
        ; Take its spot.
        tmp_k = me.key
        tmp_v = me.value
        tmp_hval = me.hval
        tmp_num_probes = me.num_probes
        me.key = k
        me.value = v
        me.hval = hval
        me.num_probes = num_probes
        k = tmp_k
        v = tmp_v
        hval = tmp_hval
        num_probes = tmp_num_probes
      }
    }
  }
  method __set__(const k, const v) {
    if entry_count > entries_threshold {
      _resize_table()
    }
    was_inserted = _set_helper(k, v, hash(k), table, sz)
    if was_inserted {
      entry_count = entry_count + 1
    }
    return was_inserted
  }
  method _index_entry(const k) {
    hval = hash(k)
    num_probes = 0
    while True {
      table_index = _pos(hval, num_probes, sz)
      num_probes = num_probes + 1
      me = table[table_index]
      if ~me {
        return None
      }
      if me.num_probes < 0 {
        continue
      }
      if hval == me.hval {
        if k == me.key {
          return me
        }
      }
    }
  }
  method __index__(const k) {
    me = _index_entry(k)
    if ~me {
      return None
    }
    return me.value
  }
  method _resize_table() {
    new_table_size = s2 * 2 + 1
    new_table = []
    new_table[new_table_size-1] = None
    new_first = None
    new_last = None
    each((entry) {
      _set_helper(entry.key, entry.value, entry.hval, new_table, new_table_size)
    })
    table = new_table
    sz = new_table_size
    first = new_first
    last = new_last
    entries_thresh = _calc_threshold(new_table_size)
  }
  method __in__(const k) {
    __index__(k)
  }
  method iter() const {}
  method each(f) {
    for me=first, me, me=me.next {
      f(me)
    }
  }
  method to_s() {
    kvs = []
    each(entry -> kvs.append(concat(entry.key, ': ', entry.value)))
    return concat('{', ', '.join(kvs), '}')
  }
}

class _MapEntry {
  field key, value, hval, num_probes
}

class Set {
  field map
  new(sz=31) {
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
  method each(f) {
    map.each((k, v) -> f(k))
  }
}

class Cache {
  field map, mutex
  new(sz=255) {
    map = Map(sz)
    mutex = sync.Mutex()
  }
  method get(k, factory, default=None) {
    mutex.acquire()
    v = map[k]
    if v {
      mutex.release()
      return v
    }
    try {
      v = factory(k)
    } catch e {
      v = default
    }
    map[k] = v
    mutex.release()
    return v
  }
}

class LoadingCache : Cache, Function {
  new(field factory, sz=255) {
    self.Cache.new(sz)
  }
  method get(const k) {
    self.Cache.get(k, factory)
  }
  method call(const k) {
    get(k)
  }
}

def cache(factory) {
  return LoadingCache(factory)
}
