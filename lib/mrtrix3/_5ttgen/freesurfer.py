# Copyright (c) 2008-2021 the MRtrix3 contributors.
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
  parser.add_argument('input',  help='The input FreeSurfer parcellation image (any image containing \'aseg\' in its name)')
  parser.add_argument('output', help='The output 5TT image')
  options = parser.add_argument_group('Options specific to the \'freesurfer\' algorithm')
  options.add_argument('-sclimbic', metavar=('image'), help='Incorporate results of subcortical limbic segmentation')



def check_output_paths(): #pylint: disable=unused-variable
  app.check_output_path(app.ARGS.output)



def get_inputs(): #pylint: disable=unused-variable
  # Don't bother converting into scratch directory, as it's only used once
  #run.command('mrconvert ' + path.from_user(app.ARGS.input) + ' ' + path.to_scratch('input.mif'))
  pass



def execute(): #pylint: disable=unused-variable
  freesurfer_home = os.environ.get('FREESURFER_HOME', '')
  if not freesurfer_home:
    raise MRtrixError('Environment variable FREESURFER_HOME is not set; please run appropriate FreeSurfer configuration script, set this variable manually, or provide script with path to file FreeSurferColorLUT.txt using -lut option')
  lut_input_path = os.path.join(freesurfer_home, 'FreeSurferColorLUT.txt')
  if not os.path.isfile(lut_input_path):
    raise MRtrixError('Could not find FreeSurfer lookup table file (expected location: ' + lut_input_path + ')')
  if app.ARGS.sclimbic:
    sclimbic_lut_input_path = os.path.join(freesurfer_home, 'models', 'sclimbic.ctab')
    if not os.path.isfile(sclimbic_lut_input_path):
      raise MRtrixError('Could not find FreeSurfer ScLimbic module lookup table file (expected location: ' + sclimbic_lut_input_path + ')')

  if app.ARGS.sgm_amyg_hipp:
    lut_output_file_name = 'FreeSurfer2ACT_sgm_amyg_hipp.txt'
  else:
    lut_output_file_name = 'FreeSurfer2ACT.txt'
  lut_output_path = os.path.join(path.shared_data_path(), path.script_subdir_name(), lut_output_file_name)
  if not os.path.isfile(lut_output_path):
    raise MRtrixError('Could not find lookup table file for converting FreeSurfer parcellation output to tissues (expected location: ' + lut_output_path + ')')
  if app.ARGS.sclimbic:
    sclimbic_lut_output_path = os.path.join(path.shared_data_path(), path.script_subdir_name(), 'ScLimbic2ACT.txt')
    if not os.path.isfile(sclimbic_lut_output_path):
      raise MRtrixError('Could not find lookup table file for converting ScLimbic module output to tissues (expected location: ' + sclimbic_lut_output_path + ')')

  # Initial conversion from FreeSurfer parcellation to five principal tissue types
  index_image = 'indices.mif'
  run.command(['labelconvert', path.from_user(app.ARGS.input, False), lut_input_path, lut_output_path, index_image])
  if app.ARGS.sclimbic:
    sclimbic_image = 'sclimbic.mif'
    run.command(['labelconvert', path.from_user(app.ARGS.sclimbic, False), sclimbic_lut_input_path, sclimbic_lut_output_path, sclimbic_image])

  # Crop to reduce file size
  if not app.ARGS.nocrop:
    cropped_index_image = 'indices_cropped.mif'
    if app.ARGS.sclimbic:
      crop_mask = 'crop_mask.mif'
      run.command('mrthreshold ' + index_image + ' -abs 0.5 ' + crop_mask)
      run.command('mrgrid ' + index_image + ' crop ' + cropped_index_image + ' -mask ' + crop_mask)
      cropped_sclimbic_image = 'sclimbic_cropped.mif'
      run.command('mrgrid ' + sclimbic_image + ' crop ' + cropped_sclimbic_image + ' -mask ' + crop_mask)
      sclimbic_image = cropped_sclimbic_image
    else:
      run.command('mrthreshold ' + index_image + ' - -abs 0.5 | mrgrid indices.mif crop ' + cropped_index_image + ' -mask -')
    index_image = cropped_index_image

  # Convert into the 5TT format for ACT
  if app.ARGS.sclimbic:
    for index in range(1, 6):
      run.command('mrcalc ' + sclimbic_image + ' 0 ' + index_image + ' -if ' + str(index) + ' -eq ' + sclimbic_image + ' ' + str(index) + ' -eq -add tissue' + str(index) + '.mif')
  else:
    for index in range(1, 6):
      run.command('mrcalc ' + index_image + ' ' + str(index) + ' -eq tissue' + str(index) + '.mif')

  run.command(['mrcat', ['tissue' + str(index) + '.mif' for index in range(1, 6)], '-', '-axis', '3', '|',
               'mrconvert', '-', 'result.mif', '-datatype', 'float32'])

  run.command('mrconvert result.mif ' + path.from_user(app.ARGS.output),
              mrconvert_keyval=path.from_user(app.ARGS.input, False),
              force=app.FORCE_OVERWRITE)
