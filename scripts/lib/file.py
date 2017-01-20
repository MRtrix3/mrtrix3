# Collection of convenience functions for manipulating files and directories



# These functions can (and should in some instances) be called upon any file / directory
#   that is no longer required by the script. If the script has been instructed to retain 
#   all temporaries, the resource will be retained; if not, it will be deleted (in particular
#   to dynamically free up storage space used by the script).
def delTempFile(path):
  import os
  import lib.app
  import lib.message
  if not lib.app.cleanup:
    return
  if lib.app.verbosity > 2:
    printMessage('Deleting temporary file: ' + path)
  try:
    os.remove(path)
  except OSError:
    pass

def delTempFolder(path):
  import os, shutil
  import lib.app
  import lib.message
  if not lib.app.cleanup:
    return
  if lib.app.verbosity > 2:
    printMessage('Deleting temporary folder: ' + path)
  try:
    shutil.rmtree(path)
  except OSError:
    pass



# Make a directory if it doesn't exist; don't do anything if it does already exist
def makeDir(path):
  import errno, os
  import lib.message
  try:
    os.makedirs(path)
    lib.message.debug('Created directory ' + path)
  except OSError as exception:
    if exception.errno != errno.EEXIST:
      raise
    lib.message.debug('Directory ' + path + ' already exists')



# Get an appropriate location and name for a new temporary file
# Note: Doesn't actually create a file; just gives a unique name that won't over-write anything
def newTempFile(suffix):
  import os, random, string
  import lib.message
  import lib.mrtrix
  if 'TmpFileDir' in lib.mrtrix.config:
    dir_path = lib.mrtrix.config['TmpFileDir']
  else:
    dir_path = '.'
  if 'TmpFilePrefix' in lib.mrtrix.config:
    prefix = lib.mrtrix.config['TmpFilePrefix']
  else:
    prefix = os.path.basename(sys.argv[0]) + '-tmp-'
  full_path = dir_path
  if not suffix:
    suffix = 'mif'
  while os.path.exists(full_path):
    random_string = ''.join(random.choice(string.ascii_uppercase + string.digits) for x in range(6))
    full_path = os.path.join(dir_path, prefix + random_string + '.' + suffix)
  lib.message.debug(full_path)
  return full_path

