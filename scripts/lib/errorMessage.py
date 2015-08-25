def errorMessage(message):
  import lib.app, os, shutil, sys
  sys.stderr.write(os.path.basename(sys.argv[0]) + ': ' + lib.app.colourError + '[ERROR] ' + message + lib.app.colourClear + '\n')
  lib.app.complete()
  exit(1)
  
