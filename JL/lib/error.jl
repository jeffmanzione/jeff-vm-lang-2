module error

import builtin
import io
import struct

class Token {
  new(field text,
      field line,
      field col,
      field fn,
      field line_text) {}
  method to_s() {
    concat('Token("', text, '":',
           line, ',',
           col, ',',
           fn, ',',
           line_text, ')')
  }
}

class StackLine {
  field mod, caller, ip, token
  new(block) {
    mod = block.$module
    caller = block.$caller
    ip = block.$ip
    token = Token(builtin.token__(mod, ip))    
  }
  method to_s() {
    tos = ''
    if caller {
      if caller is MethodInstance {
        tos.extend(caller.$method.method_path())
      } else if caller is Method {
        tos.extend(caller.method_path())
      } else {
        tos.extend(concat(mod.name,'.', caller.name))
      }
    } else {
      tos.extend('\t').extend(mod.name)
    }
    tos.extend(concat('(', token.fn, ':', token.line, ')'))
  }
}

class Error {
  field stack_lines
  new(msg) {
    self.msg = if (msg is String) then msg else if (~msg) 'None' else concat(msg)
    stack_lines = []
    ; Start at 1 since 0-th block will be this loop.
    for i=1, i<$thread.$saved_blocks.len, i=i+1 {
      block = $thread.$saved_blocks[i]
      stack_lines.append(StackLine(block))
    }
  }
  method to_s() {
    head = stack_lines[0]
    tok = head.token
    tos = concat('Error in ', tok.fn,
                 ' at line ', tok.line, ' col ', tok.col, ': ',
                 self.msg, '\n')
    tos.extend(concat(tok.line, ':', tok.line_text))
    if tok.line_text and ~str(tok.line_text).ends_with('\n') {
      tos.extend('\n')
    }
    for i=0, i<tok.col + 2, i=i+1 {
      tos.extend(' ')
    }
    for i=0, i<tok.text.len, i=i+1 {
      tos.extend('^')
    }
    tos.extend('\n')
    for i=0, i<stack_lines.len, i=i+1 {
      sl = stack_lines[i]
      tos.extend(sl.to_s()).extend('\n')
    }
    tos
  }
}

def display_error(e) {
  io.fprintln(io.ERROR, e)
}

def raise_error(e) {
  io.fprintln(io.ERROR, e)
  exit
}