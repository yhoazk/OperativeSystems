# test construction of bytearray from different objects

from array import array

# bytes, tuple, list
print(bytearray(b'123'))
print(bytearray((1, 2)))
print(bytearray([1, 2]))

# arrays
print(bytearray(array('b', [1, 2])))
print(bytearray(array('h', [1, 2])))
print(bytearray(array('I', [1, 2])))
print(bytearray(array('f', [1, 2.3])))
