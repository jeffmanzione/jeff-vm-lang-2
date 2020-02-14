module time

class Timer {
  field start_usec
  field timestamps
  method reset() {
    start_usec = 0
    timestamps = []
  }
  method start() {
    reset()
    start_usec = now_usec()
  }
  method mark(mark_name) {
    elapsed = now_usec() - start_usec
    timestamps.append((mark_name, elapsed))
    return elapsed
  }
  method elapsed_usec() {
    return timestamps
  }
}