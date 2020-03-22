module sync

import io

self.INIFINITE = 2147483647
self.LOCK_ACQUIRED = 0

class AtomicInt {
  field mutex
  new(field i) {
    mutex = Mutex()
  }
  method set(v) {
    mutex.acquire()
    i = v
    mutex.release()
  }
  method get() {
    mutex.acquire()
    cpy = i
    mutex.release()
    return cpy
  }
  method inc(a) {
    mutex.acquire()
    i = i + a
    mutex.release()
  }
  method to_s() {
    mutex.acquire()
    tos = str(i)
    mutex.release()
    return tos
  }
}

class ThreadPoolExecutor {  
  field mutex, task_lock, threads, tasks
  new(field num_threads) {
    mutex = Mutex()
    task_lock = Semaphore(0, num_threads)
    threads = []
    tasks = []
    for i=0, i<num_threads, i=i+1 {
      threads.append(Thread(execute_task, None))
    }
    for (_,t) in threads {
      t.start()
    }
  }
  method execute_task() {
    task = None
    while True {
      task_lock.lock(INFINITE)

      mutex.acquire()
      if tasks.len > 0 {
        task = tasks.pop(1)[0]
      } else {
        task_lock.unlock()
      }
      mutex.release()

      if task != None {
        task.exec()
        task = None
      }
    }
  }
  method execute_future(f) {
    mutex.acquire()
    tasks.append(f)
    mutex.release()
    
    task_lock.unlock()
    return f
  }
  method execute(fn, args=None) {
    f = Future(fn, args, self)
    return execute_future(f)
  }
}

class Future {
  field listeners, listeners_mutex, result, has_result, read_mutex
  new(field fn, field args, field ex) {
    listeners = []
    listeners_mutex = Mutex()
    result = None
    has_result = False
    read_mutex = Semaphore(0, 1)
  }
  method exec() {
    result = fn(args)
    listeners_mutex.acquire()
    has_result = True
    for (_,f) in listeners {
      f.args = result
      ex.execute_future(f)
    }
    listeners_mutex.release()
    read_mutex.unlock()
  }
  method get() {
    while True {
      read_mutex.lock(INFINITE)
      if ~has_result {
        read_mutex.unlock()
        continue
      }
      read_mutex.unlock()
      return result
    }
  }
  method thenDo(fn) {
    f = Future(fn, None, ex)
    listeners_mutex.acquire()
    if has_result {
      f.args = result
      ex.execute_future(f)
    } else {
      listeners.append(f)
    }
    listeners_mutex.release()
    return f
  }
  method wait() {
    get()
  }
}
