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

import os
from mrtrix3 import MRtrixError
from mrtrix3 import app, image, run



def usage(base_parser, subparsers): #pylint: disable=unused-variable
  parser = subparsers.add_parser('gif', parents=[base_parser])
  parser.set_author('Matteo Mancini (m.mancini@ucl.ac.uk)')
  parser.set_synopsis('Generate the 5TT image based on a Geodesic Information Flow (GIF) segmentation image')
  parser.add_argument('input',
                      type=app.Parser.ImageIn(),
                      help='The input Geodesic Information Flow (GIF) segmentation image')
  parser.add_argument('output',
                      type=app.Parser.ImageOut(),
                      help='The output 5TT image')



def execute(): #pylint: disable=unused-variable
  header = image.Header(app.ARGS.input)
  if len(header.size()) < 4:
    raise MRtrixError(f'Image "{header.name()}" does not look like GIF segmentation '
                       '(less than 4 spatial dimensions)')
  if min(header.size()[:4]) == 1:
    raise MRtrixError(f'Image "{header.name()}" does not look like GIF segmentation '
                       '(axis with size 1)')
  run.command(['mrconvert', app.ARGS.input, 'input.mif'],
              preserve_pipes=True)

  # Generate the images related to each tissue
  run.command('mrconvert input.mif -coord 3 1 CSF.mif')
  run.command('mrconvert input.mif -coord 3 2 cGM.mif')
  run.command('mrconvert input.mif -coord 3 3 cWM.mif')
  run.command('mrconvert input.mif -coord 3 4 sGM.mif')

  # Combine WM and subcortical WM into a unique WM image
  run.command('mrconvert input.mif - -coord 3 3,5 | '
              'mrmath - sum WM.mif -axis 3')

  # Create an empty lesion image
  run.command('mrcalc WM.mif 0 -mul lsn.mif')

  # Convert into the 5tt format
  run.command('mrcat cGM.mif sGM.mif WM.mif CSF.mif lsn.mif 5tt.mif -axis 3')

  if app.ARGS.nocrop:
    run.function(os.rename, '5tt.mif', 'result.mif')
  else:
    run.command('mrmath 5tt.mif sum - -axis 3 | '
                'mrthreshold - - -abs 0.5 | '
                'mrgrid 5tt.mif crop result.mif -mask -')

  run.command(['mrconvert', 'result.mif', app.ARGS.output],
              mrconvert_keyval=app.ARGS.input,
              force=app.FORCE_OVERWRITE,
              preserve_pipes=True)

  return 'result.mif'
