module stream

self.list = ((a, x) -> a.append(x), () -> [])
self.sum = (a, x) -> a + x

; Lazy-computes map() and collect() operations on iterables.
class Stream {
  field fns
  new(field iterable) {
    fns = []
  }
  ; Applies [fn] to all members of the stream.
  ; The order of the iterable is unaffected.
  method map(fn) {
    fns.append(fn)
    return self
  }
  ; Applies [fn] to the each member of the stream.
  ; 
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