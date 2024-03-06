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
  mask_path = path.to_scratch('mask.mif', False)
  if os.path.exists(mask_path):
    app.warn('-mask option is ignored by algorithm \'manual\'')
    os.remove(mask_path)
  run.command('mrconvert ' + path.from_user(app.ARGS.in_voxels) + ' ' + path.to_scratch('in_voxels.mif'))
  if app.ARGS.dirs:
    run.command('mrconvert ' + path.from_user(app.ARGS.dirs) + ' ' + path.to_scratch('dirs.mif') + ' -strides 0,0,0,1')
