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

def execute(): #pylint: disable=unused-variable
  # Generate the images related to each tissue
  run.command('mrconvert input.mif -coord 3 1 CSF.mif')
  run.command('mrconvert input.mif -coord 3 2 cGM.mif')
  run.command('mrconvert input.mif -coord 3 3 cWM.mif')
  run.command('mrconvert input.mif -coord 3 4 sGM.mif')

  # Combine WM and subcortical WM into a unique WM image
  run.command('mrconvert input.mif - -coord 3 3,5 | mrmath - sum WM.mif -axis 3')

  # Create an empty lesion image
  run.command('mrcalc WM.mif 0 -mul lsn.mif')

  # Convert into the 5tt format
  run.command('mrcat cGM.mif sGM.mif WM.mif CSF.mif lsn.mif 5tt.mif -axis 3')

  if app.ARGS.nocrop:
    run.function(os.rename, '5tt.mif', 'result.mif')
  else:
    run.command('mrmath 5tt.mif sum - -axis 3 | mrthreshold - - -abs 0.5 | mrgrid 5tt.mif crop result.mif -mask -')

  run.command('mrconvert result.mif ' + path.from_user(app.ARGS.output), mrconvert_keyval=path.from_user(app.ARGS.input, False), force=app.FORCE_OVERWRITE)
