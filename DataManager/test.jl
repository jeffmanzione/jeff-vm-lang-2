def max(x, y) {
  if x > y 
  then x 
  else y
}

def max3(x, y, z)
  max(max(x, y), z)
  
;class Rect {
;  field top, left, width, height
;  def new(top, left, width, height) {
;    self.top = top
;    self.left = left
;    self.width = width
;    self.height = height
;  }
;  def area() width * height
;  def contains(Rect other)
;    self.left <= other 
;        & self.top <= other.top 
;        & self.width >= other.width 
;        & self.height >= other.width  
;}