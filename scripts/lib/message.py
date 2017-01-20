clearLine = ''
colourClear = ''
colourConsole = ''
colourDebug = ''
colourError = ''
colourExec = ''
colourWarn = ''



def console(message):
  import lib.app, os, sys
  global colourClear, colourPrint
  if lib.app.verbosity:
    sys.stderr.write(os.path.basename(sys.argv[0]) + ': ' + colourConsole + message + colourClear + '\n')



def debug(message):
  import lib.app, inspect, os, sys
  global colourClear, colourDebug
  if lib.app.verbosity <= 2: return
  stack = inspect.stack()[1]
  try:
    fname = stack.function
  except: # Prior to Version 3.5
    fname = stack[3]
  sys.stderr.write(os.path.basename(sys.argv[0]) + ': ' + colourDebug + '[DEBUG] ' + fname + '(): ' + message + colourClear + '\n')



def error(message):
  import lib.app, os, sys
  global colourClear, colourError
  sys.stderr.write(os.path.basename(sys.argv[0]) + ': ' + colourError + '[ERROR] ' + message + colourClear + '\n')
  lib.app.cleanup = False
  lib.app.complete()
  sys.exit(1)



def warn(message):
  import lib.app, os, sys
  global colourClear, colourWarn
  sys.stderr.write(os.path.basename(sys.argv[0]) + ': ' + colourWarn + '[WARNING] ' + message + colourClear + '\n')

