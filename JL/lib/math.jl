module math

import builtin

self.epsilon = 0.000001
self.pi = 3.14159265359
self.e = 2.71828182845

def min(args) {
	m = args[0]
  for i in args {
    if (i < m) {
      m = i
    }
  }
  m
}

def max(args) {
	m = args[0]
  for i in args {
    if (i > m) {
      m = i
    }
  }
  m
}

def pow(num, power) {
  builtin.pow__(num, power)
}

; if 1 arg is specified, defaults to natural log.
def log(args) {
  builtin.log__(args)
}

def ln(x) {
  builtin.log__(x)
}

def log10(x) {
  log(10, x)
}

def sqrt(x) pow(x, 0.5) 

class Rect {
  def new(top, left, width, height) {
    self.top = top
    self.left = left
    self.width = width
    self.height = height
  }
  def area() { self.width * self.height }
  def contains(other) {
    if ~(other is Rect) {
      False
    } else self.left <= other & self.top <= other.top & self.width >= other.width & self.height >= other.width
  }
  def to_s() {
    rect = ''
    for i=0, i<self.top, i=i+1
      rect.append('\n')
    for i=0, i<self.left, i=i+1
      rect.append(' ')
    rect.append('+')
    for i=0,i<self.width,i=i+1
      rect.append('-')
    rect.append('+')
    rect.append('\n')
    for i=0, i<self.height, i=i+1 {
      for j=0, j<self.left, j=j+1
        rect.append(' ')
      rect.append('|')
      for j=0, j<self.width, j=j+1
        rect.append(' ')
      rect.append('|\n')
    }
    for i=0, i<self.left, i=i+1
      rect.append(' ')
    rect.append('+')
    for i=0,i<self.width,i=i+1
      rect.append('-')
    rect.append('+')
    rect
  }
}