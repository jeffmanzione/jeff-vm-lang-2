module sync

import io

self.INIFINITE = 2147483647
self.LOCK_ACQUIRED = 0

class AtomicInt {
  def new(orig) {
    self.i = orig
    self.mutex = Mutex()
  }
  def set(v) {
    self.mutex.acquire()
    self.i = v
    self.mutex.release()
  }
  def get() {
    self.mutex.acquire()
    cpy = self.i
    self.mutex.release()
    return cpy
  }
  def inc(a) {
    self.mutex.acquire()
    self.i = self.i + a
    self.mutex.release()
  }
  def to_s() {
    self.mutex.acquire()
    tos = str(self.i)
    self.mutex.release()
    return tos
  }
}

class ThreadPoolExecutor {  
  def new(num_threads) {
    self.num_threads = num_threads
    self.mutex = Mutex()
    self.task_lock = Semaphore(0, num_threads)
    self.threads = []
    self.tasks = []
    for i=0, i<num_threads, i=i+1 {
      self.threads.append(Thread(self.execute_task, None))
    }
    for (_,t) in self.threads {
      t.start()
    }
  }
  
  def execute_task() {
    task = None
    while True {
      self.task_lock.lock(INFINITE)

      self.mutex.acquire()
      if self.tasks.len > 0 {
        task = self.tasks.pop(1)[0]
      } else {
        self.task_lock.unlock()
      }
      self.mutex.release()

      if task != None {
        task.exec()
        task = None
      }
    }
  }
  
  def execute_future(f) {
    self.mutex.acquire()
    self.tasks.append(f)
    self.mutex.release()
    
    self.task_lock.unlock()
    return f
  }
  
  def execute(fn, args=None) {
    f = Future(fn, args, self)
    return self.execute_future(f)
  }
}

class Future {
  def new(fn, args, ex) {
    self.fn = fn
    self.args = args
    self.ex = ex
    self.listeners = []
    self.listeners_mutex = Mutex()
    self.result = None
    self.has_result = False
    self.read_mutex = Semaphore(0, 1)
  }
  
  def exec() {
    self.result = self.fn(self.args)
    self.listeners_mutex.acquire()
    self.has_result = True
    for (_,f) in self.listeners {
      f.args = self.result
      self.ex.execute_future(f)
    }
    self.listeners_mutex.release()
    self.read_mutex.unlock()
  }
  
  def get() {
    while True {
      self.read_mutex.lock(INFINITE)
      if ~self.has_result {
        self.read_mutex.unlock()
        continue
      }
      self.read_mutex.unlock()
      return self.result
    }
  }
 
  def thenDo(fn) {
    f = Future(fn, None, self.ex)
    self.listeners_mutex.acquire()
    if self.has_result {
      ex.execute_future(f)
    } else {
      self.listeners.append(f)
    }
    self.listeners_mutex.release()
    return f
  }
  
  def wait() {
    self.get()
  }
}
