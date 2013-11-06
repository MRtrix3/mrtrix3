def errorMessage(message):
  import os, sys
  sys.stderr.write(os.path.basename(sys.argv[0]) + ' [ERROR]: ' + message + '\n')
  exit(1)
  
