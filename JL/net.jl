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

self.LT = '<'
self.GT = '>'
self.LT_ESCAPED = '&lt;'
self.GT_ESCAPED = '&gt;'
self.AMPER = '&'
self.QUESTION = '?'
self.EQUALS = '='

self.HEADER_GENERIC_200_HTML = 'HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=UTF-8\r\n\r\n'
self.HEADER_GENERIC_200_CSS = 'HTTP/1.1 200 OK\r\nContent-Type: text/css; charset=UTF-8\r\n\r\n'
self.HEADER_GENERIC_200_JS = 'HTTP/1.1 200 OK\r\nContent-Type: text/javascript; charset=UTF-8\r\n\r\n'

def init() {
  if self.is_inited {
    return
  }
  self.is_inited = True
  self.init__()
}

def cleanup() {
  if ~self.is_inited {
    return
  }
  self.is_inited = False
  self.cleanup__()
}

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
    def to_s() {
      concat(self.type, ' ', self.path, ' ', self.protocol, '/', self.version)
    }
}

def parse_params(path) {
  query_parts = path.split(QUESTION)
  path = query_parts[0]
  params = struct.Map(31)
  for (_,i) in range(1, query_parts.len) {
    part = query_parts[i]
    param_parts = part.split(AMPER)
    for (_, param) in param_parts {
      kv = param.split(EQUALS)
      if kv.len < 2 {
        params[kv[0]] = '1'
      } else {
        params[kv[0]] = kv[1]
      }
    }
  }
  (path, params)
}

def parse_request(req) {
  try {
    parts = req.split('\r\n')
    if parts.len == 0 {
      raise Error(concat('Invalid request: ', req))
    }
    request = parts[0].trim()
    req_head = request.split(' ')
    type = req_head[0]
    path = req_head[1]
    (path, params) = parse_params(path)
    protocol = req_head[2].split('/')
    
    map = struct.Map(51)
    for i=1, i<parts.len, i=i+1 {
      kv = parts[i].split(':')
      map[kv[0].trim()] = kv[1].trim()
    }
    return HttpRequest(type, path, [], protocol[0], protocol[1], map)
  } catch e {
    io.fprintln(io.ERROR, e)
    return None
  }
}

def html_escape(text) {
  i = 0
  lts = []
  while i < text.len {
    f = text.find(LT, i)
    if ~f {
      break
    }
    i = i + f
    lts.append(i)
    i=i+1
  }
  i = 0
  while i < text.len {
    f = text.find(GT, i)
    if ~f {
      break
    }
    i = i + f
    lts.append(i)
    i=i+1
  }
  
  if lts.len == 0 {
    return text.copy()
  }
  lts.sort(cmp)
  result = ''
  old_i = 0
  for i=0, i<lts.len, i=i+1 {
    c = text[lts[i]]
    result.extend(text, old_i, lts[i])
    old_i = lts[i]+1
    if c == LT[0] then result.extend(LT_ESCAPED)
    else if c == GT[0] then result.extend(GT_ESCAPED)
  }
  result.extend(text, lts[lts.len-1])
  return result
}

class RequestHandler {
  def new() {
    self.handlers = []
  }
  def register(match_fn, handler) {
    self.handlers.append((match_fn, handler))
  }
  def register_else(handler) {
    self.elseHandler = handler
  }
  def handle(request) {
    for _, (match_fn, handler) in self.handlers {
      if match_fn(request) {
        return handler(request)
      }
    }
    if self.elseHandler {
      return self.elseHandler(request)
    }
    return None
  }
}

def read_entire_file(path) {
  io.FileReader(concat('.', path)).getlines()
}

class CachedTextRenderer {
  def new() {
    self.cache = struct.Cache()
  }
  def get(key, src, params) {
    parts_keys = self.cache.get(key, @(src, params) {
      indices = []
      parts = []
      keys = []
      for (k, v) in params {
        k_inds = src.find_all(k)
        for (_, k_ind) in k_inds {
          indices.append((k, k_ind))
        }
      }
      indices.sort(@(t1, t2) cmp(t1[1], t2[1]))
      index = 0
      for (_, (key, i)) in indices {
        parts.append(src.substr(index, i))
        keys.append(key)
        index = i + key.len
      }
      parts.append(src.substr(index))
      return (parts, keys)
    }, (src, params))
    
    text = ''
    parts = parts_keys[0]
    keys = parts_keys[1]
    len = keys.len + parts.len
    for i=0, i<len, i=i+1 {
      if i%2 == 0 {
        text.extend(parts[i / 2])
      } else {
        text.extend(params[keys[i / 2]])
      }
    }
    return text
  }
}