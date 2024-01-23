
# Copyright (c) 2008-2019 the MRtrix3 contributors.
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

import imp, os, sys

def imported(lib_path):
  success = False
  fp = None
  try:
    fp, pathname, description = imp.find_module('mrtrix3', [ lib_path ])
    imp.load_module('mrtrix3', fp, pathname, description)
    success = True
  except ImportError:
    pass
  finally:
    if fp:
      fp.close()
  return success

# Can the MRtrix3 Python modules be found based on their relative location to this file?
# Note that this includes the case where this file is a softlink within an external module,
# which provides a direct link to the core installation
if not imported (os.path.normpath (os.path.join ( \
  os.path.dirname (os.path.realpath (__file__)), os.pardir, 'lib') )):

  # If this file is a duplicate, which has been stored in an external module,
  # we may be able to figure out the location of the core library using the
  # build script.

  # case 1: build is a symbolic link:
  if not imported (os.path.join (os.path.dirname (os.path.realpath ( \
      os.path.join (os.path.dirname(__file__), os.pardir, 'build'))), 'lib')):

    # case 2: build is a file containing the path to the core build script:
    try:
      with open (os.path.join (os.path.dirname(__file__), os.pardir, 'build')) as fp:
        for line in fp:
          build_path = line.split ('#',1)[0].strip()
          if build_path:
            break
    except IOError:
      pass

    if not imported (os.path.join (os.path.dirname (build_path), 'lib')):

      sys.stderr.write('''
ERROR: Unable to locate MRtrix3 Python modules

For detailed instructions, please refer to:
https://mrtrix.readthedocs.io/en/latest/tips_and_tricks/external_modules.html
''')
      sys.stderr.flush()
      sys.exit(1)
