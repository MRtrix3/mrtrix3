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
from mrtrix3 import app, run
from . import ANTS_BRAIN_EXTRACTION_CMD

def execute(): #pylint: disable=unused-variable
  ants_path = os.environ.get('ANTSPATH', '')
  if not ants_path:
    raise MRtrixError('Environment variable ANTSPATH is not set; '
                      'please appropriately confirure ANTs software')
  if not shutil.which(ANTS_BRAIN_EXTRACTION_CMD):
    raise MRtrixError('Unable to find command "'
                      + ANTS_BRAIN_EXTRACTION_CMD
                      + '"; please check ANTs installation')

  run.command(ANTS_BRAIN_EXTRACTION_CMD
              + ' -d 3'
              + ' -c 3x3x2x1'
              + ' -a bzero.nii'
              + ' -e template_image.nii'
              + ' -m template_mask.nii'
              + ' -o out'
              + ('' if app.DO_CLEANUP else ' -k 1')
              + (' -z' if app.VERBOSITY >= 3 else ''))

  return 'outBrainExtractionMask.nii.gz'
