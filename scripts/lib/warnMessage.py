def warnMessage(message):
  import lib.app, os, sys
  sys.stderr.write(os.path.basename(sys.argv[0]) + ': ' + lib.app.colourWarn + '[WARNING] ' + message + lib.app.colourClear + '\n')
  sys.stderr.flush()
  
