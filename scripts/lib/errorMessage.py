def errorMessage(message):
  import lib.app, os, shutil, sys
  sys.stderr.write(os.path.basename(sys.argv[0]) + ' [ERROR]: ' + message + '\n')
  os.chdir(lib.app.workingDir)
  shutil.rmtree(lib.app.tempDir)
  exit(1)
  
