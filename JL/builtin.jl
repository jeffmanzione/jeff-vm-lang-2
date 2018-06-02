module builtin

import io

def cmp(o1, o2) {
  if (o1 is Object) & (o2 is Object) {
    return o1.cmp(o2)
  }
  o1 - o2
}

def range(start, end) {
  arr = []
  for i=start, i<end, i=i+1
    arr.append(i)
  arr
}

class Object {
  def to_s() self.class.name + '()'
  def cmp(o2) {
    o1.$adr - o2.$adr
  }
}

class Class {
  def to_s() self.name + '.class'
}

class Function {
  def to_s() self.class.name.copy().append('(').extend(self.$module.name).append('.').extend(self.name).append(')')
}

class Module {
  def to_s() 'Module(' + self.name + ')'
}

class Tuple {
  def to_s() {
    str = '('
    if self.len > 0 {
      if (self[0] is Object) {
        str.extend(self[0].to_s())
      } else str.append(self[0])
    }
    for i=1, i<self.len, i=i+1 {
      str.append(',')
      if (self[i] is Object) {
        str.extend(self[i].to_s())
      } else str.append(self[i])
    }
    str.append(')')
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
  def append(elt) {
    self[self.len] = elt
    return self
  }
  def extend(arr) {
    for i=0, i<arr.len, i=i+1 {
      self.append(arr[i])
    }
    return self
  }
  def to_s() {
    str = '['
    if self.len > 0 {
      if (self[0] is Object) {
        str.extend(self[0].to_s())
      } else str.append(self[0])
    }
    for i=1, i<self.len, i=i+1 {
      str.append(',')
      if (self[i] is Object) {
        str.extend(self[i].to_s())
      } else str.append(self[i])
    }
    str.append(']')
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
  def starts_with(array) {
    if array.len > self.len {
      return False 
    }
    for i=0, i<array.len, i=i+1 {
      if array[i] != self[i] {
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
      if array[array_len-i] != self[self_len-i] {
        return False
      }
    }
    True
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
  def new(array) {
    for i=0, i < |array|, i=i+1
      self.append(array[i])
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