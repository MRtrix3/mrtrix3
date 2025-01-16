# Copyright (c) 2008-2025 the MRtrix3 contributors.
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

import importlib



def usage(cmdline): #pylint: disable=unused-variable
  from mrtrix3 import app, version #pylint: disable=no-name-in-module, import-outside-toplevel

  cmdline.set_author('Robert E. Smith (robert.smith@florey.edu.au)'
                     ' and Warda Syeda (wtsyeda@unimelb.edu.au)')
  cmdline.set_synopsis('Generate a binary mask from DWI data')
  cmdline.add_description('This script serves as an interface for many different algorithms'
                          ' that generate a binary mask from DWI data in different ways.'
                          ' Each algorithm available has its own help page,'
                          ' including necessary references;'
                          ' e.g. to see the help page of the "fslbet" algorithm,'
                          ' type "dwi2mask fslbet".')
  cmdline.add_description('More information on mask derivation from DWI data can be found at the following link: \n'
                          f'https://mrtrix.readthedocs.io/en/{version.TAG}/dwi_preprocessing/masking.html')

  # General options
  #common_options = cmdline.add_argument_group('General dwi2mask options')
  app.add_dwgrad_import_options(cmdline)

  # Import the command-line settings for all algorithms found in the relevant directory
  cmdline.add_subparsers()



def execute(): #pylint: disable=unused-variable
  from mrtrix3 import MRtrixError #pylint: disable=no-name-in-module, import-outside-toplevel
  from mrtrix3 import app, image, run #pylint: disable=no-name-in-module, import-outside-toplevel

  # Load module for the user-requested algorithm
  alg = importlib.import_module(f'.{app.ARGS.algorithm}', 'mrtrix3.commands.dwi2mask')

  input_header = image.Header(app.ARGS.input)
  image.check_3d_nonunity(input_header)
  grad_import_option = app.dwgrad_import_options()
  if not grad_import_option and 'dw_scheme' not in input_header.keyval():
    raise MRtrixError('Script requires diffusion gradient table: '
                      'either in image header, or using -grad / -fslgrad option')

  app.activate_scratch_dir()
  # Get input data into the scratch directory
  run.command(['mrconvert', app.ARGS.input, 'input.mif', '-strides', '0,0,0,1']
              + grad_import_option,
              preserve_pipes=True)

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
  strides = ','.join(map(str, strides))

  # From here, the script splits depending on what algorithm is being used
  # The return value of the execute() function should be the name of the
  #   image in the scratch directory that is to be exported
  mask_path = alg.execute()

  # Before exporting the mask image, get a mask of voxels for which
  #   the DWI data are valid
  #   (want to ensure that no algorithm includes any voxels where
  #   there is no valid DWI data, regardless of how they operate)
  run.command(['mrcalc', mask_path, 'input_pos_mask.mif', '-mult', '-', '|',
               'mrconvert', '-', app.ARGS.output, '-strides', strides, '-datatype', 'bit'],
              mrconvert_keyval=app.ARGS.input,
              force=app.FORCE_OVERWRITE,
              preserve_pipes=True)
