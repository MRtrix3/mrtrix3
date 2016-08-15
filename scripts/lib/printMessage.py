def printMessage(message):
  import lib.app, os, sys
  if lib.app.verbosity:
    sys.stderr.write(os.path.basename(sys.argv[0]) + ': ' + lib.app.colourPrint + message + lib.app.colourClear + '\n')
  
