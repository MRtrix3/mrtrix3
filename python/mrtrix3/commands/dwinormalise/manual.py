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

import math
from mrtrix3 import app, run


DEFAULT_TARGET_INTENSITY=1000



def usage(base_parser, subcommands): #pylint: disable=unused-variable
  parser = subcommands.add_subcommand('manual', parent=base_parser)
  parser.set_author('Robert E. Smith (robert.smith@florey.edu.au) '
                    'and David Raffelt (david.raffelt@florey.edu.au)')
  parser.set_synopsis('Intensity normalise a DWI series based on the b=0 signal within a supplied mask')
  parser.add_argument('input_dwi',
                      'The input DWI series',
                      type=app.Parser.ImageIn())
  parser.add_argument('input_mask',
                      'The mask within which a reference b=0 intensity will be sampled',
                      type=app.Parser.ImageIn())
  parser.add_argument('output_dwi',
                      'The output intensity-normalised DWI series',
                      type=app.Parser.ImageOut())
  parser.add_option('intensity',
                    'Normalise the b=0 signal to a specified value',
                    type=app.Parser.Float(0.0),
                    default=DEFAULT_TARGET_INTENSITY)
  parser.add_option('percentile',
                    'Define the percentile of the b=0 image intensties within the mask used for normalisation; '
                    'if this option is not supplied then the median value (50th percentile) '
                    'will be normalised to the desired intensity value',
                    type=app.Parser.Float(0.0, 100.0))
  app.add_dwgrad_import_options(parser)



def execute(): #pylint: disable=unused-variable

  grad_option = app.dwgrad_import_options()
  if app.ARGS.percentile:
    intensities = [float(value) for value in run.command(['dwiextract', app.ARGS.input_dwi]
                                                         + grad_option
                                                         + ['-bzero', '-', '|',
                                                            'mrmath', '-', 'mean', '-', '-axis', '3', '|',
                                                            'mrdump', '-', '-mask', app.ARGS.input_mask],
                                                         preserve_pipes=True).stdout.splitlines()]
    intensities = sorted(intensities)
    float_index = 0.01 * app.ARGS.percentile * len(intensities)
    lower_index = int(math.floor(float_index))
    if app.ARGS.percentile == 100.0:
      reference_value = intensities[-1]
    else:
      interp_mu = float_index - float(lower_index)
      reference_value = (1.0-interp_mu)*intensities[lower_index] + interp_mu*intensities[lower_index+1]
  else:
    reference_value = float(run.command(['dwiextract', app.ARGS.input_dwi]
                                        + grad_option
                                        + ['-bzero', '-', '|',
                                           'mrmath', '-', 'mean', '-', '-axis', '3', '|',
                                           'mrstats', '-', '-mask', app.ARGS.input_mask, '-output', 'median'],
                                        preserve_pipes=True).stdout)
  multiplier = app.ARGS.intensity / reference_value

  run.command(['mrcalc', app.ARGS.input_dwi, str(multiplier), '-mult', '-', '|',
               'mrconvert', '-', app.ARGS.output_dwi]
              + grad_option,
              mrconvert_keyval=app.ARGS.input_dwi,
              force=app.FORCE_OVERWRITE,
              preserve_pipes=True)
