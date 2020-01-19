module time

class Timer {
  def reset() {
    self.start_usec = 0
    self.timestamps = []
  }
  def start() {
    self.reset()
    self.start_usec = now_usec()
  }
  def mark(mark_name) {
    elapsed = now_usec() - self.start_usec
    self.timestamps.append((mark_name, elapsed))
    return elapsed
  }
  def elapsed_usec() {
    return self.timestamps
  }
}