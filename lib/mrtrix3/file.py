# Collection of convenience functions for manipulating files and directories



# These functions can (and should in some instances) be called upon any file / directory
#   that is no longer required by the script. If the script has been instructed to retain 
#   all temporaries, the resource will be retained; if not, it will be deleted (in particular
#   to dynamically free up storage space used by the script).
def delTempFile(path):
  import os
  from mrtrix3 import app, message
  if not app.cleanup:
    return
  if app.verbosity > 2:
    printMessage('Deleting temporary file: ' + path)
  try:
    os.remove(path)
  except OSError:
    pass

def delTempFolder(path):
  import os, shutil
  from mrtrix3 import app, message
  if not app.cleanup:
    return
  if app.verbosity > 2:
    printMessage('Deleting temporary folder: ' + path)
  try:
    shutil.rmtree(path)
  except OSError:
    pass



# Make a directory if it doesn't exist; don't do anything if it does already exist
def makeDir(path):
  import errno, os
  from mrtrix3 import message
  try:
    os.makedirs(path)
    message.debug('Created directory ' + path)
  except OSError as exception:
    if exception.errno != errno.EEXIST:
      raise
    message.debug('Directory ' + path + ' already exists')



# Get an appropriate location and name for a new temporary file
# Note: Doesn't actually create a file; just gives a unique name that won't over-write anything
def newTempFile(suffix):
  import os, random, string
  from mrtrix3 import message, mrtrix
  if 'TmpFileDir' in mrtrix.config:
    dir_path = mrtrix.config['TmpFileDir']
  else:
    dir_path = '.'
  if 'TmpFilePrefix' in mrtrix.config:
    prefix = mrtrix.config['TmpFilePrefix']
  else:
    prefix = os.path.basename(sys.argv[0]) + '-tmp-'
  full_path = dir_path
  if not suffix:
    suffix = 'mif'
  while os.path.exists(full_path):
    random_string = ''.join(random.choice(string.ascii_uppercase + string.digits) for x in range(6))
    full_path = os.path.join(dir_path, prefix + random_string + '.' + suffix)
  message.debug(full_path)
  return full_path

