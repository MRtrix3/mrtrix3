def warnMessage(message):
  import lib.app, os, sys
  sys.stdout.write(os.path.basename(sys.argv[0]) + ': ' + lib.app.colourWarn + '[WARNING] ' + message + lib.app.colourClear + '\n')
  sys.stdout.flush()
  
