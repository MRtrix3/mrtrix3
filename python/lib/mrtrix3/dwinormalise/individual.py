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

import math
from mrtrix3 import MRtrixError
from mrtrix3 import app, path, run


DEFAULT_TARGET_INTENSITY=1000



def usage(base_parser, subparsers): #pylint: disable=unused-variable
  parser = subparsers.add_parser('individual', parents=[base_parser])
  parser.set_author('Robert E. Smith (robert.smith@florey.edu.au) and David Raffelt (david.raffelt@florey.edu.au)')
  parser.set_synopsis('Intensity normalise a DWI series based on the b=0 signal within a supplied mask')
  parser.add_argument('input_dwi', help='The input DWI series')
  parser.add_argument('input_mask', help='The mask within which a reference b=0 intensity will be sampled')
  parser.add_argument('output_dwi', help='The output intensity-normalised DWI series')
  parser.add_argument('-intensity', type=float, default=DEFAULT_TARGET_INTENSITY, help='Normalise the b=0 signal to a specified value (Default: ' + str(DEFAULT_TARGET_INTENSITY) + ')')
  parser.add_argument('-percentile', type=int, help='Define the percentile of the b=0 image intensties within the mask used for normalisation; if this option is not supplied then the median value (50th percentile) will be normalised to the desired intensity value')
  app.add_dwgrad_import_options(parser)



def check_output_paths(): #pylint: disable=unused-variable
  app.check_output_path(app.ARGS.output_dwi)



def execute(): #pylint: disable=unused-variable

  grad_option = ''
  if app.ARGS.grad:
    grad_option = ' -grad ' + path.from_user(app.ARGS.grad)
  elif app.ARGS.fslgrad:
    grad_option = ' -fslgrad ' + path.from_user(app.ARGS.fslgrad[0]) + ' ' + path.from_user(app.ARGS.fslgrad[1])

  if app.ARGS.percentile:
    if app.ARGS.percentile < 0.0 or app.ARGS.percentile > 100.0:
      raise MRtrixError('-percentile value must be between 0 and 100')
    intensities = [float(value) for value in run.command('dwiextract ' + path.from_user(app.ARGS.input_dwi) + grad_option + ' -bzero - | ' + \
                                                         'mrmath - mean - -axis 3 | ' + \
                                                         'mrdump - -mask ' + path.from_user(app.ARGS.input_mask)).stdout.splitlines()]
    intensities = sorted(intensities)
    float_index = 0.01 * app.ARGS.percentile * len(intensities)
    lower_index = int(math.floor(float_index))
    if app.ARGS.percentile == 100.0:
      reference_value = intensities[-1]
    else:
      interp_mu = float_index - float(lower_index)
      reference_value = (1.0-interp_mu)*intensities[lower_index] + interp_mu*intensities[lower_index+1]
  else:
    reference_value = float(run.command('dwiextract ' + path.from_user(app.ARGS.input_dwi) + grad_option + ' -bzero - | ' + \
                                        'mrmath - mean - -axis 3 | ' + \
                                        'mrstats - -mask ' + path.from_user(app.ARGS.input_mask) + ' -output median').stdout)
  multiplier = app.ARGS.intensity / reference_value

  run.command('mrcalc ' + path.from_user(app.ARGS.input_dwi) + ' ' + str(multiplier) + ' -mult - | ' + \
              'mrconvert - ' + path.from_user(app.ARGS.output_dwi) + grad_option, \
              mrconvert_keyval=path.from_user(app.ARGS.input_dwi, False), \
              force=app.FORCE_OVERWRITE)
