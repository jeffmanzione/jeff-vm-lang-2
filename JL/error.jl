module error

import builtin
import io
import struct

class Token {
  def new (text, line, col, fn, line_text) {
    self.text = text
    self.line = line
    self.col = col
    self.fn = fn
    self.line_text = line_text
  }
  def to_s() {
    concat('Token("', self.text, '":',
           self.line, ',',
           self.col, ',',
           self.fn, ',',
           self.line_text, ')')
  }
}

class StackLine {
  def new (block) {
    self.mod = block.$module
    self.caller = block.$caller
    self.ip = block.$ip
    self.token = Token(builtin.token__(self.mod, self.ip))    
  }
  def to_s() {
    tos = ''
    if self.caller {
      if self.caller is MethodInstance {
        tos.extend(self.caller.method.method_path())
      } else if self.caller is Method {
        tos.extend(self.caller.method_path())
      } else {
        tos.extend(concat(self.mod.name,'.', self.caller.name))
      }
    } else {
      tos.extend(self.mod.name)
    }
    tos.extend(concat('(', self.token.fn, ':', self.token.line, ')'))
  }
}

class Error {
  def new(msg) {
    self.msg = if (msg is String) then msg else concat(msg)
    self.stack_lines = []
    for i=0, i<$root.$saved_blocks.len, i=i+1 {
      block = $root.$saved_blocks[i]
      self.stack_lines.append(StackLine(block))
    }
  }
  def to_s() {
    head = self.stack_lines[0]
    tok = head.token
    tos = concat('Error in ', tok.fn,
                 ' at line ', tok.line, ' col ', tok.col, ': ',
                 self.msg, '\n')
    tos.extend(concat(tok.line, ':', tok.line_text))
    if ~tok.line_text.ends_with('\n') {
      tos.extend('\n')
    }
    for i=0, i<tok.col + 2, i=i+1 {
      tos.extend(' ')
    }
    for i=0, i<tok.text.len, i=i+1 {
      tos.extend('^')
    }
    tos.extend('\n')
    for i=0, i<self.stack_lines.len, i=i+1 {
      sl = self.stack_lines[i]
      tos.extend(sl.to_s()).extend('\n')
    }
    tos
  }
}

def raise_error(e) {
  io.fprintln(io.ERROR, e)
  exit
}