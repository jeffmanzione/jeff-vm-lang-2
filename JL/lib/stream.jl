module stream

self.list = ((a, x) -> a.append(x), () -> [])
self.sum = (a, x) -> a + x

class Stream {
  field fns
  new(field iterable) {
    fns = []
  }
  method map(fn) {
    fns.append(fn)
    return self
  }
  method each(fn) {
    for (_, elt) in iterable {
      result = _apply_fns(elt)
      fn(result)
    }
  }
  method collect(fn, fn_acc=None) {
    acc = if fn_acc then fn_acc() else _apply_fns(iterable[0])
    for (i, elt) in iterable {
      if i == 0 and ~fn_acc {
        continue
      }
      acc = fn(acc, _apply_fns(elt))
    }
    return acc
  }
  method _apply_fns(elt) {
    result = elt
    for (_, func) in fns {
      result = func(result)
    }
    return result
  }
}