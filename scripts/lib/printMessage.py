def printMessage(message):
  import os, sys
  sys.stdout.write(os.path.basename(sys.argv[0]) + ': ' + message + '\n')
  sys.stdout.flush()
  
