def warnMessage(message):
  import os, sys
  sys.stdout.write(os.path.basename(sys.argv[0]) + ' [WARNING]: ' + message + '\n')
  sys.stdout.flush()
  
