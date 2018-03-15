module math

import io

self.epsilon = 0.000001
self.pi = 3.14159265359
self.e = 2.71828182845

def max(x, y)
	if x > y then x else y

def max3(x, y, z)
  max(max(x, y), z)

def sqr(x) x * x

def int_pow(x, n) {
  if 1 == n then x
  else x*int_pow(x,n-1)
}
  
def sqrt(n) {
    x = n
    y = 1f
    while (x - y) > epsilon {
        x = (x + y) / 2.0
        y = n / x
    }
    x
}

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