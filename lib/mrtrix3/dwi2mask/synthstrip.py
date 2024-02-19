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
from mrtrix3 import app, run


NEEDS_MEAN_BZERO = True # pylint: disable=unused-variable
SYNTHSTRIP_CMD='mri_synthstrip'
SYNTHSTRIP_SINGULARITY='sythstrip-singularity'


def usage(base_parser, subparsers): #pylint: disable=unused-variable
  parser = subparsers.add_parser('synthstrip', parents=[base_parser])
  parser.set_author('Ruobing Chen (chrc@student.unimelb.edu.au)')
  parser.set_synopsis('Use the FreeSurfer Synthstrip method on the mean b=0 image')
  parser.add_description('This algorithm requires that the SynthStrip method be installed and available via PATH; '
                         'this could be via Freesufer 7.3.0 or later, '
                         'or the dedicated Singularity container.')
  parser.add_citation('A. Hoopes, J. S. Mora, A. V. Dalca, B. Fischl, M. Hoffmann. '
                      'SynthStrip: Skull-Stripping for Any Brain Image. '
                      'NeuroImage, 2022, 260, 119474',
                      is_external=True)
  parser.add_argument('input',
                      type=app.Parser.ImageIn(),
                      help='The input DWI series')
  parser.add_argument('output',
                      type=app.Parser.ImageOut(),
                      help='The output mask image')
  options=parser.add_argument_group('Options specific to the \'Synthstrip\' algorithm')
  options.add_argument('-stripped',
                       type=app.Parser.ImageOut(),
                       help='The output stripped image')
  options.add_argument('-gpu',
                       action='store_true',
                       default=False,
                       help='Use the GPU')
  options.add_argument('-model',
                       type=app.Parser.FileIn(),
                       metavar='file',
                       help='Alternative model weights')
  options.add_argument('-nocsf',
                       action='store_true',
                       default=False,
                       help='Compute the immediate boundary of brain matter excluding surrounding CSF')
  options.add_argument('-border',
                       type=app.Parser.Int(),
                       help='Control the boundary distance from the brain')



def execute(): #pylint: disable=unused-variable

  synthstrip_cmd = shutil.which(SYNTHSTRIP_CMD)
  if not synthstrip_cmd:
    synthstrip_cmd=shutil.which(SYNTHSTRIP_SINGULARITY)
    if not synthstrip_cmd:
      raise MRtrixError('Unable to locate "Synthstrip" executable; '
                        'please check installation')

  output_file = 'synthstrip_mask.nii'
  stripped_file = 'stripped.nii'
  cmd = [SYNTHSTRIP_CMD, '-i', 'bzero.nii', '-m', output_file]

  if app.ARGS.stripped:
    cmd.extend(['-o', stripped_file])
  if app.ARGS.gpu:
    cmd.append('-g')

  if app.ARGS.nocsf:
    cmd.append('--no-csf')

  if app.ARGS.border is not None:
    cmd.extend(['-b', str(app.ARGS.border)])

  if app.ARGS.model:
    cmd.extend(['--model', app.ARGS.model])

  run.command(cmd)
  if app.ARGS.stripped:
    run.command(['mrconvert', stripped_file, app.ARGS.stripped],
                mrconvert_keyval=app.ARGS.input,
                preserve_pipes=True,
                force=app.FORCE_OVERWRITE)
  return output_file
