def debugMessage(message):
  import lib.app, inspect, os, sys, types
  if lib.app.verbosity <= 2: return
  stack = inspect.stack()[1]
  try:
    fname = stack.function
  except: # Prior to Version 3.5
    fname = stack[3]
  sys.stderr.write(os.path.basename(sys.argv[0]) + ': ' + lib.app.colourDebug + '[DEBUG] ' + fname + '(): ' + message + lib.app.colourClear + '\n')

