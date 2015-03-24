def errorMessage(message):
  import app, os, shutil, sys
  sys.stderr.write(os.path.basename(sys.argv[0]) + ' [ERROR]: ' + message + '\n')
  os.chdir(app.workingDir)
  shutil.rmtree(app.tempDir)
  exit(1)
  
