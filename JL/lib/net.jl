module net

import error
import io
import struct
import sync
import time

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
self.WHITE_SPACE = ' '
self.F_SLASH = '/'
self.COLON = ':'
self.RETURN_NEWLINE = '\r\n'

self.COOKIE_KEY = 'Cookie'
self.COOKIE_SEPARATOR = '; '

self.NOT_FOUND = 'Could not find file.'
self.HEADER_GENERIC_200_HTML = 'HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=UTF-8\r\n\r\n'
self.HEADER_GENERIC_200_CSS = 'HTTP/1.1 200 OK\r\nContent-Type: text/css; charset=UTF-8\r\n\r\n'
self.HEADER_GENERIC_200_JS = 'HTTP/1.1 200 OK\r\nContent-Type: text/javascript; charset=UTF-8\r\n\r\n'
self.HEADER_GENERIC_500 = 'HTTP/1.1 500 Internal Server Error\r\n\r\n'
self.HEADER_GENERIC_501 = 'HTTP/1.1 501 Not Implemented\r\n\r\n'

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
  new(field protocol,
      field version,
      field status_code,
      field status,
      field content_type,
      field charset) {}
  method to_s() {
    concat(protocol, '/', version, ' ',
           status_code, ' ', status,
           '\r\nContent-Type: ', content_type,
           '; charset=', charset, '\r\n\r\n')
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
    new(field type,
        field path,
        field params,
        field protocol,
        field version,
        field map) {}
    method to_s() {
      ret = concat(type, WHITE_SPACE, path)
      if params and (params.keys.len > 0) {
        ret.extend(QUESTION)
        param_arr = []
        for (k, v) in params {
          param_arr.append(concat(k, EQUALS, v))
        }
        ret.extend(AMPER.join(param_arr))
      }
      ret.extend(WHITE_SPACE).extend(protocol).extend(F_SLASH)
          .extend(version).extend('\n')
      if map {
        for (k,v) in map {
          ret.extend('  ').extend(k).extend(': ').extend(str(v))
              .extend('\n')
        }
      }
      return ret
    }
}

def redirect(request, target) {
  response = 'HTTP/1.1 301 Internal Redirect\r\nLocation: '
  if target.ends_with('/') and request.path.starts_with('/') {
    response.extend(target).extend(request.path.substr(1))
  } else if target.ends_with('/') or request.path.starts_with('/') or request.path.len == 0 or target.len == 0 {
    response.extend(target).extend(request.path)
  } else {
    response.extend(target).extend('/').extend(request.path)
  }
  if request.params and (request.params.keys.len > 0) {
    response.extend(QUESTION)
    param_arr = []
    for (k, v) in request.params {
      param_arr.append(concat(k, EQUALS, v))
    }
    response.extend(AMPER.join(param_arr))
  }
  response.extend('\r\nNon-Authoritative-Reason: HSTS\r\n\r\n')
  return response
}

def parse_params(path) {
  q_index = path.find(QUESTION)
  if ~q_index {
    return (path, {})
  }
  (
    path.substr(0, q_index),
    path.substr(q_index + 1)
      .split(AMPER)
      .collect(
        (a, part) {
          eq_index = part.find(EQUALS)
          if ~eq_index {
            a[part] ='1'
          }
          a[part.substr(0, eq_index)] = part.substr(eq_index + 1)
          return a
        },
        {}))
}

def parse_request(req) {
  try {
    parts = req.split(RETURN_NEWLINE)
    if parts.len == 0 {
      raise Error(concat('Invalid request: ', req))
    }
    request = parts[0].trim()
    req_head = request.split(WHITE_SPACE)
    type = req_head[0]
    path = req_head[1]
    (path, params) = parse_params(path)
    protocol = req_head[2].split(F_SLASH)
    
    map = {}
    for i=1, i<parts.len, i=i+1 {
      kv = parts[i].split(COLON)
      key = kv[0].trim()
      map[kv[0].trim()] = kv[1].trim()
    }
    return HttpRequest(type, path, params, protocol[0], protocol[1], map)
  } catch e {
    io.fprintln(io.ERROR, e)
    return None
  }
}

class Cookies {
  new(field map) {}
  method get(key) {
    return map[key]
  }
  method to_s() {
    res = '{'
    map.each((k,v) -> res.extend('\"').extend(str(k)).extend('\":\"').extend(str(v)).extend('\" '))
    res.extend('}')
    return res
  }
}

def parse_cookie(cookie) {
  res = {}
  kvs = cookie.split(COOKIE_SEPARATOR)
  for i=0, i<kvs.len, i=i+1 {
    kv = kvs[i].split(EQUALS)
    res[kv[0]]= kv[1]
  }
  return Cookies(res)
}

def html_escape(text) {
  lts = text.find_all(LT)
  gts = text.find_all(GT)  
  if (lts.len == 0)  and (gts.len == 0){
    return text.copy()
  }
  lts.extend(gts)
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
  if lts[lts.len-1] < text.len - 1 {
    result.extend(text, lts[lts.len-1] + 1)
  }
  return result
}

