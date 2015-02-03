def printMessage(message):
  import app, os, sys
  if app.verbosity:
    sys.stdout.write(os.path.basename(sys.argv[0]) + ': ' + message + '\n')
    sys.stdout.flush()
  
