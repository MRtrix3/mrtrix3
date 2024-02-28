# Copyright (c) 2008-2024 the MRtrix3 contributors.
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
from mrtrix3 import app, run

NEEDS_MEAN_BZERO = True # pylint: disable=unused-variable
AFNI3DAUTOMASK_CMD = '3dAutomask'


def usage(base_parser, subparsers): #pylint: disable=unused-variable
  parser = subparsers.add_parser('3dautomask', parents=[base_parser])
  parser.set_author('Ricardo Rios (ricardo.rios@cimat.mx)')
  parser.set_synopsis('Use AFNI 3dAutomask to derive a brain mask from the DWI mean b=0 image')
  parser.add_citation('RW Cox. '
                      'AFNI: Software for analysis and visualization of functional magnetic resonance neuroimages. '
                      'Computers and Biomedical Research, 29:162-173, 1996.',
                      is_external=True)
  parser.add_argument('input',
                      type=app.Parser.ImageIn(),
                      help='The input DWI series')
  parser.add_argument('output',
                      type=app.Parser.ImageOut(),
                      help='The output mask image')
  options = parser.add_argument_group('Options specific to the "3dautomask" algorithm')
  options.add_argument('-clfrac',
                       type=app.Parser.Float(0.1, 0.9),
                       help='Set the "clip level fraction"; '
                            'must be a number between 0.1 and 0.9. '
                            'A small value means to make the initial threshold for clipping smaller, '
                            'which will tend to make the mask larger.')
  options.add_argument('-nograd',
                       action='store_true',
                       help='The program uses a "gradual" clip level by default. '
                       'Add this option to use a fixed clip level.')
  options.add_argument('-peels',
                       type=app.Parser.Int(0),
                       metavar='iterations',
                       help='Peel (erode) the mask n times, '
                            'then unpeel (dilate).')
  options.add_argument('-nbhrs',
                       type=app.Parser.Int(6, 26),
                       metavar='count',
                       help='Define the number of neighbors needed for a voxel NOT to be eroded. '
                            'It should be between 6 and 26.')
  options.add_argument('-eclip',
                       action='store_true',
                       help='After creating the mask, '
                            'remove exterior voxels below the clip threshold.')
  options.add_argument('-SI',
                       type=app.Parser.Float(0.0),
                       help='After creating the mask, '
                            'find the most superior voxel, '
                            'then zero out everything more than SI millimeters inferior to that. '
                            '130 seems to be decent (i.e., for Homo Sapiens brains).')
  options.add_argument('-dilate',
                       type=app.Parser.Int(0),
                       metavar='iterations',
                       help='Dilate the mask outwards n times')
  options.add_argument('-erode',
                       type=app.Parser.Int(0),
                       metavar='iterations',
                       help='Erode the mask outwards n times')

  options.add_argument('-NN1',
                       action='store_true',
                       help='Erode and dilate based on mask faces')
  options.add_argument('-NN2',
                       action='store_true',
                       help='Erode and dilate based on mask edges')
  options.add_argument('-NN3',
                       action='store_true',
                       help='Erode and dilate based on mask corners')



def execute(): #pylint: disable=unused-variable
  if not shutil.which(AFNI3DAUTOMASK_CMD):
    raise MRtrixError(f'Unable to locate AFNI "{AFNI3DAUTOMASK_CMD}" executable; '
                      f'check installation')

  # main command to execute
  mask_path = 'afni_mask.nii.gz'
  cmd_string = [AFNI3DAUTOMASK_CMD, '-prefix', mask_path]

  # Adding optional parameters
  if app.ARGS.clfrac is not None:
    cmd_string.extend(['-clfrac', str(app.ARGS.clfrac)])
  if app.ARGS.peels is not None:
    cmd_string.extend(['-peels', str(app.ARGS.peels)])
  if app.ARGS.nbhrs is not None:
    cmd_string.extend(['-nbhrs', str(app.ARGS.nbhrs)])
  if app.ARGS.dilate is not None:
    cmd_string.extend(['-dilate', str(app.ARGS.dilate)])
  if app.ARGS.erode is not None:
    cmd_string.extend(['-erode', str(app.ARGS.erode)])
  if app.ARGS.SI is not None:
    cmd_string.extend(['-SI', str(app.ARGS.SI)])

  if app.ARGS.nograd:
    cmd_string.append('-nograd')
  if app.ARGS.eclip:
    cmd_string.append('-eclip')
  if app.ARGS.NN1:
    cmd_string.append('-NN1')
  if app.ARGS.NN2:
    cmd_string.append('-NN2')
  if app.ARGS.NN3:
    cmd_string.append('-NN3')

  # Adding input dataset to main command
  cmd_string.append('bzero.nii')

  # Execute main command for afni 3dautomask
  run.command(cmd_string)

  return mask_path
