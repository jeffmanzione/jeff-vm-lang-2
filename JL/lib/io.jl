module io

self.IN = const FileReader('__STDIN__')
self.OUT = const FileWriter('__STDOUT__', True)
self.ERROR = const FileWriter('__STDERR__', True)

class FileInternal {
  field file
  new(fn, rw, a, binary) {
    mode = rw.copy()
    if binary {
      mode.append('b')
    }
    if a {
      mode.append('+')
    }
    file = File__(fn, mode)
    if ~file.success {
      raise Error(concat('Failed to open file. FileInternal(',
                      fn, ',', rw, ',', a, ',', binary,')'))
    }
  }
  method rewind() file.rewind__()
  method gets(n) file.gets__(n)
  method getline() file.getline__()
  method getlines() file.getall__()
  method puts(s) file.puts__(s)
  method close() file.close__()
}

class FileReader {
  field fi
  new(fn, binary=False) {
    fi = const FileInternal(fn, 'r', False, binary)
  }
  method rewind() fi.rewind()
  method gets(n) fi.gets(n)
  method getline() fi.getline()
  method getlines() fi.getlines()
  method getall() fi.getall()
  method close() fi.close()
}

class FileWriter {
  field fi
  new(fn, append=False, binary=False) {
    fi = const FileInternal(fn, 'w', append, binary)
  }
  method rewind() fi.rewind()
  method write(s) fi.puts(s)
  method writeln(s) {
    fi.puts(s)
    fi.puts('\n')
  }
  method close() fi.close()
}

def fprint(f, a) {
  f.write(str(a))
}

def fprintln(f, a) {
  f.writeln(str(a))
}

def print(a) {
  fprint(OUT, a)
}

def println(a) {
  fprintln(OUT, a)
}
