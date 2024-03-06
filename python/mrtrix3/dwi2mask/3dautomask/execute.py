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

import shutil
from mrtrix3 import MRtrixError
from mrtrix3 import app, run
from . import AFNI3DAUTOMASK_CMD

def execute(): #pylint: disable=unused-variable
  if not shutil.which(AFNI3DAUTOMASK_CMD):
    raise MRtrixError('Unable to locate AFNI "'
                      + AFNI3DAUTOMASK_CMD
                      + '" executable; check installation')

  # main command to execute
  mask_path = 'afni_mask.nii.gz'
  cmd_string = AFNI3DAUTOMASK_CMD + ' -prefix ' + mask_path

  # Adding optional parameters
  if app.ARGS.clfrac is not None:
    cmd_string += ' -clfrac ' + str(app.ARGS.clfrac)
  if app.ARGS.peels is not None:
    cmd_string += ' -peels ' + str(app.ARGS.peels)
  if app.ARGS.nbhrs is not None:
    cmd_string += ' -nbhrs ' + str(app.ARGS.nbhrs)
  if app.ARGS.dilate is not None:
    cmd_string += ' -dilate ' + str(app.ARGS.dilate)
  if app.ARGS.erode is not None:
    cmd_string += ' -erode ' + str(app.ARGS.erode)
  if app.ARGS.SI is not None:
    cmd_string += ' -SI ' + str(app.ARGS.SI)

  if app.ARGS.nograd:
    cmd_string += ' -nograd'
  if app.ARGS.eclip:
    cmd_string += ' -eclip'
  if app.ARGS.NN1:
    cmd_string += ' -NN1'
  if app.ARGS.NN2:
    cmd_string += ' -NN2'
  if app.ARGS.NN3:
    cmd_string += ' -NN3'

  # Adding input dataset to main command
  cmd_string +=  ' bzero.nii'

  # Execute main command for afni 3dautomask
  run.command(cmd_string)

  return mask_path
