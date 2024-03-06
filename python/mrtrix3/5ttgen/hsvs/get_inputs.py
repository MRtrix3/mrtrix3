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

import os
from mrtrix3 import app, path, run

def get_inputs(): #pylint: disable=unused-variable
  # Most freeSurfer files will be accessed in-place; no need to pre-convert them into the temporary directory
  # However convert aparc image so that it does not have to be repeatedly uncompressed
  run.command('mrconvert ' + path.from_user(os.path.join(app.ARGS.input, 'mri', 'aparc+aseg.mgz'), True) + ' ' + path.to_scratch('aparc.mif', True))
  if app.ARGS.template:
    run.command('mrconvert ' + path.from_user(app.ARGS.template, True) + ' ' + path.to_scratch('template.mif', True) + ' -axes 0,1,2')
