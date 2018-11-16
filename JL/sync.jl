module sync

class AtomicInt {
  def new(orig) {
    self.value = orig
    self.mutex = Mutex()
  }
  def set(v) {
    mutex.acquire()
    self.value = v
    mutex.release()
  }
  def get() {
    mutex.acquire()
    cpy = self.value
    mutex.release()
    return cpy
  }
  def to_s() {
    mutex.acquire()
    tos = concat(self.class.name, '(', str(self.value), ')')
    mutex.release()
    return tos
  }
}