def printMessage(message):
  import lib.app, os, sys
  if lib.app.verbosity:
    sys.stdout.write(os.path.basename(sys.argv[0]) + ': ' + message + '\n')
    sys.stdout.flush()
  
