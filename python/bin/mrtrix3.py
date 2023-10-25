
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

import importlib, os, sys

try:
  spec = importlib.util.spec_from_file_location('mrtrix3', os.path.normpath(os.path.join(os.path.realpath(__file__), '..', 'lib', '__init__.py')))
  module = importlib.util.module_from_spec(spec)
  sys.modules[spec.name] = module
  spec.loader.exec_module(module)
except ImportError:
  sys.stderr.write('''
ERROR: Unable to locate MRtrix3 Python modules

For detailed instructions, please refer to:
https://mrtrix.readthedocs.io/en/latest/tips_and_tricks/external_modules.html
''')
  sys.stderr.flush()
  sys.exit(1)
