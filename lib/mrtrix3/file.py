# Collection of convenience functions for manipulating files and directories



# This function can (and should in some instances) be called upon any file / directory
#   that is no longer required by the script. If the script has been instructed to retain
#   all temporaries, the resource will be retained; if not, it will be deleted (in particular
#   to dynamically free up storage space used by the script).
def delTemporary(path):
  import shutil, os
  from mrtrix3 import app
  if not app._cleanup:
    return
  if os.path.isfile(path):
    type = 'file'
    func = os.remove
  elif os.path.isdir(path):
    type = 'directory'
    func = shutil.rmtree
  else:
    app.debug('Unknown target \'' + path + '\'')
    return
  if app._verbosity > 2:
    app.console('Deleting temporary ' + type + ': \'' + path + '\'')
  try:
    func(path)
  except OSError:
    app.debug('Unable to delete temporary ' + type + ': \'' + path + '\'')



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
    app.debug('Directory \'' + path + '\' already exists')



# Wait until a particular file not only exists, but also does not have any
#   other process operating on it (so hopefully whatever created it has
#   finished its work)
# This functionality is achieved in different ways, depending on the capabilities
#   of the system:
#   - On Windows, two processes cannot open the same file in read mode. Therefore,
#     try to open the file in 'rb+' mode, which requests write access but does not
#     create a new file if none exists
#   - If command fuser is available, use it to test if any processes are currently
#     accessing the file (note that since fuser's silent mode is used and a decision
#     is made based on the return code, other processes accessing the file will
#     result in the script pausing regardless of whether or not those processes have
#     write mode access)
#   - If neither of those applies, no additional safety check can be performed.
# Initially, checks for the file once every 1/1000th of a second; this gradually
#   increases if the file still doesn't exist, until the program is only checking
#   for the file once a minute.
def waitFor(path):
  import os, time
  from mrtrix3 import app

  def inUse(path):
    import subprocess
    from distutils.spawn import find_executable
    if app.isWindows():
      if not os.access(path, os.W_OK):
        return None
      try:
        with open(path, 'rb+') as f:
          pass
        return False
      except:
        return True
    if not find_executable('fuser'):
      return None
    # fuser returns zero if there IS at least one process accessing the file
    # A fatal error will result in a non-zero code -> inUse() = False, so waitFor() can return
    return not subprocess.call(['fuser', '-s', path], shell=False, stdin=None, stdout=None, stderr=None)

  if not os.path.exists(path):
    delay = 1.0/1024.0
    app.console('Waiting for creation of new file \"' + path + '\"')
    while not os.path.exists(path):
      time.sleep(delay)
      delay = max(60.0, delay*2.0)
    app.debug('File \"' + path + '\" appears to have been created')
  if not os.path.isfile(path):
    app.debug('Path \"' + path + '\" is not a file; not testing for finalization')
    return
  init_test = inUse(path)
  if init_test is None:
    app.debug('Unable to test for finalization of new file \"' + path + '\"')
    return
  if not init_test:
    app.debug('File \"' + path + '\" immediately ready')
    return
  app.console('Waiting for finalization of new file \"' + path + '\"')
  delay = 1.0/1024.0
  while True:
    if inUse(path):
      time.sleep(delay)
      delay = max(60.0, delay*2.0)
    else:
      app.debug('File \"' + path + '\" appears to have been finalized')
      return
