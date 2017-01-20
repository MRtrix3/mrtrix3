# This file provides functionality for locating resources and controlling code execution depending on
#   whether the script is being run directly from the MRtrix3 source, or whether it has been compiled
#   into a single executable via PyInstaller and provided as part of an MRtrix3 package.
#
# PyInstaller creates a temporary directory when extracting the contents of the compiled package,
#   and stores the path to this directory in sys._MEIPASS. It also creates an attribute sys.frozen, which
#   is absent otherwise and can hence be used to detect whether the running script has been packaged.
#





def isPackaged():
  import sys
  return getattr(sys, 'frozen', False)



# Helper function for lib/algorithm.py
def algorithmsPath():
  import os, sys
  # Deal with adding the '_' for e.g. 5ttgen
  # TODO This needs to find the name of the underlying script; *not* the name of e.g. a symlink to it
  # TODO Should this name be stored in e.g. lib.app?
  script_name = os.path.basename(sys.argv[0])
  if not script_name[0].isalpha():
    script_name = '_' + script_name
  if isPackaged():
    # During the packaging process, algorithm files are copied from the relevant sub-directory of
    #   scripts/src/, and will be extracted to a directory with the same name as the script
    #   when the packaged script is executed
    return os.path.join(sys._MEIPASS, script_name)
  else:
    return os.path.abspath(os.path.join(os.path.abspath(os.path.dirname(os.path.realpath(__file__))), os.pardir, 'src', script_name))



# When additional data files are provided, these reside in scripts/data/ for a repository clone;
#   but for a packaged script, these will instead reside simply in data/ relative to the
#   extraction directory.
def dataPath(filename):
  if isPackaged():
    return os.path.join(sys._MEIPASS, 'data', filename)
  else:
    return os.path.abspath(os.path.join(os.path.abspath(os.path.dirname(os.path.realpath(__file__))), os.pardir, 'data', filename))