class RequestHandler {
  field handlers, elseHandler
  new() {
    handlers = []
  }
  method register(match_fn, handler) {
    handlers.append((match_fn, handler))
  }
  method register_else(handler) {
    elseHandler = handler
  }
  method handle(request) {
    for i=0, i < handlers.len, i=i+1 {
      (match_fn, handler) = handlers[i]
      if match_fn(request) {
        res = handler(request)
        return res
      }
    }
    if elseHandler {
      res = elseHandler(request)
      return res
    }
    return None
  }
}

def read_entire_file(path) {
  f = io.FileReader(concat('.', path))
  text = f.getlines()
  f.close()
  return text
}

class CachedTextRenderer {
  field cache
  new() {
    cache = struct.Cache()
  }
  method write(key, src, params, sink) {
    parts_keys = cache.get(key, (_) {
      indices = []
      parts = []
      keys = []
      for i=0, i < params.keys.len, i=i+1 {
        k_inds = src.find_all(params.keys[i])
        k_inds_len = k_inds.len
        for j=0, j < k_inds_len, j=j+1 {
          indices.append((params.keys[i], k_inds[j]))
        }
      }
      indices.sort((t1, t2) -> cmp(t1[1], t2[1]))
      index = 0
      for (_, (key, i)) in indices {
        parts.append(src.substr(index, i))
        keys.append(key)
        index = i + key.len
      }
      parts.append(src.substr(index))
      return (parts, keys)
    })
    if ~parts_keys or (parts_keys is error.Error) {
      sink(NOT_FOUND)
    } else {
      (parts, keys) = parts_keys
      len = keys.len + parts.len
      if len == 1 {
        sink(parts[0])
        return
      }
      for i=0, i<len, i=i+1 {
        if i%2 == 0 {
          sink(parts[i / 2])
        } else {
          sink(params[keys[i / 2]])
        }
      }
    }
  }
}

class Server {
  field ex
  new(field applications={}, field pool_size) {
    ex = sync.ThreadPool(pool_size)
  }
  
  method start() {
    for (port, app) in applications {
      try {
        sock = app.create_socket(port)
        ex.execute(
          (app) {
            while True {
              handle = None
              handle = sock.accept()
              ex.execute((handle) {
                try {
                  req = parse_request(handle.receive())
                  app.process(req, handle.send)
                } catch e {
                  if handle {
                    handle.send([HEADER_GENERIC_500, 'Sorry.\n'])
                  }
                  io.fprintln(io.ERROR, e)
                }
                if handle {
                  handle.close()
                }
              }, handle)
            }
          }, app)
      } catch e {
        io.fprintln(io.ERROR, e)
      }
    }
    sync.sleep(sync.INFINITE)
  }
}

class Application {
  method create_socket(port) {
    raise Error('Not Implemented.')
  }
  method process(req, sink) {
    sink([HEADER_GENERIC_501, 'Application not implemented.\n'])
  }
}

class HttpApplication : Application {
  new(field respond) {}
  method create_socket(port) {
    Socket(AF_INET, SOCK_STREAM, port, 0, 4)
  }
  method process(req, sink) {
    sink(_get_header(req))
    respond(req, sink)
  }
  method _get_header(req) {
    if req.path.ends_with('.css') {
      return HEADER_GENERIC_200_CSS
    }
    if req.path.ends_with('.js') {
      return HEADER_GENERIC_200_JS
    }
    return HEADER_GENERIC_200_HTML
  }
}

class HttpsWrapperApplication : Application {
  new(field http_application,
      field cert_fn,
      field private_key_fn) {}
  method create_socket(port) {
    SSLSocket(
      http_application.create_socket(port),
      cert_fn,
      private_key_fn)
  }
  method process(req, sink) {
    http_application.process(req, sink)
  }
}

self.ANY = None

class HttpRedirect : HttpApplication {
  new(field redirect_to_addr) {
    self.HttpApplication.new(()->None)
  }
  method process(req, sink) {
    sink(redirect(req, redirect_to_addr))
  }
}

def wrap_in_ssl(application, cert_fn, private_key_fn) {
  HttpsWrapperApplication(application, cert_fn, private_key_fn)
}

class ShardedHttpApplication : HttpApplication {
  new(field shards) {
    directory = 
        $module.HttpApplication(
            (req, sink) {
              response = '<table>'
              for (path, app) in shards {
                response.extend('<tr><td><a href=\"').extend(path).extend('\">').extend(path).extend('</a></td><td>')
                response.extend(str(app)).extend('</td></tr>')
              }
              response.extend('</table>')
              sink(response)
            })
    shards['/index'] = directory
    if '/' notin shards {
      shards['/'] = directory
    }
    ; Sort from longest to shortest.
    shards.keys.inssort((x,y) -> y.len - x.len)
  }
  method process(req, sink) {
    for (_,shard_key) in shards.keys {
      if req.path.starts_with(shard_key) {
        shards[shard_key].process(_rebase(shard_key, req), sink)
        return
      }
    }
    sink([HEADER_GENERIC_500, 'No such shard.\n'])
  }
  method _rebase(shard_key, req) {
    req.path = req.path.substr(shard_key.len)
    return req
  }
}