# Copyright (c) 2008-2025 the MRtrix3 contributors.
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

import os, shlex, sys
from collections import namedtuple
from .version import VERSION



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
  COMMAND_HISTORY_STRING += '  (version=' + VERSION + ')'



# 'CONFIG' is a dictionary containing those entries present in the MRtrix config files
# Can add default values here that would otherwise appear in multiple locations
CONFIG = {
  'Dwi2maskAlgorithm': 'legacy'
}



BZERO_THRESHOLD_DEFAULT = 22.5 #pylint: disable=unused-variable



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
