# Copyright (c) 2008-2019 the MRtrix3 contributors.
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
#from distutils.spawn import find_executable
from mrtrix3 import MRtrixError
from mrtrix3 import app, fsl, image, path, run, utils



def usage(base_parser, subparsers): #pylint: disable=unused-variable
  parser = subparsers.add_parser('fslbet', parents=[base_parser])
  parser.set_author('Warda Syeda (wtsyeda@unimelb.edu.au) and Robert E. Smith (robert.smith@florey.edu.au)')
  parser.set_synopsis('Use FSL commands to generate mask image using BET')
  parser.add_citation('Smith, S. M. Fast robust automated brain extraction. Human Brain Mapping, 2002, 17, 143-155', is_external=True)
  parser.add_argument('input',  help='The input DWI image')
  parser.add_argument('output', help='The output mask image')
  options = parser.add_argument_group('Options specific to the \'fsl\' algorithm')
  options.add_argument('-f', metavar='<f float>', help='Fractional intensity threshold (0->1); default=0.5; smaller values give larger brain outline estimates')  #Ask Robert: how to add metavar for float?
  options.add_argument('-g', metavar='<g float>', help='vertical gradient in fractional intensity threshold (-1->1); default=0; positive values give larger brain outline at bottom, smaller at top')
  options.add_argument('-c', metavar='<x float y float z float>', help='centre-of-gravity (voxels not mm) of initial mesh surface.')
  options.add_argument('-r', metavar='<r float>', help='head radius (mm not voxels); initial surface sphere is set to half of this')
  options.add_argument('-rescale', action='store_true', help=' Use this option to rescale voxel size to 1mm isotropic, works best for rodent data')



def check_output_paths(): #pylint: disable=unused-variable
  app.check_output_path(app.ARGS.output)



def get_inputs(): #pylint: disable=unused-variable
  image.check_3d_nonunity(path.from_user(app.ARGS.input, False))
  run.command('mrconvert ' + path.from_user(app.ARGS.input) + ' ' + path.to_scratch('input.mif'))



def execute(): #pylint: disable=unused-variable
  if utils.is_windows():
    raise MRtrixError('\'fsl\' algorithm of dwi2mask script cannot be run on Windows: FSL not available on Windows')

  fsl_path = os.environ.get('FSLDIR', '')
  if not fsl_path:
    raise MRtrixError('Environment variable FSLDIR is not set; please run appropriate FSL configuration script')

  bet_cmd = fsl.exe_name('bet')
  fsl_suffix = fsl.suffix()

  # Calculating mean b0 image for BET
  run.command('dwiextract input.mif - -bzero | mrmath - mean mean_bzero.nii -axis 3')

  # Starting brain masking using BET
  if app.ARGS.rescale:
    run.command('mrconvert mean_bzero.nii mean_bzero_rescaled.nii -vox 1,1,1')
    vox=image.Header('mean_bzero.nii').spacing()
    b0_image='mean_bzero_rescaled.nii'
  else:
    b0_image='mean_bzero.nii'

  cmd_string=bet_cmd + ' ' + b0_image + ' ' + 'DWI_BET ' + ' -R -m'

  if app.ARGS.f:
    cmd_string += ' -f ' + app.ARGS.f
  if app.ARGS.g:
    cmd_string += ' -g ' + app.ARGS.g
  if app.ARGS.r:
    cmd_string += ' -r ' + app.ARGS.r
  if app.ARGS.c:
    cmd_string += ' -c ' + app.ARGS.c

# Running BET command
  run.command(cmd_string)

  mask = fsl.find_image('DWI_BET_mask' + fsl_suffix)

 
  
  run.command('mrconvert '+ mask + ' ' + path.from_user(app.ARGS.output) + (' -vox ' + ','.join(str(value) for value in vox) if app.ARGS.rescale else ''), mrconvert_keyval=path.from_user(app.ARGS.input, False), force=app.FORCE_OVERWRITE)


