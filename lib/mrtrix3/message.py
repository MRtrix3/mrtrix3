clearLine = ''
colourClear = ''
colourConsole = ''
colourDebug = ''
colourError = ''
colourExec = ''
colourWarn = ''



def console(message):
  import os, sys
  from mrtrix3 import app
  global colourClear, colourPrint
  if app.verbosity:
    sys.stderr.write(os.path.basename(sys.argv[0]) + ': ' + colourConsole + message + colourClear + '\n')



def debug(message):
  import inspect, os, sys
  from mrtrix3 import app
  global colourClear, colourDebug
  if app.verbosity <= 2: return
  stack = inspect.stack()[1]
  try:
    fname = stack.function
  except: # Prior to Version 3.5
    fname = stack[3]
  sys.stderr.write(os.path.basename(sys.argv[0]) + ': ' + colourDebug + '[DEBUG] ' + fname + '(): ' + message + colourClear + '\n')



def error(message):
  import os, sys
  from mrtrix3 import app
  global colourClear, colourError
  sys.stderr.write(os.path.basename(sys.argv[0]) + ': ' + colourError + '[ERROR] ' + message + colourClear + '\n')
  app.cleanup = False
  app.complete()
  sys.exit(1)



def warn(message):
  import os, sys
  global colourClear, colourWarn
  sys.stderr.write(os.path.basename(sys.argv[0]) + ': ' + colourWarn + '[WARNING] ' + message + colourClear + '\n')

