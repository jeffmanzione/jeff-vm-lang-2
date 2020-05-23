module sync

import io
import struct

self.INFINITE = 2147483647
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

class ThreadPool {  
  field mutex, task_lock, threads, tasks
  new(field num_threads) {
    mutex = Mutex()
    task_lock = Semaphore(0, num_threads)
    threads = []
    tasks = []
    for i=0, i<num_threads, i=i+1 {
      threads.append(Thread(_execute_task, None))
    }
    for (_,t) in threads {
      t.start()
    }
  }
  method _execute_task() {
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

      if task{
        task.exec()
        task = None
      }
    }
  }
  method _execute_future(f) {
    mutex.acquire()
    if f.mark {
      mutex.release()
      return f
    }
    tasks.append(f)
    f.mark = True
    mutex.release()
    
    task_lock.unlock()
    return f
  }
  method execute(fn, args=None) {
    f = None
    if fn is TaskGraph {
      f = TaskGraphFuture(fn, self)
    } else {
      f = Future(fn, args, self)
    }
    return _execute_future(f)
  }
}

class Future {
  field listeners, listeners_mutex, result, has_result, read_mutex, mark
  new(field fn, field args, field ex) {
    listeners = []
    listeners_mutex = Mutex()
    result = None
    has_result = False
    read_mutex = Semaphore(0, 1)
    mark = False
  }
  method exec() {
    result = fn(args)
    listeners_mutex.acquire()
    has_result = True
    for (_,f) in listeners {
      f.args = result
      ex._execute_future(f)
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
  method then_do(fn) {
    return _then_do_future(Future(fn, None, ex))
  }
  method _then_do_future(f) {
    listeners_mutex.acquire()
    if has_result {
      f.args = result
      ex._execute_future(f)
    } else {
      listeners.append(f)
    }
    listeners_mutex.release()
    return f
  }
  method is_complete() {
    read_mutex.lock(0)
    res = has_result
    read_mutex.unlock()
    return res
  }
  method wait() {
    get()
  }
}

class TaskGraph {
  field next_id, nodes, roots
  new() {
    next_id = 0
    nodes = []
    roots = []
  }
  method add_node(work, deps=None) {
    id = _next_id()
    if ~deps {
      roots.append(id)
    }
    deps = _wrap_deps(deps)
    node = _WorkNode(id, deps, work)
    nodes.append(node)
    deps.each(node_id -> nodes[node_id]._add_dep_for(id))
    return id
  }
  method _next_id() {
    retval = next_id
    next_id = next_id + 1
    return retval
  }
  method _wrap_deps(deps) {
    if ~deps {
      return []
    } else if deps is Array {
      return deps
    } else {
      return [deps]
    }
  }
}

class _WorkNode {
  field deps_for
  new(field id, field depends_on, field work) {
    deps_for = []
  }
  method _add_dep_for(dep_id) {
    deps_for.append(dep_id)
  }
  method to_s() {
    return concat('_WorkNode(id=', id, ',depends_on=', depends_on, ',deps_for=', deps_for, ')')
  }
}

class TaskGraphFuture : Future {
  new(field task_graph, field ex) {
    self.Future.new(_execute_graph, None, ex)
  }
  ; Why do I have to do this?
  method get() {
    return self.Future.get().map(f -> f.get())
  }
  method exec() {
    return self.Future.exec()
  }
  method _execute_graph() {
    visited = []
    visited[task_graph.nodes.len - 1] = None
    _create_futures(visited)
    task_graph.roots.each(id -> ex._execute_future(visited[id]))
    return visited
  }
  method _create_futures(visited) {
    next_to_process = task_graph.roots.copy()
    while next_to_process.len > 0 {
      id = next_to_process.pop()
      ; Skip if already visited
      if visited[id] {
        continue
      }
      node = task_graph.nodes[id]
      future = _WorkNodeFuture(node, visited, task_graph, ex)
      visited[id] = future
      node.deps_for.each(next_to_process.append)
    }
    return visited
  }
}

class _WorkNodeFuture : Future {
  new(field node, field visited, field task_graph, field ex) {
    self.Future.new(node.work, None, ex)
  }
  method get() {
    return self.Future.get()
  }
  method exec() {
    result_map = {}
    for (_, dep) in node.depends_on {
      result_map[dep] = visited[dep].get()
    }
    self.Future.args = result_map
    self.Future.exec()
    node.deps_for.each((id) {
      child = task_graph.nodes[id]
      child_future = visited[id]
      all_deps_completed = child.depends_on.stream()
                                .map(id -> visited[id].is_complete())
                                .collect((a,b) -> a and b)
      if all_deps_completed {
        ex._execute_future(child_future)
      }
    })
  }
  method is_complete() {
    return self.Future.is_complete()
  }
}
