module toy_server

import io
import net
import struct
import sync
import time

class ToyServer {
  def new(port, num_threads) {
    net.init()
    self.port = port
    self.num_threads = num_threads
    self.sock = net.Socket(net.AF_INET, net.SOCK_STREAM, port, 0, num_threads)
    if (num_threads > 1) {
      self.ex = sync.ThreadPoolExecutor(num_threads)
    }
    self.cache = struct.Cache()
    self.text_renderer = net.CachedTextRenderer()

    self.handler = net.RequestHandler()

    self.handler.register(@(server, request, params) { request.path == '/' },
                          @(server, request, params) {
      request.path = '/index.html'
      server.handler.handle(server, request, params)
    })
    self.handler.register(@(server, request, params) {request.path.ends_with('.ico') | request.path.ends_with('.css') | request.path.ends_with('.js')}, 
                          @(server, request, params) {
      self.cache.get(request.path, @(request) {
        index = net.read_entire_file(request.path)
      }, request)
    })
    self.handler.register_else(@(server, request, params) {
      src = self.cache.get(request.path, @(request) {
        text = net.read_entire_file(request.path)
        if ~request.path.ends_with('.html') {
          return ''.extend('<pre>')
             .extend(net.html_escape(text))
             .extend('</pre>')
        } else {
          return text
        }
      }, request)
      return server.text_renderer.get(request.path, src, params)
    })
  }
  
  def get_header(req) {
    if req.path.ends_with('.css') {
      return net.HEADER_GENERIC_200_CSS
    }
    if req.path.ends_with('.js') {
      return net.HEADER_GENERIC_200_JS
    }
    return net.HEADER_GENERIC_200_HTML
  }
  
  def make_params(r_token) {
    params = struct.Map(31)
    params['{{r-token}}'] = r_token
    return params
  }
  
  def handle_requests() {
    while (True) {
      io.println(concat('Thread ', $thread.id, ' is listening...'))
      timer = time.Timer()
      handle = self.sock.accept()
      timer.start()
      msg = handle.receive()
      timer.mark('Receive')
      
      request = net.parse_request(msg)
      timer.mark('Parse')
      
      if request {
        handle.send(self.get_header(request))
        timer.mark('Header sent')
        io.println(concat('Fetching ', request))
        params = self.make_params(str(time.now_usec()))
        timer.mark('Params created')
        file_lines = self.handler.handle(self, request, params)
        timer.mark('File rendered')
        if file_lines {
          handle.send(file_lines)
          timer.mark('File sent')
        }
      } else {
        io.println('Failed: ', msg)
      }
      self.pretty_print_timer(request, timer.elapsed_usec())
      handle.close()
    }
  }
  
  def pretty_print_timer(request, elapsed) {
    path = 'None'
    if request {
      path = request.path
    }
    msg = concat('Request(', path, ')\n')
    for (_, (mark, timestamp)) in elapsed {
      msg.extend('\t').extend(str(timestamp/1000)).extend(': ').extend(mark).extend('\n')
    }
    io.println(msg)
  }
  
  def run() {
    if self.num_threads > 1 {
      for _ in range(0, self.num_threads) {
        self.ex.execute(@(server) {
          server.handle_requests()
        }, self)
      }
      while True {
        sync.sleep(100000)
      }      
    } else {
      self.handle_requests()
    }
    self.sock.close()
    net.cleanup()
  }
}