_value = None
def isWindows():
  import platform
  from lib.debugMessage import debugMessage
  global _value
  if _value is not None:
    return _value
  system = platform.system().lower()
  _value = system.startswith('mingw') or system.startswith('msys') or system.startswith('windows')
  debugMessage('System = ' + system + ' is Windows? ' + str(_value))
  return _value
  