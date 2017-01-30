# A collection of functions used to operate upon file and directory paths

# Determines the common postfix for a list of filenames (including the file extension)
def commonPostfix(inputFiles):
  from mrtrix3 import message
  first = inputFiles[0];
  cursor = 0
  found = False;
  common = ''
  for i in reversed(first):
    if found == False:
      for j in inputFiles:
        if j[len(j)-cursor-1] != first[len(first)-cursor-1]:
          found = True
          break
      if found == False:
        common = first[len(first)-cursor-1] + common
      cursor += 1
  message.debug('Common postfix of ' + str(len(inputFiles)) + ' is \'' + common + '\'')
  return common



# Get the full absolute path to a user-specified location.
#   This function serves two purposes:
#   To get the intended user-specified path when a script is operating inside a temporary directory, rather than
#     the directory that was current when the user specified the path;
#   To add quotation marks where the output path is being interpreted as part of a full command string
#     (e.g. to be passed to run.command()); without these quotation marks, paths that include spaces would be
#     erroneously split, subsequently confusing whatever command is being invoked.
def fromUser(filename, is_command):
  import os
  from mrtrix3 import app, message
  wrapper=''
  if is_command and (filename.count(' ') or app.workingDir.count(' ')):
    wrapper='\"'
  path = wrapper + os.path.abspath(os.path.join(app.workingDir, filename)) + wrapper
  message.debug(path)
  return path



# Determine the name of a sub-directory containing additional data / source files for a script
# This can be algorithm files in lib/mrtrix3, or data files in /share/mrtrix3/
def scriptSubDirName():
  import inspect, os
  # TODO Test this on multiple Python versions, with & without softlinking
  name = os.path.basename(inspect.stack()[-1][1])
  if not name[0].isalpha():
    name = '_' + name
  return name



# Find data in the relevant directory
# Some scripts come with additional requisite data files; this function makes it easy to find them.
# For data that is stored in a named sub-directory specifically for a particular script, this function will
#   need to be used in conjunction with scriptSubDirName()
def sharedDataPath(filename):
  import inspect, os
  return os.path.realpath(os.path.abspath(os.path.join(os.path.dirname(os.path.realpath(__file__)), os.pardir, os.pardir, 'share', 'mrtrix3')))

