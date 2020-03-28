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
  new(field start, field inc, field end) {}
  method __in__(n) {
    (n <= end) & (n >= start) & (n % inc == start % inc)
  }
  method __index__(i) {
    if (i < 0) raise Error(cat('Range index out of bounds. was ', i))
    val = start + i * inc
    if (val > end) raise Error(cat('Range index out of bounds. was ', i))
    val
  }
  method iter() {
    IndexIterator(self, 0, (end - start) / inc)
  }
  method to_s() {
    ':'.join(str(start), str(inc), str(end))
  }
  method list() {
    result = []
    for i=start, i<end, i=i+inc {
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
  method to_s() $module.concat(self.class.name, '@', self.$adr)
  method hash() {
    return $adr
  }
  method cmp(other) {
    $adr - other.$adr
  }
  method eq(other) $adr == other.$adr
  method neq(other) ~eq(other)
}

class Class {
  method to_s() {
    $module.concat(name, '.class')
  }
}

class Function {
  method to_s() $module.concat(self.class.name, '(',
                    self.module.name, '.',
                    name, ')')
  method call(args) {
    self.module.$lookup(name)(args)
  }
}

class Method : Function {
  method to_s() $module.concat(self.class.name, '(', self.method_path(), ')')
  method method_path() $module.concat($module.name, '.',
                           self.parent_class.name, '.',
                           self.Function.name)
  method call(obj, args) {
    obj.$lookup(self.Function.name)(args)
  }
}

class ExternalMethod {
  method to_s() $module.concat(self.class.name, '(', self.method_path(), ')')
  method method_path() $module.concat(self.module.name, '.',
                           self.parent_class.name, '.',
                           self.Function.name)
  method call(obj, args) {
    obj.$lookup(self.Function.name)(args)
  }
}

class MethodInstance {
  new(obj, meth) {
    self.obj = obj
    self.$method = meth
  }
  method call(args) {
    self.$method.call(self.obj, args)
  }
  method to_s() {
    $module.concat('MethodInstance(obj=', self.obj,
           ',$method=', self.$method, ')')
  }
}

class Module {
  field name, classes, functions
  method to_s() $module.concat('Module(name=', name, ', classes=', classes, ', functions=', functions, ')')
}

class Tuple {
  field len
  method to_s() {
    strng = '('
    if len > 0 {
      strng.extend(str(self[0]))
    }
    for i=1, i<len, i=i+1 {
      strng.extend(',').extend(str(self[i]))
    }
    strng.extend(')')
  }
  method eq(t) {
    if t.len != len {
      return False
    }
    for i=0, i<len, i=i+1 {
      if ~$module.eq(self[i],t[i]) return False
    }
    return True
  }
}

class Array {
  field len
  method copy() {
    cpy = Array()
    for i=0, i<len, i=i+1 {
      cpy.append(self[i])
    }
    cpy
  }
  method hash() {
    hashval = 5381
    for i=0, i < len, i=i+1 {
      c = $module.hash(self[i])
      hashval = ((hashval * 33) + hashval) + c
    }
    hashval
  }
  method append(elt) {
    self[len] = elt
    return self
  }
  method extend(arr) {
    if ~(arr is Array) raise Error('Cannot extend something not an array.')
    if (0 == arr.len) return self
    self_len = len
    self[self_len+arr.len-1] = None
    for i=0, i<arr.len, i=i+1 {
      self[self_len+i] = arr[i]
    }
    return self
  }
  method elt_to_str(elt) {
    if elt is String {
      return '\''.extend(elt).extend('\'')
    }
    return str(elt)
  }
  method to_s() {
    '['.extend(','.join(map(elt_to_str))).extend(']')
  }
  method map(f) {
    arr = []
    for i=len-1, i>=0, i=i-1 {
      arr[i] = f(self[i])
    }
    arr
  }
  method each(f) {
    for i=0, i<len, i=i+1 {
      f(self[i])
    }
  }
  method flatten() {
    arr = []
    for i=0, i<len, i=i+1 {
      if self[i] is Array {
        child = self[i]
        for j=0, j<child.len, j=j+1
           arr.append(child[j])
      } else arr.append(self[i])
    }
    arr
  }
  method collect(f, s=None) {
    if len == 0 {
      if s is Function {
        return s()
      } else {
        return s
      }
    }
    a = self[0]
    for i=1, i<len, i=i+1 {
      a = f(a, self[i])
    }
    a
  }
  method equals_range(array, start, end) {
    if (start < 0) return False
    if (end > array.len) return False
    if((array.len - start) > self.len) return False
    if(array.len < end) return False
 
    for i=start, i<end, i=i+1 {
      if ~$module.eq(array[i], self[i]) {
        return False
      }
    }
    True
  }
  method eq(o) {
    if !0 return False
    if o.len != len return False
    equals_range(o, 0, len)
  }
  method partition(c, l, h) {
    x = self[h]
    i = l - 1
    for j=l, j<h, j=j+1 {
      if c(self[j], x) < 0 {
        i = i+1
        ; TODO: Fix external methods since they cannot be
        ; called witout self. for some reason.
        self.swap(i,j)
      }
    }
    i = i+1
    self.swap(i,h)
    i-1
  }
  method introsort(maxdepth, c, l, h) {
    if l < h {
      ; If recursed too many times, switch to insertion sort.
      if maxdepth <= 0 {
        inssort(c,l,h)
      } else { ; Otherwise quicksort.
        p = partition(c, l, h)
        introsort(maxdepth-1, c, l, p)
        introsort(maxdepth-1, c, p+1, h)
      }
    }
    self
  }
  method inssort(c, l=0, h=len-1) {
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
  method sort() {
    self.introsort(math.log(len) * 2, cmp, 0, len-1)
  }
  method iter() {
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
  method append(elt) {
    self[len] = elt
    self
  }
  method extend(arr, start=None, end=None) {
    if ~(arr is String) raise Error('Cannot extend something not a String.')
    if start {
      if (start < 0) | (start > arr.len) {
        raise Error('Cannot extend with range before the start.')
      }
      if ~end {
        end = arr.len
      }
      if (end < start) {
        raise Error(concat('end must be >= start. start=', start, ' end=', end, '.'))
      }
      self.extend_range__(arr, start, end)
    } else self.extend__(arr)
    self
  }
  method find(substr, index=0) {
    self.find__(substr, index)
  }
  method find_all(substr, index=0) {
    self.find_all__(substr, index)
  }
  method __in__(substr) {
    find(substr) != None
  }
  method starts_with(array) {
    return equals_range(array, 0, array.len)
  }
  method join(array) {
    if array.len == 0 {
      return ''
    }
    a = array[0].copy()
    for i=1, i<array.len, i=i+1 {
      a.extend(self).extend(array[i])
    }
    a
  }
  method to_s() {
    self
  }
  method iter() {
    IndexIterator(self)
  }
  method substr(start, end=len) {
    return self.substr__(start, end)
  }
}

class Iterator {
  new(field has_next, field next) {}
}

class IndexIterator : Iterator {
  field indexable, start, end, i, index
  new(args) {
    if (args is Array) | (args is String) {
      indexable = args
      start = 0
      end = args.len
    } else if args is Tuple {
      (indexable, start, end) = args
    } else {
      raise Error(concat('Strange input: ', args))
    }
    i = -1
    index = start - 1
    self.Iterator.new(has_next_internal,
                      next_internal2)
  }
  method has_next_internal() {
    index < (end - 1)
  }
  method next_internal2() {
    i = i + 1
    index = index + 1
    (i, indexable[index])
  }
}

class KVIterator : Iterator {
  new(field key_iter, field container) {
    self.Iterator.new(key_iter.has_next,
                      next_internal2)
  }
  method next_internal2() {
    k = key_iter.next()[1]
    (k, container[k])
  }
}

class MemoizedFunction : Function {
  field result, has_result
  new(field target) {
    result = None
    has_result = False
  }
  method call(args) {
    if ~has_result {
      has_result = True
      result = target(args)
    }
    return result
  }
  method to_s() {
    cat(self.class.name, '(', target.to_s(), ')')
  }
}

def memoize(fn) {
  return MemoizedFunction(fn)
}
