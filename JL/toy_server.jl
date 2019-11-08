module toy_server

import io
import net
import struct
import sync


self.NUM_THREADS = 1

class ToyServer {

  def new() {
    net.init()
    self.header = net.HEADER_GENERIC_200
    self.sock = net.Socket(net.AF_INET, net.SOCK_STREAM, 80, 0, NUM_THREADS)
    self.cache = struct.Cache()

    self.handler = net.RequestHandler()

    self.handler.register(@(request) { request.path == '/' },
                           @(request) {
      self.cache.get(request.path, @(request) {
        return net.read_entire_file('/index.html')
      }, request)
    })
    self.handler.register(@(request) {request.path.ends_with('.ico')}, 
                          @(request) {
      self.cache.get(request.path, @(request) {
        return net.read_entire_file(request.path)
      }, request)
    })
    self.handler.register_else(@(request) {
      self.cache.get(request.path, @(request) {
        text = net.read_entire_file(request.path)
        if ~request.path.ends_with('.html') {
          return ''.extend('<pre>')
             .extend(net.html_escape(text))
             .extend('</pre>')
        } else {
          return text
        }
      }, request)
    })
  }
  
  def run() {
    while (True) {
      io.println('Listening...') 
      handle = self.sock.accept()
      msg = handle.receive()
      
      handle.send(self.header)
      request = net.parse_request(msg)
      if request {    
        io.println(concat('Fetching ', request))
        file_lines = self.handler.handle(request)
        if file_lines {
          handle.send(file_lines)
        }
      } else {
        io.println('Failed')
      }
      handle.close()
      io.println(concat('Deleted ', collect_garbage(), ' Objects.'))
    }
    self.sock.close()
    net.cleanup()
  }
}