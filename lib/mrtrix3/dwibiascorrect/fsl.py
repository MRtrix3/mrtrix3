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



def usage(base_parser, subparsers): #pylint: disable=unused-variable
  parser = subparsers.add_parser('fsl', parents=[base_parser])
  parser.set_author('Robert E. Smith (robert.smith@florey.edu.au)')
  parser.set_synopsis('Perform DWI bias field correction using the \'fast\' command as provided in FSL')
  parser.add_citation('Zhang, Y.; Brady, M. & Smith, S. Segmentation of brain MR images through a hidden Markov random field model and the expectation-maximization algorithm. IEEE Transactions on Medical Imaging, 2001, 20, 45-57', is_external=True)
  parser.add_citation('Smith, S. M.; Jenkinson, M.; Woolrich, M. W.; Beckmann, C. F.; Behrens, T. E.; Johansen-Berg, H.; Bannister, P. R.; De Luca, M.; Drobnjak, I.; Flitney, D. E.; Niazy, R. K.; Saunders, J.; Vickers, J.; Zhang, Y.; De Stefano, N.; Brady, J. M. & Matthews, P. M. Advances in functional and structural MR image analysis and implementation as FSL. NeuroImage, 2004, 23, S208-S219', is_external=True)
  parser.add_description('The FSL \'fast\' command only estimates the bias field within a brain mask, and cannot extrapolate this smoothly-varying field beyond the defined mask. As such, this algorithm by necessity introduces a hard masking of the input DWI. Since this attribute may interfere with the purpose of using the command (e.g. correction of a bias field is commonly used to improve brain mask estimation), use of this particular algorithm is generally not recommended.')
  parser.add_argument('input',  help='The input image series to be corrected')
  parser.add_argument('output', help='The output corrected image series')



def check_output_paths(): #pylint: disable=unused-variable
  pass



def get_inputs(): #pylint: disable=unused-variable
  pass



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
