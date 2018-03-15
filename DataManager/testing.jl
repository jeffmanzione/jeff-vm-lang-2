module testing

def test(test_funcs) {
  test_funcs.map(@(e) e())
}