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
# This functionality is achieved in different ways, depending on the capabilities
#   of the system:
#   - On Windows, two processes cannot open the same file in read mode. Therefore,
#     try to open the file in 'rb+' mode, which requests write access but does not
#     create a new file if none exists
#   - If command fuser is available, parse its output and look for any processes
#     that have write access to the file.
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
    # Apparently the 'F' for open with write access only appears in the verbose output
    proc = subprocess.Popen(['fuser', '-v', path], shell=False, stdin=None, stdout=subprocess.PIPE, stderr=subprocess.PIPE, close_fds=True)
    (stdout, stderr) = proc.communicate()
    # fuser writes process IDs to stdout, and all other info to stderr - even when tabulated together in the verbose output
    # Need to first remove the leading printout, which is the path being queried
    stderr = stderr.decode('utf-8').lstrip(':')
    # Now look for a line where a 'F' appears in the ACCESS field
    for line in stderr.splitlines():
      line = line.split()
      if len(line) == 3 and 'F' in line[1]:
        return True
    return False

  if not os.path.exists(path):
    delay = 1.0/1024.0
    app.console('Waiting for creation of new file \"' + path + '\"')
    while not os.path.exists(path):
      time.sleep(delay)
      delay = max(60.0, delay*2.0)
    app.debug('File \"' + path + '\" appears to have been created')
  init_test = inUse(path)
  if init_test is None:
    app.warn('Unable to test for finalization of new file \"' + path + '\"')
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

