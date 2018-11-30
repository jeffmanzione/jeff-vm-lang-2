module sync

class NonAtomicInt {
   def new(orig) {
    self.value = orig
  }
  def set(v) {
    self.value = v
  }
  def get() {
    return self.value
  }
  def to_s() {
    return concat(self.class.name, '(', str(self.value), ')')
  } 
}

class AtomicInt : NonAtomicInt {
  def new(orig) {
    self.NonAtomicInt.new(orig)
    self.mutex = Mutex()
  }
  def set(v) {
    mutex.acquire()
    self.NonAtomicInt.set(v)
    mutex.release()
  }
  def get() {
    mutex.acquire()
    cpy = self.NonAtomicInt.get()
    mutex.release()
    return cpy
  }
  def to_s() {
    mutex.acquire()
    tos = self.NonAtomicInt.to_s()
    mutex.release()
    return tos
  }
}