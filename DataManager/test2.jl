module animals;

class Animal {
  def new() {}
  def speak() println('Noise!')
}

enum Breed {
  DASCHUND, LAB, PUG
}

class Dog : Animal {
  field breed
  def new(Breed breed) {
    self.breed = breed
  }
  def speak() 'Woof!'
}