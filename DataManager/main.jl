import io
import math

io.println(math.sqrt(4.0))

(a, b, c) = (1, 2, 3)
io.println(a,b,c)

for a = 0, a < 10, a = a + 1
  io.println(a)

io.println(if ~Nil then 'True' else 'False')

a = 0
while a < 10 {
  io.println(a)
  a = a + 1
}

class Test {
  def new(a) {
    self.a = a
  }
  def test() {
    [1,2]
  }
  def test2() {
    [3,4]
  }
  def test3() {
    self.a
  }
;  def to_s() {
;    'Test(a=' + self.a + ')'
;  }
}

def test() io.println('test')

x = math.max3(4,5,6)
io.println(x)

a = []
b = [1, 2, 3]
io.println(b[1])
b[10] = 4
io.println(b)
b.a = 9
io.println(b.a)
io.println(self)
io.println(test)
test()

t1 = Test(55)
io.println(t1.a)
io.println(t1.test())
io.println(t1)
io.println(t1.test3())

println = io.println

z = (1, 2, 3)
println(z, z.len)

io.println('')
io.println(math.int_pow(2,5))

io.println('abc' is String)

pass = @(func, arg) func(arg)

pass(io.println, 'Hello, world!')

arr = [0, 1, 2, 3, 4, 5]
io.println(|arr|, arr.len)
for i = 0, i < |arr|, i=i+1 {
  io.println(arr[i])
}

if (True | True | True)
  io.println('Test')
io.println(self.$parent.$block.$ip, math)

if True {
  io.println('Test')
} else {
  io.println('NoTest')
}

x = @(a) {
  io.println(String([a,' turd'].flatten()))
}

pass(x, 'Hello')
io.println(math.pi, math.e)

io.println(Array.class)
io.println(Function)

io.println([0,1,2,3,4,5].map(@(x) range(0,x)).flatten())

;io.println(math.Rect(3, 2, 5, 4))

io.println(io.println)

x = Object()
io.println(x)
;io.println(x, 'Test')

crap = ['foo', 'bar', 'baz', 'quux']

io.println(crap.collect(@() {''}, @(a, e) {a + ', ' + e}))
io.println(', '.join(crap))

io.println('abcdefg'.starts_with('abc'), 'abcdefg'.starts_with('abb'))
io.println('abcdefg'.ends_with('efg'), 'abcdefg'.ends_with('ffg'))

io.println([1,2,3,4,5].sort(@(x,y) {x - y}))
io.println([5,4,3,2,1].sort(@(x,y) {x - y}))
io.println([4,3,5,1,2].sort(@(x,y) {x - y}))

io.println(io)
io.println(Object)