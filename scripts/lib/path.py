# A collection of functions used to operate upon file and directory paths

# Determines the common postfix for a list of filenames (including the file extension)
def commonPostfix(inputFiles):
  import lib.message
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
  lib.message.debug('Common postfix of ' + str(len(inputFiles)) + ' is \'' + common + '\'')
  return common



# Get the full absolute path to a user-specified location.
#   This function serves two purposes:
#   To get the intended user-specified path when a script is operating inside a temporary directory, rather than
#     the directory that was current when the user specified the path;
#   To add quotation marks where the output path is being interpreted as part of a full command string
#     (e.g. to be passed to runCommand()); without these quotation marks, paths that include spaces would be
#     erroneously split, subsequently confusing whatever command is being invoked.
def fromUser(filename, is_command):
  import os
  import lib.app
  import lib.message
  wrapper=''
  if is_command and (filename.count(' ') or lib.app.workingDir.count(' ')):
    wrapper='\"'
  path = wrapper + os.path.abspath(os.path.join(lib.app.workingDir, filename)) + wrapper
  lib.message.debug(path)
  return path

