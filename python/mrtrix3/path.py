# Copyright (c) 2008-2026 the MRtrix3 contributors.
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



import ctypes, os, shutil, subprocess, time



# List the content of a directory
def all_in_dir(directory, **kwargs): #pylint: disable=unused-variable
  from mrtrix3 import utils #pylint: disable=import-outside-toplevel
  dir_path = kwargs.pop('dir_path', True)
  ignore_hidden_files = kwargs.pop('ignore_hidden_files', True)
  if kwargs:
    raise TypeError('Unsupported keyword arguments passed to path.all_in_dir(): '
                    + str(kwargs))
  def is_hidden(directory, filename):
    if utils.is_windows():
      try:
        attrs = ctypes.windll.kernel32.GetFileAttributesW(str(os.path.join(directory, filename)))
        assert attrs != -1
        return bool(attrs & 2)
      except (AttributeError, AssertionError):
        return filename.startswith('.')
    return filename.startswith('.')
  flist = sorted([filename for filename in os.listdir(directory) if not ignore_hidden_files or not is_hidden(directory, filename) ])
  if dir_path:
    return [ os.path.join(directory, filename) for filename in flist ]
  return flist



# Find data in the relevant directory
# Some scripts come with additional requisite data files; this function makes it easy to find them.
# For data that is stored in a named sub-directory specifically for a particular script, this function will
#   need to be used in conjunction with scriptSubDirName()
def shared_data_path(): #pylint: disable=unused-variable
  from mrtrix3 import app #pylint: disable=import-outside-toplevel
  result = os.path.realpath(os.path.abspath(os.path.join(os.path.dirname(os.path.realpath(__file__)), os.pardir, os.pardir, 'share', 'mrtrix3')))
  app.debug(result)
  return result



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

  def num_exist(data):
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
  current_num_exist = num_exist(paths)
  if current_num_exist != len(paths):
    item_message = f'new item "{paths[0]}"' if len(paths) == 1 else f'{len(paths)} new items'
    progress = app.ProgressBar(f'Waiting for creation of {item_message}', len(paths))
    for _ in range(num_exist):
      progress.increment()
    delay = 1.0/1024.0
    while not current_num_exist == len(paths):
      time.sleep(delay)
      new_num_exist = num_exist(paths)
      if new_num_exist == current_num_exist:
        delay = min(60.0, delay*2.0)
      elif new_num_exist > current_num_exist:
        for _ in range(new_num_exist - current_num_exist):
          progress.increment()
        current_num_exist = new_num_exist
        delay = 1.0/1024.0
    progress.done()
  else:
    app.debug(f'{"Items" if len(paths) > 1 else "Item"} existed immediately')

  # Check to see if active use of the file(s) needs to be tested
  at_least_one_file = False
  for entry in paths:
    if os.path.isfile(entry):
      at_least_one_file = True
      break
  if not at_least_one_file:
    app.debug('No target files, directories only; '
              'not testing for finalization')
    return

  # Can we query the in-use status of any of these paths
  current_num_in_use = num_in_use(paths)
  if current_num_in_use is None:
    app.debug('Unable to test for finalization of new files')
    return

  # Wait until all files are not in use
  if not current_num_in_use:
    app.debug(f'{"Items" if len(paths) > 1 else "Item"} immediately ready')
    return

  item_message = f'new file "{paths[0]}"' if len(paths) == 1 else f'{len(paths)} new files'
  progress = app.ProgressBar('Waiting for finalization of {item_message}')
  for _ in range(len(paths) - current_num_in_use):
    progress.increment()
  delay = 1.0/1024.0
  while current_num_in_use:
    time.sleep(delay)
    new_num_in_use = current_num_in_use(paths)
    if new_num_in_use == current_num_in_use:
      delay = min(60.0, delay*2.0)
    elif new_num_in_use < current_num_in_use:
      for _ in range(current_num_in_use - new_num_in_use):
        progress.increment()
      current_num_in_use = new_num_in_use
      delay = 1.0/1024.0
  progress.done()
