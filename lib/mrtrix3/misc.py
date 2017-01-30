# A collection of miscellaneous functions that couldn't be grouped with anything else

# Determine whether or not a particular command of interest can be found within the
#   locations contained in the PATH environment variable, and is executable
def haveBinary(cmdname):
  import os
  from mrtrix3 import message
  for path_dir in os.environ["PATH"].split(os.pathsep):
    path_dir = path_dir.strip('"')
    full_path = os.path.join(path_dir, cmdname)
    if os.path.isfile(full_path) and os.access(full_path, os.X_OK):
      message.debug('Command \'' + cmdname + '\' found in PATH')
      return True
  message.debug('Command \'' + cmdname + '\' NOT found in PATH')
  return False



# Return a boolean flag to indicate whether or not script is being run on a Windows machine
def isWindows():
  import platform
  from mrtrix3 import message
  system = platform.system().lower()
  result = system.startswith('mingw') or system.startswith('msys') or system.startswith('windows')
  message.debug('System = ' + system + '; is Windows? ' + str(result))
  return result

