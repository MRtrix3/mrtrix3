import os, sys
from collections import namedtuple
from mrtrix3._version import __version__



class MRtrixBaseError(Exception):
  pass

class MRtrixError(MRtrixBaseError): #pylint: disable=unused-variable
  pass



# Contains the command currently being executed, appended with the version of the MRtrix3 Python library
COMMAND_STRING = sys.argv[0]
try:
  from shlex import quote
except ImportError:
  from pipes import quote
for arg in sys.argv[1:]:
  COMMAND_STRING += ' ' + quote(arg) # Use quotation marks only if required
COMMAND_STRING += '  (version=' + __version__ + ')"'


# Location of binaries that belong to the same MRtrix3 installation as the Python library being invoked
BIN_PATH = os.path.abspath(os.path.join(os.path.abspath(os.path.dirname(os.path.abspath(__file__))), os.pardir, os.pardir, 'bin'))
# Must remove the '.exe' from Windows binary executables
EXE_LIST = [ os.path.splitext(name)[0] for name in os.listdir(BIN_PATH) ] #pylint: disable=unused-variable



# - 'CONFIG' is a directory containing those entries present in the MRtrix config files
CONFIG = { }

# Codes for printing information to the terminal
ANSICodes = namedtuple('ANSI', 'lineclear clear console debug error execute warn')
ANSI = ANSICodes('', '', '', '', '', '', '') #pylint: disable=unused-variable



# Load the MRtrix configuration files here, and create a dictionary
# Load system config first, user second: Allows user settings to override
for config_path in [ os.environ.get ('MRTRIX_CONFIGFILE', os.path.join(os.path.sep, 'etc', 'mrtrix.conf')),
                     os.path.join(os.path.expanduser('~'), '.mrtrix.conf') ]:
  try:
    f = open (config_path, 'r')
    for line in f:
      line = line.strip().split(': ')
      if len(line) != 2:
        continue
      if line[0][0] == '#':
        continue
      CONFIG[line[0]] = line[1]
  except IOError:
    pass

# Set up terminal special characters now, since they may be dependent on the config file
if sys.stderr.isatty() and not ('TerminalColor' in CONFIG and CONFIG['TerminalColor'].lower() in ['no', 'false', '0']):
  ANSI = ANSICodes('\033[0K', '\033[0m', '\033[03;32m', '\033[03;34m', '\033[01;31m', '\033[03;36m', '\033[00;31m') #pylint: disable=unused-variable
