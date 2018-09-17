module builtin

import io

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
  v
}

def cmp(o1, o2) {
  if (o1 is Object) & (o2 is Object) {
    return o1.cmp(o2)
  }
  o1 - o2
}
; Do NOT CHANGE
def eq(o1, o2) {
  if hash(o1) != hash(o2) return False
  if (o1 is Object) & (o2 is Object) {
    return o1.eq(o2)
  }
  if (o1 is Object) | (o2 is Object) {
    return False
  }
  return o1 == o2
}

def range(params) {
  start = params[0]
  end = params[1]
  if params.len == 3 {
    inc = params[2]
  } else {
    inc = 1
  }
  range_inner(start, end, inc)
}

def range_inner(start, end, inc) {
  arr = []
  for i=start, i<end, i=i+inc
    arr.append(i)
  arr
}

class Object {
  def to_s() self.class.name.copy().extend('()')
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
    self.name.copy().extend('.class')
  }
}

class Function {
  def to_s() self.class.name.copy().extend('(').extend(self.$module.name).extend('.').extend(self.name).extend(')')
}

class Method {
  def to_s() self.class.name.copy().extend('(').extend(self.$module.name).extend('.').extend(self.parent[0].name).extend('.').extend(self.name).extend(')')
}

class Module {
  def to_s() 'Module(' + self.name + ')'
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
    return self
  }
  def extend(arr) {
    if ~(arr is String) raise Error('Cannot extend something not a String.')
    self.__extend__(arr)
    return self
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
}