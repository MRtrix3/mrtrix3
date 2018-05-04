# Collection of convenience functions for manipulating files and directories



# This function can (and should in some instances) be called upon any file / directory
#   that is no longer required by the script. If the script has been instructed to retain
#   all temporaries, the resource will be retained; if not, it will be deleted (in particular
#   to dynamically free up storage space used by the script).
def delTemporary(path): #pylint: disable=unused-variable
  import shutil, os
  from mrtrix3 import app
  if not app.cleanup:
    return
  if os.path.isfile(path):
    temporary_type = 'file'
    func = os.remove
  elif os.path.isdir(path):
    temporary_type = 'directory'
    func = shutil.rmtree
  else:
    app.debug('Unknown target \'' + path + '\'')
    return
  if app.verbosity > 2:
    app.console('Deleting temporary ' + temporary_type + ': \'' + path + '\'')
  try:
    func(path)
  except OSError:
    app.debug('Unable to delete temporary ' + temporary_type + ': \'' + path + '\'')



# Make a directory if it doesn't exist; don't do anything if it does already exist
def makeDir(path): #pylint: disable=unused-variable
  import errno, os
  from mrtrix3 import app
  try:
    os.makedirs(path)
    app.debug('Created directory ' + path)
  except OSError as exception:
    if exception.errno != errno.EEXIST:
      raise
    app.debug('Directory \'' + path + '\' already exists')



# Get an appropriate location and name for a new temporary file
# Note: Doesn't actually create a file; just gives a unique name that won't over-write anything
def newTempFile(suffix): #pylint: disable=unused-variable
  import os.path, random, string
  from mrtrix3 import app
  if 'TmpFileDir' in app.config:
    dir_path = app.config['TmpFileDir']
  elif app.tempDir:
    dir_path = app.tempDir
  else:
    dir_path = os.getcwd()
  app.debug(dir_path)
  if 'TmpFilePrefix' in app.config:
    prefix = app.config['TmpFilePrefix']
  else:
    prefix = 'mrtrix-tmp-'
  app.debug(prefix)
  full_path = dir_path
  suffix = suffix.lstrip('.')
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
#   - If command fuser is available, use it to test if any processes are currently
#     accessing the file (note that since fuser's silent mode is used and a decision
#     is made based on the return code, other processes accessing the file will
#     result in the script pausing regardless of whether or not those processes have
#     write mode access)
#   - If neither of those applies, no additional safety check can be performed.
# Initially, checks for the file once every 1/1000th of a second; this gradually
#   increases if the file still doesn't exist, until the program is only checking
#   for the file once a minute.
def waitFor(paths): #pylint: disable=unused-variable
  import os, time
  from mrtrix3 import app

  def inUse(path):
    import subprocess
    from distutils.spawn import find_executable
    if not os.path.isfile(path):
      return None
    if app.isWindows():
      if not os.access(path, os.W_OK):
        return None
      try:
        with open(path, 'rb+') as dummy_f:
          pass
        return False
      except:
        return True
    if not find_executable('fuser'):
      return None
    # fuser returns zero if there IS at least one process accessing the file
    # A fatal error will result in a non-zero code -> inUse() = False, so waitFor() can return
    return not subprocess.call(['fuser', '-s', path], shell=False, stdin=None, stdout=None, stderr=None)

  def numExist(data):
    count = 0
    for entry in data:
      if os.path.exists(entry):
        count += 1
    return count

  def numInUse(data):
    count = 0
    valid_count = 0
    for entry in data:
      result = inUse(entry)
      if result:
        count += 1
      if result is not None:
        valid_count += 1
    if not valid_count:
      return None
    return count

  # Make sure the data we're dealing with is a list of strings;
  #   or make it a list of strings if it's just a single entry
  if isinstance(paths, str):
    paths = [ paths ]
  else:
    assert isinstance(paths, list)
    for entry in paths:
      assert isinstance(entry, str)

  app.debug(str(paths))

  # Wait until all files exist
  num_exist = numExist(paths)
  if num_exist != len(paths):
    progress = app.progressBar('Waiting for creation of ' + (('new item \"' + paths[0] + '\"') if len(paths) == 1 else (str(len(paths)) + ' new items')), len(paths))
    for _ in range(num_exist):
      progress.increment()
    delay = 1.0/1024.0
    while not num_exist == len(paths):
      time.sleep(delay)
      new_num_exist = numExist(paths)
      if new_num_exist == num_exist:
        delay = max(60.0, delay*2.0)
      elif new_num_exist > num_exist:
        for _ in range(new_num_exist - num_exist):
          progress.increment()
        num_exist = new_num_exist
        delay = 1.0/1024.0
    progress.done()
  else:
    app.debug('Item' + ('s' if len(paths) > 1 else '') + ' existed immediately')

  # Check to see if active use of the file(s) needs to be tested
  at_least_one_file = False
  for entry in paths:
    if os.path.isfile(entry):
      at_least_one_file = True
      break
  if not at_least_one_file:
    app.debug('No target files, directories only; not testing for finalization')
    return

  # Can we query the in-use status of any of these paths
  num_in_use = numInUse(paths)
  if num_in_use is None:
    app.debug('Unable to test for finalization of new files')
    return

  # Wait until all files are not in use
  if not num_in_use:
    app.debug('Item' + ('s' if len(paths) > 1 else '') + ' immediately ready')
    return

  progress = app.progressBar('Waiting for finalization of ' + (('new file \"' + paths[0] + '\"') if len(paths) == 1 else (str(len(paths)) + ' new files')))
  for _ in range(len(paths) - num_in_use):
    progress.increment()
  delay = 1.0/1024.0
  while num_in_use:
    time.sleep(delay)
    new_num_in_use = numInUse(paths)
    if new_num_in_use == num_in_use:
      delay = max(60.0, delay*2.0)
    elif new_num_in_use < num_in_use:
      for _ in range(num_in_use - new_num_in_use):
        progress.increment()
      num_in_use = new_num_in_use
      delay = 1.0/1024.0
  progress.done()
