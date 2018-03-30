module builtin

import io

def range(start,end) {
  arr = []
  for i=start, i<end, i=i+1
    arr.append(i)
  arr
}

class Object {
  def to_s() self.class.name + '()'
}

class Class {
  def to_s() self.name + '.class'
}

class Function {
  def to_s() self.class.name + '(' + self.$module.name + '.' + self.name + ')'
}

class Module {
  def to_s() 'Module(' + self.name + ')'
}

class Array {
  def append(elt) {
    self[self.len] = elt
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
  def join(array) {
    if array.len == 0 {
      return ''
    }
    a = array[0]
    for i=1, i<array.len, i=i+1 {
      a = a + self + array[i]
    }
    a
  }
}