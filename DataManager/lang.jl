module lang

class Function {
  def new(name, parent_module, ip) {
    self.$name = name
    self.$module = parent_module
    self.$ip = ip
  }
  def name() self.$name
  def module() self.$module
  def ip() self.$ip
}

class Field {
  def new(name, parent) {
    self.$name = name
    self.parent = parent
  }
  def name() self.$name
  def parent() self.parent
}

class String : Array {
  def new(str) {
    for i = str.len - 1, i >= 0, i--
      self[i] = str[i]
  }
  def to_s() self
  def copy() String(self)
  static copy(String str) String(str)
  
}