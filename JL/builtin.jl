module builtin

import io

$root.Int = self.Int
$root.Float = self.Float
$root.Char = self.Char

def load_module(path) {
  load_module__(path)  ; External C function
}

def str(input) {
  if ~input return 'None'
  if input is Object return input.to_s()
  stringify__(input)  ; External C function
}

def concat(args) {
  if ~(args.len) {
    return str(args)
  }
  strc = ''
  for i=0, i<args.len, i=i+1 {
    strc.extend(str(args[i]))
  }
  strc
}

def hash(v) {
  if v is Object {
    return v.hash()
  }
  Int(v)
}

def cmp(o1, o2) {
  if (o1 is Object) & (o2 is Object) {
    return o1.cmp(o2)
  }
  o1 - o2
}
; DO NOT CHANGE
def eq(o1, o2) {
  if hash(o1) != hash(o2) return False
  if (o1 is Object) & (o2 is Object) {
    return o1.eq(o2)
  }
  if (o1 is Object) | (o2 is Object) {
    return False
  }
  return Int(o1) == Int(o2)
}

class Range {
  def new(start, inc, end) {
    self.start = start
    self.inc = inc
    self.end = end
  }
  def __in__(n) {
    (n <= self.end) & (n >= self.start) & (n % self.inc == self.start % self.inc)
  }
  def __index__(i) {
    if (i < 0) raise Error(cat('Range index out of bounds. was ', i))
    val = self.start + i * self.inc
    if (val > self.end) raise Error(cat('Range index out of bounds. was ', i))
    val
  }
  def iter() {
    IndexIterator(self, 0, (self.end - self.start) / self.inc)
  }
  def to_s() {
    ':'.join(str(self.start), str(self.inc), str(self.end))
  }
}

def range(params) {
  start = params[0]
  if params.len == 3 {
    inc = params[1]
    end = params[2]
  } else {
    inc = 1
    end = params[1]
  }
  Range(start, inc, end)
}

class Object {
  def to_s() concat(self.class.name, '()')
  def hash() {
    return self.$adr
  }
  def cmp(other) {
    self.$adr - other.$adr
  }
  def eq(other) self.cmp(other) == 0
  def neq(other) ~self.eq(other)
}

class Class {
  def to_s() {
    concat(self.name, '.class')
  }
}

class Function {
  def to_s() concat(self.class.name, '(',
                    self.module.name, '.',
                    self.name, ')')
  def call(args) {
    self.module.$lookup(self.name)(args)
  }
}

class Method : Function {
  def to_s() concat(self.class.name, '(', self.method_path(), ')')
  def method_path() concat(self.module.name, '.',
                           self.parent_class.name, '.',
                           self.Function.name)
  def call(obj, args) {
    obj.$lookup(self.Function.name)(args)
  }
}

class ExternalMethod {
  def to_s() concat(self.class.name, '(', self.method_path(), ')')
  def method_path() concat(self.module.name, '.',
                           self.parent_class.name, '.',
                           self.Function.name)
  def call(obj, args) {
    obj.$lookup(self.Function.name)(args)
  }
}

class MethodInstance {
  def new(obj, method) {
    self.obj = obj
    self.method = method
  }
  def call(args) {
    self.method.call(self.obj, args)
  }
  def to_s() {
    concat(self.class.name, '(obj=', self.obj,
           ',method=', self.method, ')')
  }
}

class Module {
  def to_s() concat('Module(', self.name, ')')
}

class Tuple {
  def to_s() {
    strng = '('
    if self.len > 0 {
      strng.extend(str(self[0]))
    }
    for i=1, i<self.len, i=i+1 {
      strng.extend(',').extend(str(self[i]))
    }
    strng.extend(')')
  }
  def eq(t) {
    if t.len != self.len {
      return False
    }
    for i=0, i<self.len, i=i+1 {
      if ~eq(self[i],t[i]) return False
    }
    return True
  }
}

class Array {
  def copy() {
    cpy = Array()
    for i=0, i<|self|, i=i+1 {
      cpy.append(self[i])
    }
    cpy
  }
  def hash() {
    hashval = 5381
    for i=0, i < self.len, i=i+1 {
      c = hash(self[i])
      hashval = ((hashval * 33) + hashval) + c
    }
    hashval
  }
  def append(elt) {
    self[self.len] = elt
    return self
  }
  def extend(arr) {
    if ~(arr is Array) raise Error('Cannot extend something not an array.')
    if (0 == arr.len) return self
    self_len = self.len
    self[self_len+arr.len-1] = None
    for i=0, i<arr.len, i=i+1 {
      self[self_len+i] = arr[i]
    }
    return self
  }
  def to_s() {
    strng = '['
    if self.len > 0 {
      strng.extend(str(self[0]))
    }
    for i=1, i<self.len, i=i+1 {
      strng.extend(',').extend(str(self[i]))
    }
    strng.extend(']')
  }
  def map(f) {
    arr = []
    for i=self.len-1, i>=0, i=i-1 {
      arr[i] = f(self[i])
    }
    arr
  }
  def flatten() {
    arr = []
    for i=0, i<self.len, i=i+1 {
      if self[i] is Array {
        child = self[i]
        for j=0, j<child.len, j=j+1
           arr.append(child[j])
      } else arr.append(self[i])
    }
    arr
  }
  def collect(s, f) {
    if self.len == 0 {
      if s is Function {
        a = s()
      } else {
        a = s
      }
      return a
    }
    a = self[0]
    for i=1, i<self.len, i=i+1 {
      a = f(a, self[i])
    }
    a
  }
  def equals_range(array, start, end) {
    if (start < 0) return False
    if (end > array.len) return False
    if((array.len - start) > self.len) return False
    if(array.len < end) return False
 
    for i=start, i<end, i=i+1 {
      if ~eq(array[i], self[i]) {
        return False
      }
    }
    True
  }
  def eq(o) {
    if !0 return False
    if o.len != self.len return False
    self.equals_range(o, 0, self.len)
  }
  def partition(c, l, h) {
    x = self[h]
    i = l - 1
    for j=l, j<=h-1, j=j+1 {
      if c(self[j], x) <= 0 {
        i = i+1
        tmp = self[i]
        self[i] = self[j]
        self[j] = tmp
      }
    }
    tmp = self[i+1]
    self[i+1] = self[h]
    self[h] = tmp
    i+1
  }
  def qsort(c, l, h) {
    if c(l, h) < 0 {
      p = self.partition(c, l, h)
      self.qsort(c, l, p-1)
      self.qsort(c, p+1, h)
    }
    self
  }
  def sort(c) {
    self.qsort(c, 0, self.len-1)
  }
  def iter() {
    IndexIterator(self)
  }
}

