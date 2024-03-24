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

import importlib, sys
from mrtrix3 import CONFIG, MRtrixError
from mrtrix3 import app, image, path, run

def execute(): #pylint: disable=unused-variable

  # Find out which algorithm the user has requested
  algorithm_module_name = 'mrtrix3.dwibiascorrect.' + app.ARGS.algorithm
  alg = sys.modules[algorithm_module_name]

  app.check_output_path(app.ARGS.output)
  app.check_output_path(app.ARGS.bias)
  importlib.import_module('.check_output_paths', algorithm_module_name)
  alg.check_output_paths.check_output_paths()

  app.make_scratch_dir()

  grad_import_option = app.read_dwgrad_import_options()
  run.command('mrconvert ' + path.from_user(app.ARGS.input) + ' ' + path.to_scratch('in.mif') + grad_import_option)
  if app.ARGS.mask:
    run.command('mrconvert ' + path.from_user(app.ARGS.mask) + ' ' + path.to_scratch('mask.mif') + ' -datatype bit')

  importlib.import_module('.get_inputs', algorithm_module_name)
  alg.get_inputs.get_inputs()

  app.goto_scratch_dir()

  # Make sure it's actually a DWI that's been passed
  dwi_header = image.Header('in.mif')
  if len(dwi_header.size()) != 4:
    raise MRtrixError('Input image must be a 4D image')
  if 'dw_scheme' not in dwi_header.keyval():
    raise MRtrixError('No valid DW gradient scheme provided or present in image header')
  if len(dwi_header.keyval()['dw_scheme']) != dwi_header.size()[3]:
    raise MRtrixError('DW gradient scheme contains different number of entries (' + str(len(dwi_header.keyval()['dw_scheme'])) + ' to number of volumes in DWIs (' + dwi_header.size()[3] + ')')

  # Generate a brain mask if required, or check the mask if provided by the user
  if app.ARGS.mask:
    if not image.match('in.mif', 'mask.mif', up_to_dim=3):
      raise MRtrixError('Provided mask image does not match input DWI')
  else:
    run.command('dwi2mask ' + CONFIG['Dwi2maskAlgorithm'] + ' in.mif mask.mif')

  # From here, the script splits depending on what estimation algorithm is being used
  importlib.import_module('.execute', algorithm_module_name)
  alg.execute.execute()
