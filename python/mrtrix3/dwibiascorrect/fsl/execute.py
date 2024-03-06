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
from mrtrix3 import MRtrixError
from mrtrix3 import app, fsl, path, run, utils

def execute(): #pylint: disable=unused-variable
  if utils.is_windows():
    raise MRtrixError('Script cannot run using FSL on Windows due to FSL dependency')

  if not os.environ.get('FSLDIR', ''):
    raise MRtrixError('Environment variable FSLDIR is not set; please run appropriate FSL configuration script')

  fast_cmd = fsl.exe_name('fast')

  app.warn('Use of fsl algorithm in dwibiascorrect script is discouraged due to its strong dependence ' + \
           'on brain masking (specifically its inability to correct voxels outside of this mask).' + \
           'Use of the ants algorithm is recommended for quantitative DWI analyses.')

  # Generate a mean b=0 image
  run.command('dwiextract in.mif - -bzero | mrmath - mean mean_bzero.mif -axis 3')

  # FAST doesn't accept a mask input; therefore need to explicitly mask the input image
  run.command('mrcalc mean_bzero.mif mask.mif -mult - | mrconvert - mean_bzero_masked.nii -strides -1,+2,+3')
  run.command(fast_cmd + ' -t 2 -o fast -n 3 -b mean_bzero_masked.nii')
  bias_path = fsl.find_image('fast_bias')

  # Rather than using a bias field estimate of 1.0 outside the brain mask, zero-fill the
  #   output image outside of this mask
  run.command('mrcalc in.mif ' + bias_path + ' -div mask.mif -mult result.mif')
  run.command('mrconvert result.mif ' + path.from_user(app.ARGS.output), mrconvert_keyval=path.from_user(app.ARGS.input, False), force=app.FORCE_OVERWRITE)
  if app.ARGS.bias:
    run.command('mrconvert ' + bias_path + ' ' + path.from_user(app.ARGS.bias), mrconvert_keyval=path.from_user(app.ARGS.input, False), force=app.FORCE_OVERWRITE)
