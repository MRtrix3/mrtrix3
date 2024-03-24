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

from mrtrix3 import CONFIG, MRtrixError
from mrtrix3 import app, path, run

def get_inputs(): #pylint: disable=unused-variable
  if app.ARGS.template:
    run.command('mrconvert ' + app.ARGS.template[0] + ' ' + path.to_scratch('template_image.nii')
                + ' -strides +1,+2,+3')
    run.command('mrconvert ' + app.ARGS.template[1] + ' ' + path.to_scratch('template_mask.nii')
                + ' -strides +1,+2,+3 -datatype uint8')
  elif all(item in CONFIG for item in ['Dwi2maskTemplateImage', 'Dwi2maskTemplateMask']):
    run.command('mrconvert ' + CONFIG['Dwi2maskTemplateImage'] + ' ' + path.to_scratch('template_image.nii')
                + ' -strides +1,+2,+3')
    run.command('mrconvert ' + CONFIG['Dwi2maskTemplateMask'] + ' ' + path.to_scratch('template_mask.nii')
                + ' -strides +1,+2,+3 -datatype uint8')
  else:
    raise MRtrixError('No template image information available from '
                      'either command-line or MRtrix configuration file(s)')
