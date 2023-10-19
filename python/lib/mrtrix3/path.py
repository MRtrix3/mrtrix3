# Copyright (c) 2008-2023 the MRtrix3 contributors.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# Covered Software is provided under this License on an "as is"
# basis, without warranty of any kind, either expressed, implied, or
# statutory, including, without limitation, warranties that the
# Covered Software is free of defects, merchantable, fit for a
# particular purpose or non-infringing.
# See the Mozilla Public License v. 2.0 for more details.
#
# For more details, see http://www.mrtrix.org/.

# Collection of convenience functions for manipulating filesystem paths



import ctypes, errno, inspect, os, random, shlex, shutil, string, subprocess, time
from mrtrix3 import CONFIG



# List the content of a directory
def all_in_dir(directory, **kwargs): #pylint: disable=unused-variable
  from mrtrix3 import utils #pylint: disable=import-outside-toplevel
  dir_path = kwargs.pop('dir_path', True)
  ignore_hidden_files = kwargs.pop('ignore_hidden_files', True)
  if kwargs:
    raise TypeError('Unsupported keyword arguments passed to path.all_in_dir(): ' + str(kwargs))
  def is_hidden(directory, filename):
    if utils.is_windows():
      try:
        attrs = ctypes.windll.kernel32.GetFileAttributesW("%s" % str(os.path.join(directory, filename)))
        assert attrs != -1
        return bool(attrs & 2)
      except (AttributeError, AssertionError):
        return filename.startswith('.')
    return filename.startswith('.')
  flist = sorted([filename for filename in os.listdir(directory) if not ignore_hidden_files or not is_hidden(directory, filename) ])
  if dir_path:
    return [ os.path.join(directory, filename) for filename in flist ]
  return flist



# Get the full absolute path to a user-specified location.
# This function serves two purposes:
# - To get the intended user-specified path when a script is operating inside a scratch directory, rather than
#     the directory that was current when the user specified the path;
# - To add quotation marks where the output path is being interpreted as part of a full command string
#     (e.g. to be passed to run.command()); without these quotation marks, paths that include spaces would be
#     erroneously split, subsequently confusing whatever command is being invoked.
#   If the filesystem path provided by the script is to be interpreted in isolation, rather than as one part
#     of a command string, then parameter 'escape' should be set to False in order to not add quotation marks
def from_user(filename, escape=True): #pylint: disable=unused-variable
  from mrtrix3 import app #pylint: disable=import-outside-toplevel
  fullpath = os.path.abspath(os.path.join(app.WORKING_DIR, filename))
  if escape:
    fullpath = shlex.quote(fullpath)
  app.debug(filename + ' -> ' + fullpath)
  return fullpath



# Make a directory if it doesn't exist; don't do anything if it does already exist
def make_dir(path): #pylint: disable=unused-variable
  from mrtrix3 import app #pylint: disable=import-outside-toplevel
  try:
    os.makedirs(path)
    app.debug('Created directory ' + path)
  except OSError as exception:
    if exception.errno != errno.EEXIST:
      raise
    app.debug('Directory \'' + path + '\' already exists')



# Make a temporary empty file / directory with a unique name
# If the filesystem path separator is provided as the 'suffix' input, then the function will generate a new
#   directory rather than a file.
def make_temporary(suffix): #pylint: disable=unused-variable
  from mrtrix3 import app #pylint: disable=import-outside-toplevel
  is_directory = suffix in '\\/' and len(suffix) == 1
  while True:
    temp_path = name_temporary(suffix)
    try:
      if is_directory:
        os.makedirs(temp_path)
      else:
        with open(temp_path, 'a', encoding='utf-8'):
          pass
      app.debug(temp_path)
      return temp_path
    except OSError as exception:
      if exception.errno != errno.EEXIST:
        raise



# Get an appropriate location and name for a new temporary file / directory
# Note: Doesn't actually create anything; just gives a unique name that won't over-write anything.
# If you want to create a temporary file / directory, use the make_temporary() function above.
def name_temporary(suffix): #pylint: disable=unused-variable
  from mrtrix3 import app #pylint: disable=import-outside-toplevel
  dir_path = CONFIG['TmpFileDir'] if 'TmpFileDir' in CONFIG else (app.SCRATCH_DIR if app.SCRATCH_DIR else os.getcwd())
  prefix = CONFIG['TmpFilePrefix'] if 'TmpFilePrefix' in CONFIG else 'mrtrix-tmp-'
  full_path = dir_path
  suffix = suffix.lstrip('.')
  while os.path.exists(full_path):
    random_string = ''.join(random.choice(string.ascii_uppercase + string.ascii_lowercase + string.digits) for x in range(6))
    full_path = os.path.join(dir_path, prefix + random_string + '.' + suffix)
  app.debug(full_path)
  return full_path



