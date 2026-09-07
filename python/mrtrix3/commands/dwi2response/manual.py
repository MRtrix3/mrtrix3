# Copyright (c) 2008-2026 the MRtrix3 contributors.
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
from mrtrix3 import app, image, run

NEEDS_SINGLE_SHELL = False # pylint: disable=unused-variable
SUPPORTS_MASK = False # pylint: disable=unused-variable


def usage(base_parser, subparsers): #pylint: disable=unused-variable
  parser = subparsers.add_parser('manual', parents=[base_parser])
  parser.set_author('Robert E. Smith (robert.smith@florey.edu.au)')
  parser.set_synopsis('Derive a response function using an input mask image alone '
                      '(i.e. pre-selected voxels)')
  parser.add_argument('input',
                      type=app.Parser.ImageIn(),
                      help='The input DWI')
  parser.add_argument('in_voxels',
                      type=app.Parser.ImageIn(),
                      help='Input voxel selection mask')
  parser.add_argument('output',
                      type=app.Parser.FileOut(),
                      help='Output response function text file')
  options = parser.add_argument_group('Options specific to the "manual" algorithm')
  options.add_argument('-dirs',
                       type=app.Parser.ImageIn(),
                       help='Provide an input image that contains a pre-estimated fibre direction in each voxel '
                            '(a tensor fit will be used otherwise)')



def execute(): #pylint: disable=unused-variable
  if os.path.exists('mask.mif'):
    app.warn('-mask option is ignored by algorithm "manual"')
    os.remove('mask.mif')

  run.command(['mrconvert', app.ARGS.in_voxels, 'in_voxels.mif'],
              preserve_pipes=True)
  if app.ARGS.dirs:
    run.command(['mrconvert', app.ARGS.dirs, 'dirs.mif', '-strides', '0,0,0,1'],
                preserve_pipes=True)

  shells = [ int(round(float(x))) for x in image.mrinfo('dwi.mif', 'shell_bvalues').split() ]
  bvalues_option = f' -shells {",".join(map(str,shells))}'

  # Get lmax information (if provided)
  lmax_option = ''
  if app.ARGS.lmax:
    if len(app.ARGS.lmax) != len(shells):
      raise MRtrixError(f'Number of manually-defined lmax\'s ({len(app.ARGS.lmax)}) '
                        f'does not match number of b-value shells ({len(shells)})')
    if any(l % 2 for l in app.ARGS.lmax):
      raise MRtrixError('Values for lmax must be even')
    if any(l < 0 for l in app.ARGS.lmax):
      raise MRtrixError('Values for lmax must be non-negative')
    lmax_option = f' -lmax {",".join(map(str,app.ARGS.lmax))}'

  # Do we have directions, or do we need to calculate them?
  if not os.path.exists('dirs.mif'):
    run.command('dwi2tensor dwi.mif - -mask in_voxels.mif | '
                'tensor2metric - -vector dirs.mif')

  # Get response function
  run.command('amp2response dwi.mif in_voxels.mif dirs.mif response.txt' + bvalues_option + lmax_option)

  run.function(shutil.copyfile, 'response.txt', app.ARGS.output)
  if app.ARGS.voxels:
    run.command(['mrconvert', 'in_voxels.mif', app.ARGS.voxels],
                mrconvert_keyval=app.ARGS.input,
                force=app.FORCE_OVERWRITE,
                preserve_pipes=True)
