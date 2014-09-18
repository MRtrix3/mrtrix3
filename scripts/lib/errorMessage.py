def errorMessage(message):
  import os, sys
  sys.stderr.write(os.path.basename(sys.argv[0]) + ' [ERROR]: ' + message + '\n')
  exit(1)
  
def errorMessage(message, temp_dir, working_dir):
  import os, shutil, sys
  sys.stderr.write(os.path.basename(sys.argv[0]) + ' [ERROR]: ' + message + '\n')
  os.chdir(working_dir)
  shutil.rmtree(temp_dir)
  exit(1)