# Determine the name of a sub-directory containing additional data / source files for a script
# This can be algorithm files in lib/mrtrix3/, or data files in share/mrtrix3/
# This function appears here rather than in the algorithm module as some scripts may
#   need to access the shared data directory but not actually be using the algorithm module
def script_subdir_name(): #pylint: disable=unused-variable
  from mrtrix3 import app #pylint: disable=import-outside-toplevel
  frameinfo = inspect.stack()[-1]
  try:
    frame = frameinfo.frame
  except AttributeError: # Prior to Version 3.5
    frame = frameinfo[0]
  # If the script has been run through a softlink, we need the name of the original
  #   script in order to locate the additional data
  name = os.path.basename(os.path.realpath(inspect.getfile(frame)))
  if not name[0].isalpha():
    name = '_' + name
  app.debug(name)
  return name



# Find data in the relevant directory
# Some scripts come with additional requisite data files; this function makes it easy to find them.
# For data that is stored in a named sub-directory specifically for a particular script, this function will
#   need to be used in conjunction with scriptSubDirName()
def shared_data_path(): #pylint: disable=unused-variable
  from mrtrix3 import app #pylint: disable=import-outside-toplevel
  result = os.path.realpath(os.path.abspath(os.path.join(os.path.dirname(os.path.realpath(__file__)), os.pardir, os.pardir, 'share', 'mrtrix3')))
  app.debug(result)
  return result



# Get the full absolute path to a location in the script's scratch directory
# Also deals with the potential for special characters in a path (e.g. spaces) by wrapping in quotes,
#   as long as parameter 'escape' is true (if the path yielded by this function is to be interpreted in
#   isolation rather than as one part of a command string, parameter 'escape' should be set to False)
def to_scratch(filename, escape=True): #pylint: disable=unused-variable
  from mrtrix3 import app #pylint: disable=import-outside-toplevel
  fullpath = os.path.abspath(os.path.join(app.SCRATCH_DIR, filename))
  if escape:
    fullpath = shlex.quote(fullpath)
  app.debug(filename + ' -> ' + fullpath)
  return fullpath



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
def wait_for(paths): #pylint: disable=unused-variable
  from mrtrix3 import app, utils #pylint: disable=import-outside-toplevel

  def in_use(path):
    if not os.path.isfile(path):
      return None
    if utils.is_windows():
      if not os.access(path, os.W_OK):
        return None
      try:
        with open(path, 'rb+') as dummy_f:
          pass
        return False
      except IOError:
        return True
    if not shutil.which('fuser'):
      return None
    # fuser returns zero if there IS at least one process accessing the file
    # A fatal error will result in a non-zero code -> in_use() = False, so wait_for() can return
    return not subprocess.call(['fuser', '-s', path], shell=False, stdin=None, stdout=None, stderr=None)

  def num_exit(data):
    count = 0
    for entry in data:
      if os.path.exists(entry):
        count += 1
    return count

  def num_in_use(data):
    count = 0
    valid_count = 0
    for entry in data:
      result = in_use(entry)
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
  num_exist = num_exit(paths)
  if num_exist != len(paths):
    progress = app.ProgressBar('Waiting for creation of ' + (('new item \"' + paths[0] + '\"') if len(paths) == 1 else (str(len(paths)) + ' new items')), len(paths))
    for _ in range(num_exist):
      progress.increment()
    delay = 1.0/1024.0
    while not num_exist == len(paths):
      time.sleep(delay)
      new_num_exist = num_exit(paths)
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
  num_in_use = num_in_use(paths)
  if num_in_use is None:
    app.debug('Unable to test for finalization of new files')
    return

  # Wait until all files are not in use
  if not num_in_use:
    app.debug('Item' + ('s' if len(paths) > 1 else '') + ' immediately ready')
    return

  progress = app.ProgressBar('Waiting for finalization of ' + (('new file \"' + paths[0] + '\"') if len(paths) == 1 else (str(len(paths)) + ' new files')))
  for _ in range(len(paths) - num_in_use):
    progress.increment()
  delay = 1.0/1024.0
  while num_in_use:
    time.sleep(delay)
    new_num_in_use = num_in_use(paths)
    if new_num_in_use == num_in_use:
      delay = max(60.0, delay*2.0)
    elif new_num_in_use < num_in_use:
      for _ in range(num_in_use - new_num_in_use):
        progress.increment()
      num_in_use = new_num_in_use
      delay = 1.0/1024.0
  progress.done()
