module net

import io
import struct

self.SOCKET_ERROR = -1
self.AF_INET = 2
self.INADDR_ANY = 0
self.SOCK_STREAM = 1

self.HTTP = 'HTTP'
self.OK = 'OK'
self.TEXT_HTML = 'text/html'
self.UTF_8 = 'UTF-8'

class Header {
  def new(protocol, version, status_code, status, content_type, charset) {
    self.protocol = protocol
    self.version = version
    self.status_code = status_code
    self.status = status
    self.content_type = content_type
    self.charset = charset
  }
  def to_s() {
    concat(self.protocol, '/', self.version, ' ',
           self.status_code, ' ', self.status,
           '\r\nContent-Type: ', self.content_type,
           '; charset=', self.charset, '\r\n\r\n')
  }
}

def parse_header(header_text) {
  i = header_text.find('/')
  protocol = header_text.substr(0, i)
  i2 = i + 1 + header_text.find(' ', i+1)
  version = header_text.substr(i+1, i2)
  i3 = i2 + 1 + header_text.find(' ', i2+1)
  status_code = header_text.substr(i2+1, i3)
  i4 = i3 + 1 + header_text.find('\r\n', i3+1)
  status = header_text.substr(i3+1, i4)
  i5 = i4 + 1 + header_text.find(' ', i4+1)
  i6 = i5 + 1 + header_text.find(';', i5+1)
  content_type = header_text.substr(i5+1, i6)
  i7 = i6 + 1 + header_text.find('=', i6+1)
  i8 = i7 + 1 + header_text.find('\r\n', i7+1)
  charset = header_text.substr(i7+1, i8)
  return Header(protocol, version, status_code, status, content_type, charset)
}


class HttpRequest {
    def new(type, path, params, protocol, version, map) {
      self.type = type
      self.path = path
      self.params = params
      self.protocol = protocol
      self.version = version
      self.map = map
    }
}

def parse_request(req) {
  io.println(req)
  parts = req.split('\r\n')
  request = parts[0].trim()
  type = request.substr(0, request.find(' ', 0))
  map = struct.Map(51)
  for i=1, i<parts.len, i=i+1 {
    kv = parts[i].split(':')
    map[kv[0].trim()] = kv[1].trim()
  }
  io.println(type)
  io.println(map)
}