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
  def mark(name) {
    elapsed = now_usec() - self.start_usec
    self.timestamps.append((name, elapsed))
    return elapsed
  }
  def elapsed_usec() {
    return self.timestamps
  }
}