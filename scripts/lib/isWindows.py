def isWindows():
  import platform
  system = platform.system().lower()
  return system.startswith('mingw') or system.startswith('msys') or system.startswith('windows')
  