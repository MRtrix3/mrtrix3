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
from mrtrix3 import CONFIG, MRtrixError
from mrtrix3 import app, path, run
from . import DEFAULT_SOFTWARE

def get_inputs(): #pylint: disable=unused-variable

  reg_software = app.ARGS.software if app.ARGS.software else CONFIG.get('Dwi2maskTemplateSoftware', DEFAULT_SOFTWARE)
  if reg_software.startswith('ants'):
    if not os.environ.get('ANTSPATH', ''):
      raise MRtrixError('Environment variable ANTSPATH is not set; '
                        'please appropriately configure ANTs software')
    if app.ARGS.ants_options:
      if os.path.isfile(path.from_user(app.ARGS.ants_options, False)):
        run.function(shutil.copyfile, path.from_user(app.ARGS.ants_options, False), path.to_scratch('ants_options.txt', False))
  elif reg_software == 'fsl':
    fsl_path = os.environ.get('FSLDIR', '')
    if not fsl_path:
      raise MRtrixError('Environment variable FSLDIR is not set; '
                        'please run appropriate FSL configuration script')
    if app.ARGS.fnirt_config:
      fnirt_config = path.from_user(app.ARGS.fnirt_config, False)
      if not os.path.isfile(fnirt_config):
        raise MRtrixError('No file found at -fnirt_config location "' + fnirt_config + '"')
    elif 'Dwi2maskTemplateFSLFnirtConfig' in CONFIG:
      fnirt_config = CONFIG['Dwi2maskTemplateFSLFnirtConfig']
      if not os.path.isfile(fnirt_config):
        raise MRtrixError('No file found at config file entry "Dwi2maskTemplateFSLFnirtConfig" location "' + fnirt_config + '"')
    else:
      fnirt_config = None
    if fnirt_config:
      run.function(shutil.copyfile, fnirt_config, path.to_scratch('fnirt_config.cnf', False))
  else:
    assert False

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
