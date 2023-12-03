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
from mrtrix3 import app, image, path, run



def usage(base_parser, subparsers): #pylint: disable=unused-variable
  parser = subparsers.add_parser('fa', parents=[base_parser])
  parser.set_author('Robert E. Smith (robert.smith@florey.edu.au)')
  parser.set_synopsis('Use the old FA-threshold heuristic for single-fibre voxel selection and response function estimation')
  parser.add_citation('Tournier, J.-D.; Calamante, F.; Gadian, D. G. & Connelly, A. Direct estimation of the fiber orientation density function from diffusion-weighted MRI data using spherical deconvolution. NeuroImage, 2004, 23, 1176-1185')
  parser.add_argument('input', help='The input DWI')
  parser.add_argument('output', help='The output response function text file')
  options = parser.add_argument_group('Options specific to the \'fa\' algorithm')
  options.add_argument('-erode', type=int, default=3, help='Number of brain mask erosion steps to apply prior to threshold (not used if mask is provided manually)')
  options.add_argument('-number', type=int, default=300, help='The number of highest-FA voxels to use')
  options.add_argument('-threshold', type=float, help='Apply a hard FA threshold, rather than selecting the top voxels')
  parser.flag_mutually_exclusive_options( [ 'number', 'threshold' ] )



def check_output_paths(): #pylint: disable=unused-variable
  app.check_output_path(app.ARGS.output)



def get_inputs(): #pylint: disable=unused-variable
  pass



def needs_single_shell(): #pylint: disable=unused-variable
  return False



def supports_mask(): #pylint: disable=unused-variable
  return True



def execute(): #pylint: disable=unused-variable
  bvalues = [ int(round(float(x))) for x in image.mrinfo('dwi.mif', 'shell_bvalues').split() ]
  if len(bvalues) < 2:
    raise MRtrixError('Need at least 2 unique b-values (including b=0).')
  lmax_option = ''
  if app.ARGS.lmax:
    lmax_option = ' -lmax ' + app.ARGS.lmax
  if not app.ARGS.mask:
    run.command('maskfilter mask.mif erode mask_eroded.mif -npass ' + str(app.ARGS.erode))
    mask_path = 'mask_eroded.mif'
  else:
    mask_path = 'mask.mif'
  run.command('dwi2tensor dwi.mif -mask ' + mask_path + ' tensor.mif')
  run.command('tensor2metric tensor.mif -fa fa.mif -vector vector.mif -mask ' + mask_path)
  if app.ARGS.threshold:
    run.command('mrthreshold fa.mif voxels.mif -abs ' + str(app.ARGS.threshold))
  else:
    run.command('mrthreshold fa.mif voxels.mif -top ' + str(app.ARGS.number))
  run.command('dwiextract dwi.mif - -singleshell -no_bzero | amp2response - voxels.mif vector.mif response.txt' + lmax_option)

  run.function(shutil.copyfile, 'response.txt', path.from_user(app.ARGS.output, False))
  if app.ARGS.voxels:
    run.command('mrconvert voxels.mif ' + path.from_user(app.ARGS.voxels), mrconvert_keyval=path.from_user(app.ARGS.input, False), force=app.FORCE_OVERWRITE)
