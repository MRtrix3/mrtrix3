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

import importlib, sys
from mrtrix3 import app

def execute(): #pylint: disable=unused-variable

  # Find out which algorithm the user has requested
  algorithm_module_name = 'mrtrix3.dwinormalise.' + app.ARGS.algorithm
  alg = sys.modules[algorithm_module_name]
  importlib.import_module('.check_output_paths', algorithm_module_name)
  alg.check_output_paths.check_output_paths()

  # From here, the script splits depending on what algorithm is being used
  importlib.import_module('.execute', algorithm_module_name)
  alg.execute.execute()
