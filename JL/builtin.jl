module builtin

import io
import math

def load_module(path) {
  load_module__(path)  ; External C function
}

def str(input) {
  if ~input return 'None'
  if input is Object return input.to_s()
  stringify__(input)  ; External C function
}

def concat(args) {
  if ~args {
    return 'None'
  }
  if ~(args.len) {
    return str(args)
  }
  strc = ''
  for i=0, i<args.len, i=i+1 {
    strc.extend(str(args[i]))
  }
  strc
}
self.cat = self.concat

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
  if o1 ^ o2 return False
  if hash(o1) != hash(o2) return False
  if (o1 is Object) & (o2 is Object) {
    return o1.eq(o2)
  }
  if (o1 is Object) | (o2 is Object) {
    return False
  }
  return Int(o1) == Int(o2)
}

def neq(o1, o2) {
  ~eq(o1, o2)
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
  def list() {
    result = []
    for i=self.start, i<self.end, i=i+self.inc {
      result.append(i)
    }
    result
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
  def elt_to_str(elt) {
    if elt is String {
      return '\''.extend(elt).extend('\'')
    }
    return str(elt)
  }
  def to_s() {
    '['.extend(','.join(self.map(self.elt_to_str))).extend(']')
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
  def collect(f, s=None) {
    if self.len == 0 {
      if s is Function {
        return s()
      } else {
        return s
      }
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
    for j=l, j<h, j=j+1 {
      if c(self[j], x) < 0 {
        i = i+1
        self.swap(i,j)
      }
    }
    i = i+1
    self.swap(i,h)
    i-1
  }
  def introsort(maxdepth, c, l, h) {
    if l < h {
      ; If recursed too many times, switch to insertion sort.
      if maxdepth <= 0 {
        self.inssort(c,l,h)
      } else { ; Otherwise quicksort.
        p = self.partition(c, l, h)
        self.introsort(maxdepth-1, c, l, p)
        self.introsort(maxdepth-1, c, p+1, h)
      }
    }
    self
  }
  def inssort(c, l=0, h=self.len-1) {
    i = l + 1
    while i < h {
      x = self[i]
      j = i-1
      while (j >= l)  {
        if (self[j] <= x) {
          break
        }
        j = j - 1
      }
      self.shift(j+1, i-(j+1), 1)
      self[j+1] = x
      i = i + 1
    }
    self
  }
  def sort() {
    self.introsort(math.log(self.len) * 2, cmp, 0, self.len-1)
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
  def extend(arr, start=None, end=None) {
    if ~(arr is String) raise Error('Cannot extend something not a String.')
    if start {
      if (start < 0) | (start > arr.len) {
        raise Error('Cannot extend with range before the start.')
      }
      if ~end {
        end = arr.len
      }
      self.extend_range__(arr, start, end)
    } else self.extend__(arr)
    self
  }
  def find(substr, index=0) {
    self.find__(substr, index)
  }
  def find_all(substr, index=0) {
    ; Remove after the bug is fixed.
    if ~index {
      index = 0
    }
    self.find_all__(substr, index)
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
  def substr(start, end=self.len) {
    string = ''
    ; Remove once this bug is fixed
    if ~end {
      end = self.len
    }
    for i = start, i < end, i = i + 1 {
      string.append(self[i])
    }
    string
  }
;  def split(del) {
;    del_len = del.len
;    parts = []
;    last_delim_end = 0
;    for i=0, i < self.len, i=i+1 {
;      if equals_range(self, i, del, 0, del_len) {
;        parts.append(self.substr(last_delim_end, i))
;        i = i + del_len
;        last_delim_end = i
;      }
;      if i==self.len-1 & last_delim_end < self.len - del_len {
;        parts.append(self.substr(last_delim_end, self.len))
;      }
;    }
;
;    return parts
;  }
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
