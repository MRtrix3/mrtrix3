# This file handles cases where the invoked script is calling upon other MRtrix3 functionalities;
#   e.g. calling binaries from within runCommand(). Whenever such functionality is used, we want
#   to ensure that the binary / script invoked comes from the same version of MRtrix3 as the
#   script currently being executed.



# A list of MRtrix3 executable files; includes both binaries and scripts
exe_list = [ ]

# Absolute paths to the MRtrix3 binaries and scripts coinciding with this installation
binary_path = ''
script_path = ''

# Command-line options that should be appended to any MRtrix commands invoked
optionForce = ''
optionVerbosity = ' -quiet'
optionNThreads = ''

# Key-value pairs stored in any MRtrix config files present
config = { }


def initialise():
  import os, sys
  import lib.package
  global exe_list, binary_path, script_path, config
  sys.stderr.write('Starting lib.mrtrix.initialise()\n')
  if exe_list:
    sys.stderr.write('Script error: Do not invoke lib.mrtrix.initialise() function more than once')
    sys.exit(1)
  # os.path.abspath gymnastics ensures that even if the script is being called using a symlink, any MRtrix3 binaries called during the
  #   course of this script come from the same version that the invoked script originates
  if lib.package.isPackaged():
    # Binaries and scripts all reside in the same directory
    # TODO This probably doesn't work: Will point to the temporary extracted package file, which doesn't coincide with the MRtrix3 installation
    binary_path = os.path.abspath(os.path.join(os.path.abspath(os.path.dirname(os.path.realpath(__file__))), os.pardir))
    script_path = binary_path
    # On Windows, strip the .exe's from binary paths
    exe_list = [ os.path.splitext(name)[0] for name in os.listdir(binary_path) ]
  else:
    binary_path = os.path.abspath(os.path.join(os.path.abspath(os.path.dirname(os.path.realpath(__file__))), os.pardir, os.pardir, 'release', 'bin'))
    exe_list = [ os.path.splitext(name)[0] for name in os.listdir(binary_path) ]
    # Also need to add the contents of scripts directory - only script files, not the sub-directories
    script_path = os.path.abspath(os.path.join(os.path.abspath(os.path.dirname(os.path.realpath(__file__))), os.pardir))
    exe_list.extend( [ f for f in os.listdir(script_path) if os.path.isfile(os.path.join(script_path, f)) ] )
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
  import lib.misc
  from distutils.spawn import find_executable
  global exe_list, binary_path, script_path
  if not exe_list:
    sys.stderr.write('Script error: Do not invoke lib.mrtrix.versionMatch() without having first run lib.mrtrix.initialise()')
    sys.exit(1)
  if not item in exe_list:
    sys.stderr.write('Script error: lib.mrtrix.versionMatch() called for non-MRtrix command (' + cmd + ')')
    sys.exit(1)
  exe_path_sys = find_executable(item)
  exe_path_manual = os.path.join(binary_path, item)
  if (lib.misc.isWindows()):
    exe_path_manual = exe_path_manual + '.exe'
  if not os.path.isfile(exe_path_manual):
    exe_path_manual = os.path.join(lib.mrtrix.script_path, item)
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


