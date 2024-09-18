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

import os.path, shutil
from mrtrix3 import MRtrixError
from mrtrix3 import app, path, run



def usage(base_parser, subparsers): #pylint: disable=unused-variable
  parser = subparsers.add_parser('freesurfer', parents=[base_parser])
  parser.set_author('Robert E. Smith (robert.smith@florey.edu.au)')
  parser.set_synopsis('Generate the 5TT image based on a FreeSurfer parcellation image')
  parser.add_argument('input',
                      type=app.Parser.ImageIn(),
                      help='The input FreeSurfer parcellation image '
                           '(any image containing "aseg" in its name)')
  parser.add_argument('output',
                      type=app.Parser.ImageOut(),
                      help='The output 5TT image')
  options = parser.add_argument_group('Options specific to the "freesurfer" algorithm')
  options.add_argument('-lut',
                       type=app.Parser.FileIn(),
                       help='Manually provide path to the lookup table on which the input parcellation image is based '
                            '(e.g. FreeSurferColorLUT.txt)')



def execute(): #pylint: disable=unused-variable
  run.command(['mrconvert', app.ARGS.input, 'input.mif'],
              preserve_pipes=True)
  lut_input_path = 'LUT.txt'
  if app.ARGS.lut:
    run.function(shutil.copyfile, app.ARGS.lut, lut_input_path)
  else:
    freesurfer_home = os.environ.get('FREESURFER_HOME', '')
    if not freesurfer_home:
      raise MRtrixError('Environment variable FREESURFER_HOME is not set; '
                        'please run appropriate FreeSurfer configuration script, '
                        'set this variable manually, '
                        'or provide script with path to file FreeSurferColorLUT.txt using -lut option')
    lut_input_path = os.path.join(freesurfer_home, 'FreeSurferColorLUT.txt')
    if not os.path.isfile(lut_input_path):
      raise MRtrixError(f'Could not find FreeSurfer lookup table file '
                        f'(expected location: {lut_input_path}), and none provided using -lut')

  if app.ARGS.sgm_amyg_hipp:
    lut_output_file_name = 'FreeSurfer2ACT_sgm_amyg_hipp.txt'
  else:
    lut_output_file_name = 'FreeSurfer2ACT.txt'
  lut_output_path = os.path.join(path.shared_data_path(), '5ttgen', lut_output_file_name)
  if not os.path.isfile(lut_output_path):
    raise MRtrixError(f'Could not find lookup table file for converting FreeSurfer parcellation output to tissues '
                      f'(expected location: {lut_output_path})')

  # Initial conversion from FreeSurfer parcellation to five principal tissue types
  run.command(f'labelconvert input.mif {lut_input_path} {lut_output_path} indices.mif')

  # Crop to reduce file size
  if app.ARGS.nocrop:
    image = 'indices.mif'
  else:
    image = 'indices_cropped.mif'
    run.command(f'mrthreshold indices.mif - -abs 0.5 | '
                f'mrgrid indices.mif crop {image} -mask -')

  # Convert into the 5TT format for ACT
  run.command(f'mrcalc {image} 1 -eq cgm.mif')
  run.command(f'mrcalc {image} 2 -eq sgm.mif')
  run.command(f'mrcalc {image} 3 -eq  wm.mif')
  run.command(f'mrcalc {image} 4 -eq csf.mif')
  run.command(f'mrcalc {image} 5 -eq path.mif')

  run.command('mrcat cgm.mif sgm.mif wm.mif csf.mif path.mif - -axis 3 | '
              'mrconvert - result.mif -datatype float32')

  run.command(['mrconvert', 'result.mif', app.ARGS.output],
              mrconvert_keyval=app.ARGS.input,
              force=app.FORCE_OVERWRITE,
              preserve_pipes=True)

  return 'result.mif'
