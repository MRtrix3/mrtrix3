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
from mrtrix3 import app, image, path, run



def usage(base_parser, subparsers): #pylint: disable=unused-variable
  parser = subparsers.add_parser('manual', parents=[base_parser])
  parser.set_author('Robert E. Smith (robert.smith@florey.edu.au)')
  parser.set_synopsis('Derive a response function using an input mask image alone (i.e. pre-selected voxels)')
  parser.add_argument('input', type=app.Parser.ImageIn(), help='The input DWI')
  parser.add_argument('in_voxels', type=app.Parser.ImageIn(), help='Input voxel selection mask')
  parser.add_argument('output', type=app.Parser.FileOut(), help='Output response function text file')
  options = parser.add_argument_group('Options specific to the \'manual\' algorithm')
  options.add_argument('-dirs', type=app.Parser.ImageIn(), metavar='image', help='Provide an input image that contains a pre-estimated fibre direction in each voxel (a tensor fit will be used otherwise)')



def check_output_paths(): #pylint: disable=unused-variable
  app.check_output_path(app.ARGS.output)



def get_inputs(): #pylint: disable=unused-variable
  mask_path = path.to_scratch('mask.mif', False)
  if os.path.exists(mask_path):
    app.warn('-mask option is ignored by algorithm \'manual\'')
    os.remove(mask_path)
  run.command('mrconvert ' + path.from_user(app.ARGS.in_voxels) + ' ' + path.to_scratch('in_voxels.mif'),
              preserve_pipes=True)
  if app.ARGS.dirs:
    run.command('mrconvert ' + path.from_user(app.ARGS.dirs) + ' ' + path.to_scratch('dirs.mif') + ' -strides 0,0,0,1',
                preserve_pipes=True)



def needs_single_shell(): #pylint: disable=unused-variable
  return False



def supports_mask(): #pylint: disable=unused-variable
  return False



def execute(): #pylint: disable=unused-variable
  shells = [ int(round(float(x))) for x in image.mrinfo('dwi.mif', 'shell_bvalues').split() ]
  bvalues_option = ' -shells ' + ','.join(map(str,shells))

  # Get lmax information (if provided)
  lmax_option = ''
  if app.ARGS.lmax:
    if len(app.ARGS.lmax) != len(shells):
      raise MRtrixError('Number of manually-defined lmax\'s (' + str(len(app.ARGS.lmax)) + ') does not match number of b-value shells (' + str(len(shells)) + ')')
    if any(l % 2 for l in app.ARGS.lmax):
      raise MRtrixError('Values for lmax must be even')
    if any(l < 0 for l in app.ARGS.lmax):
      raise MRtrixError('Values for lmax must be non-negative')
    lmax_option = ' -lmax ' + ','.join(map(str,app.ARGS.lmax))

  # Do we have directions, or do we need to calculate them?
  if not os.path.exists('dirs.mif'):
    run.command('dwi2tensor dwi.mif - -mask in_voxels.mif | tensor2metric - -vector dirs.mif')

  # Get response function
  run.command('amp2response dwi.mif in_voxels.mif dirs.mif response.txt' + bvalues_option + lmax_option)

  run.function(shutil.copyfile, 'response.txt', path.from_user(app.ARGS.output, False))
  if app.ARGS.voxels:
    run.command('mrconvert in_voxels.mif ' + path.from_user(app.ARGS.voxels),
                mrconvert_keyval=path.from_user(app.ARGS.input, False),
                force=app.FORCE_OVERWRITE,
                preserve_pipes=True)
