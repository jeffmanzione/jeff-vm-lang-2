import io
import net
import struct

NUM_THREADS = 1
header = 'HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=UTF-8\r\n\r\n'

net.init()

sock = net.Socket(net.AF_INET, net.SOCK_STREAM, 80, 0, NUM_THREADS)

cache = struct.Cache()

handler = net.RequestHandler()

handler.register(@(request) {request.path.ends_with('.ico')}, 
                 @(request, cache) {
    return cache.get(request.path, @(request) {
      return net.readEntireFile(request.path)
    }, request)
})
handler.register_else(@(request, cache) {
    return cache.get(request.path, @(request) {
      text = net.readEntireFile(request.path)
      if ~request.path.ends_with('.html') {
        result = ''
        result.extend('<pre>')
           .extend(net.html_escape(text))
           .extend('</pre>')
        return result
      } else {
        return text
      }
    }, request)
})

while (True) {
  io.println('Listening...') 
  handle = sock.accept()
  msg = handle.receive()
  handle.send(header)
  request = net.parse_request(msg)
  if request {    
    io.println(concat('Fetching ', request))
    file_lines = handler.handle(request, cache)
    if file_lines {
      handle.send(file_lines)
    }
  } else {
    io.println('Failed')
  }
  handle.close()
  ;io.println(concat('Deleted ', collect_garbage(), ' Objects.'))
}

sock.close()

net.cleanup()