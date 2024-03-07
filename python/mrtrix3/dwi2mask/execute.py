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
from mrtrix3 import MRtrixError
from mrtrix3 import app, image, path, run

def execute(): #pylint: disable=unused-variable

  # Find out which algorithm the user has requested
  algorithm_module_name = 'mrtrix3.dwi2mask.' + app.ARGS.algorithm
  alg = sys.modules[algorithm_module_name]

  app.check_output_path(app.ARGS.output)

  input_header = image.Header(path.from_user(app.ARGS.input, False))
  image.check_3d_nonunity(input_header)
  grad_import_option = app.read_dwgrad_import_options()
  if not grad_import_option and 'dw_scheme' not in input_header.keyval():
    raise MRtrixError('Script requires diffusion gradient table: '
                      'either in image header, or using -grad / -fslgrad option')

  app.make_scratch_dir()

  # Get input data into the scratch directory
  run.command('mrconvert ' + path.from_user(app.ARGS.input) + ' ' + path.to_scratch('input.mif')
              + ' -strides 0,0,0,1' + grad_import_option)
  importlib.import_module('.get_inputs', algorithm_module_name)
  alg.get_inputs.get_inputs()

  app.goto_scratch_dir()

  # Generate a mean b=0 image (common task in many algorithms)
  if alg.NEEDS_MEAN_BZERO:
    run.command('dwiextract input.mif -bzero - | '
                'mrmath - mean - -axis 3 | '
                'mrconvert - bzero.nii -strides +1,+2,+3')

  # Get a mask of voxels for which the DWI data are valid
  #   (want to ensure that no algorithm includes any voxels where
  #   there is no valid DWI data, regardless of how they operate)
  run.command('mrmath input.mif max - -axis 3 | '
              'mrthreshold - -abs 0 -comparison gt input_pos_mask.mif')

  # Make relative strides of three spatial axes of output mask equivalent
  #   to input DWI; this may involve decrementing magnitude of stride
  #   if the input DWI is volume-contiguous
  strides = image.Header('input.mif').strides()[0:3]
  strides = [(abs(value) + 1 - min(abs(v) for v in strides)) * (-1 if value < 0 else 1) for value in strides]

  # From here, the script splits depending on what algorithm is being used
  # The return value of the execute() function should be the name of the
  #   image in the scratch directory that is to be exported
  importlib.import_module('.execute', algorithm_module_name)
  mask_path = alg.execute.execute()

  # Before exporting the mask image, get a mask of voxels for which
  #   the DWI data are valid
  #   (want to ensure that no algorithm includes any voxels where
  #   there is no valid DWI data, regardless of how they operate)
  run.command('mrcalc '
              + mask_path
              + ' input_pos_mask.mif -mult -'
              + ' |'
              + ' mrconvert - '
              + path.from_user(app.ARGS.output)
              + ' -strides ' + ','.join(str(value) for value in strides)
              + ' -datatype bit',
              mrconvert_keyval=path.from_user(app.ARGS.input, False),
              force=app.FORCE_OVERWRITE)
