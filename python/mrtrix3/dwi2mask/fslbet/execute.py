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
from mrtrix3 import app, fsl, image, run

def execute(): #pylint: disable=unused-variable
  if not os.environ.get('FSLDIR', ''):
    raise MRtrixError('Environment variable FSLDIR is not set; please run appropriate FSL configuration script')
  bet_cmd = fsl.exe_name('bet')

  # Starting brain masking using BET
  if app.ARGS.rescale:
    run.command('mrconvert bzero.nii bzero_rescaled.nii -vox 1,1,1')
    vox = image.Header('bzero.nii').spacing()
    b0_image = 'bzero_rescaled.nii'
  else:
    b0_image = 'bzero.nii'

  cmd_string = bet_cmd + ' ' + b0_image + ' DWI_BET -R -m'

  if app.ARGS.bet_f is not None:
    cmd_string += ' -f ' + str(app.ARGS.bet_f)
  if app.ARGS.bet_g is not None:
    cmd_string += ' -g ' + str(app.ARGS.bet_g)
  if app.ARGS.bet_r is not None:
    cmd_string += ' -r ' + str(app.ARGS.bet_r)
  if app.ARGS.bet_c is not None:
    cmd_string += ' -c ' + ' '.join(app.ARGS.bet_c)

  # Running BET command
  run.command(cmd_string)
  mask = fsl.find_image('DWI_BET_mask')

  if app.ARGS.rescale:
    run.command('mrconvert ' + mask + ' mask_rescaled.nii -vox ' + ','.join(str(value) for value in vox))
    return 'mask_rescaled.nii'
  return mask
