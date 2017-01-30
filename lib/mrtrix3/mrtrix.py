# This file handles cases where the invoked script is calling upon other MRtrix3 functionalities;
#   e.g. calling binaries from within run.command(). Whenever such functionality is used, we want
#   to ensure that the binary / script invoked comes from the same version of MRtrix3 as the
#   script currently being executed.



# A list of MRtrix3 executable files; includes both binaries and scripts
exe_list = [ ]

# Absolute path to the MRtrix3 binaries and scripts coinciding with this installation
exe_path = ''

# Command-line options that should be appended to any MRtrix commands invoked
optionForce = ''
optionVerbosity = ' -quiet'
optionNThreads = ''

# Key-value pairs stored in any MRtrix config files present
config = { }


def initialise():
  import os, sys
  global exe_list, exe_path, config
  if exe_list:
    sys.stderr.write('Script error: Do not invoke mrtrix.initialise() function more than once')
    sys.exit(1)
  # If allowing installation to put library contents in /lib/mrtrix3/ rather than within the MRtrix3 installation (e.g. /usr/local/mrtrix3/lib/mrtrix3/),
  #   this will need to be updated using inspect module
  exe_path = os.path.join(os.path.abspath(os.path.dirname(os.path.abspath(__file__))), os.pardir, os.pardir, 'bin')
  # Remember to remove the '.exe' from Windows binary executables
  exe_list = [ os.path.splitext(name)[0] for name in os.listdir(exe_path) ]
  # Load the MRtrix configuration files here, and create a dictionary
  # Load system config first, user second: Allows user settings to override
  for path in [ '/etc/mrtrix.conf', '.mrtrix.conf' ]:
    sys.stderr.write(path + '\n')
    try:
      f = open (path, 'r')
      for line in f:
        line = line.strip().split(': ')
        if len(line) != 2: continue
        if line[0][0] == '#': continue
        config[line[0]] = line[1]
    except IOError:
      pass



# Make sure we're not accidentally running an MRtrix executable on the system that belongs to a different installation
#   to the script currently being executed
def exeVersionMatch(item):
  import distutils, os, sys
  from mrtrix3 import misc
  from distutils.spawn import find_executable
  global exe_list, exe_path
  if not exe_list:
    sys.stderr.write('Script error: Do not invoke mrtrix.versionMatch() without having first run mrtrix.initialise()')
    sys.exit(1)
  if not item in exe_list:
    sys.stderr.write('Script error: mrtrix.versionMatch() called for non-MRtrix command (' + item + ')')
    sys.exit(1)
  exe_path_sys = find_executable(item)
  exe_path_manual = os.path.join(exe_path, item)
  if (misc.isWindows()):
    exe_path_manual = exe_path_manual + '.exe'
  if not os.path.isfile(exe_path_manual):
    exe_path_manual = ''
  # Always use the manual path if the item isn't found in the system path
  use_manual_exe_path = not exe_path_sys
  if not use_manual_exe_path:
    # os.path.samefile() not supported on all platforms / Python versions
    if hasattr(os.path, 'samefile'):
      use_manual_exe_path = not os.path.samefile(exe_path_sys, exe_path_manual)
    else:
      # Hack equivalent of samefile(); not perfect, but should be adequate for use here
      use_manual_exe_path = not os.path.normcase(os.path.normpath(exe_path_sys)) == os.path.normcase(os.path.normpath(exe_path_manual))
  if use_manual_exe_path:
    return exe_path_manual
  else:
    return item


