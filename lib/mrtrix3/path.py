# A collection of functions used to operate upon file and directory paths

# Determines the common postfix for a list of filenames (including the file extension)
#pylint: disable=unused-variable
def commonPostfix(inputFiles):
  from mrtrix3 import app
  first = inputFiles[0]
  cursor = 0
  found = False
  common = ''
  for dummy_i in reversed(first):
    if not found:
      for j in inputFiles:
        if j[len(j)-cursor-1] != first[len(first)-cursor-1]:
          found = True
          break
      if not found:
        common = first[len(first)-cursor-1] + common
      cursor += 1
  app.debug('Common postfix of ' + str(len(inputFiles)) + ' is \'' + common + '\'')
  return common



# Get the full absolute path to a user-specified location.
#   This function serves two purposes:
#   To get the intended user-specified path when a script is operating inside a temporary directory, rather than
#     the directory that was current when the user specified the path;
#   To add quotation marks where the output path is being interpreted as part of a full command string
#     (e.g. to be passed to run.command()); without these quotation marks, paths that include spaces would be
#     erroneously split, subsequently confusing whatever command is being invoked.
#pylint: disable=unused-variable
def fromUser(filename, is_command):
  import os
  from mrtrix3 import app
  wrapper=''
  if is_command and (filename.count(' ') or app._workingDir.count(' ')):
    wrapper='\"'
  path = wrapper + os.path.abspath(os.path.join(app._workingDir, filename)) + wrapper
  app.debug(filename + ' -> ' + path)
  return path



# Get an appropriate location and name for a new temporary file / directory
# Note: Doesn't actually create anything; just gives a unique name that won't over-write anything
#pylint: disable=unused-variable
def newTemporary(suffix):
  import os, random, string, sys
  from mrtrix3 import app
  if 'TmpFileDir' in app.config:
    dir_path = app.config['TmpFileDir']
  elif app._tempDir:
    dir_path = app._tempDir
  else:
    dir_path = os.getcwd()
  if 'TmpFilePrefix' in app.config:
    prefix = app.config['TmpFilePrefix']
  else:
    prefix = 'mrtrix-tmp-'
  full_path = dir_path
  suffix = suffix.lstrip('.')
  while os.path.exists(full_path):
    random_string = ''.join(random.choice(string.ascii_uppercase + string.digits) for x in range(6))
    full_path = os.path.join(dir_path, prefix + random_string + '.' + suffix)
  app.debug(full_path)
  return full_path



# Determine the name of a sub-directory containing additional data / source files for a script
# This can be algorithm files in lib/mrtrix3, or data files in /share/mrtrix3/
#pylint: disable=unused-variable
def scriptSubDirName():
  import inspect, os
  from mrtrix3 import app
  # TODO Test this on multiple Python versions, with & without softlinking
  name = os.path.basename(inspect.stack()[-1][1])
  if not name[0].isalpha():
    name = '_' + name
  app.debug(name)
  return name



# Find data in the relevant directory
# Some scripts come with additional requisite data files; this function makes it easy to find them.
# For data that is stored in a named sub-directory specifically for a particular script, this function will
#   need to be used in conjunction with scriptSubDirName()
#pylint: disable=unused-variable
def sharedDataPath():
  import os
  from mrtrix3 import app
  result = os.path.realpath(os.path.abspath(os.path.join(os.path.dirname(os.path.realpath(__file__)), os.pardir, os.pardir, 'share', 'mrtrix3')))
  app.debug(result)
  return result



# Get the full absolute path to a location in the temporary script directory
#pylint: disable=unused-variable
def toTemp(filename, is_command):
  import os
  from mrtrix3 import app
  wrapper=''
  if is_command and filename.count(' '):
    wrapper='\"'
  path = wrapper + os.path.abspath(os.path.join(app._tempDir, filename)) + wrapper
  app.debug(filename + ' -> ' + path)
  return path
