module error

import builtin
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
    concat('Token("', self.text, '":', self.line, ',', self.col, ',', self.fn, ',', self.line_text, ')')
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
    tos = self.mod.name.copy()
    if self.caller {
      if self.caller is Method {
        tos.extend('.').extend(self.caller.parent_class.name)
      }
      tos.extend('.').extend(self.caller.name)
    }
    tos.extend(concat('(', self.token.fn,':', self.ip, ')'))
  }
}

class Error {
  def new(msg) {
  self.msg = msg
    self.stack_lines = []
    for i=0, i<$root.$saved_blocks.len, i=i+1 {
      block = $root.$saved_blocks[i]
      self.stack_lines.append(StackLine(block))
    }
  }
  def to_s() {
    head = self.stack_lines[0]
    tok = head.token
    tos = concat('Error in ', tok.fn, ' at line ', tok.line, ' col ', tok.col, ':\n')
    tos.extend(concat(tok.line, ':', tok.line_text))
    if ~tok.line_text.ends_with('\n') {
      tos.extend('\n')
    }
    for i=0, i<self.stack_lines.len, i=i+1 {
      sl = self.stack_lines[i]
      tos.extend(sl.to_s()).extend('\n')

    }
    tos
  }
}