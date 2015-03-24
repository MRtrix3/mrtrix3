# Now have separate -quiet and -verbose options: 
# * -quiet prints nothing until script completes (unless warnings occur)
# * Default is to display command info, but no progress bars etc. (i.e. MRtrix still suppressed with -quiet)
# * -verbose prints all commands and all command outputs

cleanup = True
mrtrixQuiet = '-quiet'
numArgs = 0
tempDir = ''
verbosity = 1
workingDir = ''

def initialise(n):
  import os, random, string, sys
  from printMessage          import printMessage
  from readMRtrixConfSetting import readMRtrixConfSetting
  global cleanup, mrtrixQuiet, numArgs, tempDir, verbosity, workingDir
  #if not numArgs:
  #  sys.stderr.write('Must set numArgs value before calling initialise()\n')
  #  exit(1)
  numArgs = n
  workingDir = os.getcwd()
  for option in sys.argv[numArgs+1:]:
    if '-verbose'.startswith(option):
      verbosity = 2
      mrtrixQuiet = ''
    elif '-quiet'.startswith(option):
      verbosity = 0
      mrtrixQuiet = '-quiet'
    elif '-nocleanup'.startswith(option):
      cleanup = False
    else:
      sys.stderr.write('Unknown option: ' + option + '\n')
      exit(1)
  # Create the temporary directory
  dir_path = readMRtrixConfSetting('TmpFileDir')
  if not dir_path:
    dir_path = '.'
  prefix = readMRtrixConfSetting('TmpFilePrefix')
  if not prefix:
    prefix = os.path.basename(sys.argv[0]) + '-tmp-'
  tempDir = dir_path
  while os.path.isdir(tempDir):
    random_string = ''.join(random.choice(string.ascii_uppercase + string.digits) for x in range(6))
    tempDir = os.path.join(dir_path, prefix + random_string) + os.sep
  os.makedirs(tempDir)
  printMessage('Generated temporary directory: ' + tempDir)



def gotoTempDir():
  import os
  from printMessage import printMessage
  if verbosity:
    printMessage('Changing to temporary directory (' + tempDir + ')')
  os.chdir(tempDir)
  
  

def moveFileToDest(local_path, destination):
  import os, shutil
  from printMessage import printMessage
  if destination[0] != '/':
    destination = os.path.abspath(os.path.join(workingDir, destination))
  printMessage('Moving output file from temporary directory to user specified location')
  shutil.move(local_path, destination)
  


def terminate():
  import os, shutil
  from printMessage import printMessage
  printMessage('Changing back to original directory (' + workingDir + ')')
  os.chdir(workingDir)
  if cleanup:
    printMessage('Deleting temporary directory ' + tempDir)
    shutil.rmtree(tempDir)
  else:
    printMessage('Contents of temporary directory kept, location: ' + tempDir)

