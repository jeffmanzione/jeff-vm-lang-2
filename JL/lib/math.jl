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
