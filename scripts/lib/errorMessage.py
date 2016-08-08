def errorMessage(message):
  import lib.app, os, sys
  sys.stderr.write(os.path.basename(sys.argv[0]) + ': ' + lib.app.colourError + '[ERROR] ' + message + lib.app.colourClear + '\n')
  if lib.app.cleanup and lib.app.keepOnError:
    lib.app.cleanup = False
  lib.app.complete()
  exit(1)