def equals_range(a1, a1_start, a2, a2_start, length) {
  if (a1_start + length) > a1.len return False
  if (a2_start + length) > a2.len return False
  for i=0, i < length, i=i+1 {
    if a1[a1_start + i] != a2[a2_start+i] return False
  }
  True
}

class String {
  def hash() {
    hashval = 5381
    for i=0, i < self.len, i=i+1 {
      c = hash(self[i])
      hashval = ((hashval * 33) + hashval) + c
    }
    hashval
  }
  def append(elt) {
    self[self.len] = elt
    self
  }
  def extend(arr) {
    if ~(arr is String) raise Error('Cannot extend something not a String.')
    self.extend__(arr)
    self
  }
  def find(args) {
    index = 0
    if args is Tuple {
      substr = args[0]
      index = args[1]
    } else if args is String {
      substr = args
    } else {
      raise Error(concat('Expected (String, int) or (String) input, but was.', args))
    }
    self.find__(substr, index)
  }
  def __in__(substr) {
    self.find(substr) != None
  }
  
  def equals_range(array, start, end) {
    if (start < 0) return False
    if (end > array.len) return False
    if((array.len - start) > self.len) return False
    if(array.len < end) return False
 
    for i=start, i<end, i=i+1 {
      if ~eq(array[i], self[i]) {
        return False
      }
    }
    True
  }
  def eq(o) {
    if ~o return False
    if o.len != self.len return False
    self.equals_range(o, 0, self.len)
  }
  def starts_with(array) {
    if array.len > self.len {
      return False 
    }
    for i=0, i<array.len, i=i+1 {
      if eq(array[i], self[i]) {
        return False
      }
    }
    True
  }
  def ends_with(array) {
    if array.len > self.len {
      return False 
    }
    self_len = self.len-1
    array_len = array.len-1
    for i=0, i<=array_len, i=i+1 {
      if ~eq(array[array_len-i], self[self_len-i]) {
        return False
      }
    }
    True
  }
  def copy() {
    cpy = ''
    for i=0, i<|self|, i=i+1 {
      cpy.append(self[i])
    }
    cpy
  }
  def join(array) {
    if array.len == 0 {
      return ''
    }
    a = array[0].copy()
    for i=1, i<array.len, i=i+1 {
      a.extend(self).extend(array[i])
    }
    a
  }
  def to_s() {
    self
  }
  def iter() {
    IndexIterator(self)
  }
  def substr(start, end) {
    string = ''
    for i = start, i < end, i = i + 1 {
      string.append(self[i])
    }
    string
  }
  def split(del) {
    del_len = del.len
    parts = []
    last_delim_end = 0
    for i=0, i < self.len, i=i+1 {
      if equals_range(self, i, delim, 0, del_len) {
        parts.append(self.substr(last_delim_end, i))
        i = i + delim_len
        last_delim_end = i
      }
    }
    return parts
  }
}

; Immutable String
class Immutable {
  def new(mo) {
    self.mo = mo
  }
  def __index__(i) self.mo[i]
  def __in__(o) o in self.mo
  def to_s() self.mo.to_s()
}

class Iterator {
  def new(has_next, next) {
    self.has_next = has_next
    self.next = next    
  }
}

class IndexIterator : Iterator {
  def new(args) {
    if (args is Array) | (args is String) {
      self.indexable = args
      self.start = 0
      self.end = args.len
    } else if args is Tuple {
      self.indexable = args[0]
      self.start = args[1]
      self.end = args[2]
    } else {
      raise Error(concat('Strange input: ', args))
    }
    self.i = -1
    self.index = self.start - 1
    self.Iterator.new(self.has_next_internal,
                      self.next_internal2)
  }
  def has_next_internal() {
    self.index < (self.end - 1)
  }
  def next_internal2() {
    self.i = self.i + 1
    self.index = self.index + 1
    (self.i, self.indexable[self.index])
  }
}

class KVIterator : Iterator {
  def new(key_iter, container) {
    self.key_iter = key_iter
    self.container = container
    self.Iterator.new(self.key_iter.has_next,
                      self.next_internal2)
  }
  def next_internal2() {
    k = self.key_iter.next()[1]
    (k, self.container[k])
  }
}
