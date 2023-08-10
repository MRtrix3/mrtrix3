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

# Various utility functions / classes that don't sensibly slot into any other module



import errno, os, platform, random, re, string
from mrtrix3 import CONFIG



# A simple wrapper class for executing a set of commands or functions of some known length,
#   generating and managing a progress bar as it does so
# Can use in one of two ways:
# - Construct using a progress bar message, and the number of commands / functions that are to be executed;
#     each is then executed by calling member functions command() and function(), which
#     use the corresponding functions in the mrtrix3.run module
# - Construct using a progress bar message, and a list of command strings to run;
#     all commands within the list will be executed sequentially within the constructor
class RunList: #pylint: disable=unused-variable
  def __init__(self, message, value):
    from mrtrix3 import app, run #pylint: disable=import-outside-toplevel
    if isinstance(value, int):
      self.progress = app.ProgressBar(message, value)
      self.target_count = value
      self.counter = 0
      self.valid = True
    elif isinstance(value, list):
      assert all(isinstance(entry, str) for entry in value)
      self.progress = app.ProgressBar(message, len(value))
      for entry in value:
        run.command(entry)
        self.progress.increment()
      self.progress.done()
      self.valid = False
    else:
      raise TypeError('Construction of RunList class expects either an '
                      'integer (number of commands/functions to run), or a '
                      'list of command strings to execute')
  def command(self, cmd, **kwargs):
    from mrtrix3 import run #pylint: disable=import-outside-toplevel
    assert self.valid
    run.command(cmd, **kwargs)
    self._increment()
  def function(self, func, *args, **kwargs):
    from mrtrix3 import run #pylint: disable=import-outside-toplevel
    assert self.valid
    run.function(func, *args, **kwargs)
    self._increment()
  def _increment(self):
    self.counter += 1
    if self.counter == self.target_count:
      self.progress.done()
      self.valid = False
    else:
      self.progress.increment()



# Return a boolean flag to indicate whether or not script is being run on a Windows machine
def is_windows(): #pylint: disable=unused-variable
  system = platform.system().lower()
  return any(system.startswith(s) for s in [ 'mingw', 'msys', 'nt', 'windows' ])



# Load key-value entries from the comments within a text file
def load_keyval(filename, **kwargs): #pylint: disable=unused-variable
  comments = kwargs.pop('comments', '#')
  encoding = kwargs.pop('encoding', 'latin1')
  errors = kwargs.pop('errors', 'ignore')
  if kwargs:
    raise TypeError('Unsupported keyword arguments passed to utils.load_keyval(): ' + str(kwargs))

  def decode(line):
    if isinstance(line, bytes):
      line = line.decode(encoding, errors=errors)
    return line

  if comments:
    regex_comments = re.compile('|'.join(comments))

  res = {}
  with open(filename, 'rb') as infile:
    for line in infile.readlines():
      line = decode(line)
      if comments:
        line = regex_comments.split(line, maxsplit=1)[0]
        if len(line) < 2:
          continue
        name, var = line.rstrip().partition(":")[::2]
        if name in res:
          res[name].append(var.split())
        else:
          res[name] = var.split()
  return res



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
