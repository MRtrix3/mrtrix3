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

import inspect, os, shlex, sys
from collections import namedtuple
from mrtrix3._version import __version__



class MRtrixBaseError(Exception):
  pass

class MRtrixError(MRtrixBaseError): #pylint: disable=unused-variable
  pass



# Contains the command currently being executed, appended with the version of the MRtrix3 Python library
COMMAND_HISTORY_STRING = None
if sys.argv:
  COMMAND_HISTORY_STRING = sys.argv[0]
  for arg in sys.argv[1:]:
    COMMAND_HISTORY_STRING += ' ' + shlex.quote(arg) # Use quotation marks only if required
  COMMAND_HISTORY_STRING += '  (version=' + __version__ + ')'


# Location of binaries that belong to the same MRtrix3 installation as the Python library being invoked
BIN_PATH = os.path.abspath(os.path.join(os.path.abspath(os.path.dirname(os.path.abspath(__file__))), os.pardir, os.pardir, 'bin'))
# Must remove the '.exe' from Windows binary executables
EXE_LIST = [ os.path.splitext(name)[0] for name in os.listdir(BIN_PATH) ] #pylint: disable=unused-variable


# - 'CONFIG' is a directory containing those entries present in the MRtrix config files
CONFIG = { }


# Codes for printing information to the terminal
ANSICodes = namedtuple('ANSI', 'lineclear clear console debug error execute warn')
ANSI = ANSICodes('\033[0K', '', '', '', '', '', '') #pylint: disable=unused-variable


# Load the MRtrix configuration files here, and create a dictionary
# Load system config first, user second: Allows user settings to override
for config_path in [ os.environ.get ('MRTRIX_CONFIGFILE', os.path.join(os.path.sep, 'etc', 'mrtrix.conf')),
                     os.path.join(os.path.expanduser('~'), '.mrtrix.conf') ]:
  try:
    with open (config_path, 'r', encoding='utf-8') as f:
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
def setup_ansi():
  global ANSI
  if sys.stderr.isatty() and not ('TerminalColor' in CONFIG and CONFIG['TerminalColor'].lower() in ['no', 'false', '0']):
    ANSI = ANSICodes('\033[0K', '\033[0m', '\033[03;32m', '\033[03;34m', '\033[01;31m', '\033[03;36m', '\033[00;31m') #pylint: disable=unused-variable
setup_ansi()



# Execute a command
def execute(): #pylint: disable=unused-variable
  from . import app #pylint: disable=import-outside-toplevel
  app._execute(inspect.getmodule(inspect.stack()[1][0])) # pylint: disable=protected-access
