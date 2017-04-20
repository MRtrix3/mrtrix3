# Collection of convenience functions for manipulating files and directories



# These functions can (and should in some instances) be called upon any file / directory
#   that is no longer required by the script. If the script has been instructed to retain
#   all temporaries, the resource will be retained; if not, it will be deleted (in particular
#   to dynamically free up storage space used by the script).
def delTempFile(path):
  import os
  from mrtrix3 import app
  if not app._cleanup:
    return
  if app._verbosity > 2:
    app.console('Deleting temporary file: ' + path)
  try:
    os.remove(path)
  except OSError:
    app.debug('Unable to delete temporary file ' + path)

def delTempFolder(path):
  import shutil
  from mrtrix3 import app
  if not app._cleanup:
    return
  if app._verbosity > 2:
    app.console('Deleting temporary folder: ' + path)
  try:
    shutil.rmtree(path)
  except OSError:
    app.debug('Unable to delete temprary folder ' + path)



# Make a directory if it doesn't exist; don't do anything if it does already exist
def makeDir(path):
  import errno, os
  from mrtrix3 import app
  try:
    os.makedirs(path)
    app.debug('Created directory ' + path)
  except OSError as exception:
    if exception.errno != errno.EEXIST:
      raise
    app.debug('Directory ' + path + ' already exists')



# Get an appropriate location and name for a new temporary file
# Note: Doesn't actually create a file; just gives a unique name that won't over-write anything
def newTempFile(suffix):
  import os, random, string, sys
  from mrtrix3 import app
  if 'TmpFileDir' in app.config:
    dir_path = app.config['TmpFileDir']
  else:
    dir_path = app._tempDir
  if 'TmpFilePrefix' in app.config:
    prefix = app.config['TmpFilePrefix']
  else:
    prefix = 'mrtrix-tmp-'
  full_path = dir_path
  if not suffix:
    suffix = 'mif'
  while os.path.exists(full_path):
    random_string = ''.join(random.choice(string.ascii_uppercase + string.digits) for x in range(6))
    full_path = os.path.join(dir_path, prefix + random_string + '.' + suffix)
  app.debug(full_path)
  return full_path



# Wait until a particular file not only exists, but also does not have any
#   other process operating on it (so hopefully whatever created it has
#   finished its work)
# Using open() rather than os.path.exists() ensures that not only has the file
#   appeared in the directory listing, but the data are there and a file handle
#   can be obtained. Additionally, an exclusive file lock is obtained.
# 'r+' mode is used since it includes write access (which may fail if some other
#   process also has write access to that file), but will _not_ create a new file
#   if the file is somehow deleted between os.path.exists() and open().
# Initially, checks for the file once every 1/1000th of a second; this gradually
#   increases if the file still doesn't exist, until the program is only checking
#   for the file once a minute.
def waitFor(path):
  import os, time
  
  try:
    import fcntl
    do_fcntl = True
  except ImportError:
    do_fcntl = False
  from mrtrix3 import app
  if not os.path.exists(path):
    delay = 1.0/1024.0
    app.console('Waiting for creation of new file \"' + path + '\"')
    while not os.path.exists(path):
      time.sleep(delay)
      delay = max(60.0, delay*2.0)
    app.debug('File \"' + path + '\" appears to have been created')
  if not os.access(path, os.W_OK):
    app.warn('User does not have write access to file \"' + path + '\"; unable to verify file is complete')
    return
  try:
    with open(path, 'rb+') as f:
      if do_fcntl:
        fcntl.lockf(f, fcntl.LOCK_EX)
        fcntl.lockf(f, fcntl.LOCK_UN)
    app.debug('File \"' + path + '\" immediately ready')
    return
  except:
    pass
  app.console('Waiting for finalization of new file \"' + path + '\"')
  delay = 1.0/1024.0
  while True:
    try:
      with open(path, 'rb+') as f:
        if do_fcntl:
          fcntl.lockf(f, fcntl.LOCK_EX)
          fcntl.lockf(f, fcntl.LOCK_UN)
      app.debug('File \"' + path + '\" appears to have been finalized')
      return
    except (IOError, OSError):
      time.sleep(delay)
      delay = max(60.0, delay*2.0)

