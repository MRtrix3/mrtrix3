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

import os, shutil
from mrtrix3 import MRtrixError
from mrtrix3 import app, image, path, run

def execute(): #pylint: disable=unused-variable
  shells = [ int(round(float(x))) for x in image.mrinfo('dwi.mif', 'shell_bvalues').split() ]

  # Get lmax information (if provided)
  lmax = [ ]
  if app.ARGS.lmax:
    lmax = [ int(x.strip()) for x in app.ARGS.lmax.split(',') ]
    if not len(lmax) == len(shells):
      raise MRtrixError('Number of manually-defined lmax\'s (' + str(len(lmax)) + ') does not match number of b-value shells (' + str(len(shells)) + ')')
    for shell_l in lmax:
      if shell_l % 2:
        raise MRtrixError('Values for lmax must be even')
      if shell_l < 0:
        raise MRtrixError('Values for lmax must be non-negative')

  # Do we have directions, or do we need to calculate them?
  if not os.path.exists('dirs.mif'):
    run.command('dwi2tensor dwi.mif - -mask in_voxels.mif | tensor2metric - -vector dirs.mif')

  # Get response function
  bvalues_option = ' -shells ' + ','.join(map(str,shells))
  lmax_option = ''
  if lmax:
    lmax_option = ' -lmax ' + ','.join(map(str,lmax))
  run.command('amp2response dwi.mif in_voxels.mif dirs.mif response.txt' + bvalues_option + lmax_option)

  run.function(shutil.copyfile, 'response.txt', path.from_user(app.ARGS.output, False))
  if app.ARGS.voxels:
    run.command('mrconvert in_voxels.mif ' + path.from_user(app.ARGS.voxels), mrconvert_keyval=path.from_user(app.ARGS.input, False), force=app.FORCE_OVERWRITE)
