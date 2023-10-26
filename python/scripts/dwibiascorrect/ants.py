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
from mrtrix3 import app, path, run



OPT_N4_BIAS_FIELD_CORRECTION = {
    's': ('4','shrink-factor applied to spatial dimensions'),
    'b':('[100,3]','[initial mesh resolution in mm, spline order] This value is optimised for human adult data and needs to be adjusted for rodent data.'),
    'c':('[1000,0.0]', '[numberOfIterations,convergenceThreshold]')}



def usage(base_parser, subparsers): #pylint: disable=unused-variable
  parser = subparsers.add_parser('ants', parents=[base_parser])
  parser.set_author('Robert E. Smith (robert.smith@florey.edu.au)')
  parser.set_synopsis('Perform DWI bias field correction using the N4 algorithm as provided in ANTs')
  parser.add_citation('Tustison, N.; Avants, B.; Cook, P.; Zheng, Y.; Egan, A.; Yushkevich, P. & Gee, J. N4ITK: Improved N3 Bias Correction. IEEE Transactions on Medical Imaging, 2010, 29, 1310-1320', is_external=True)
  ants_options = parser.add_argument_group('Options for ANTs N4BiasFieldCorrection command')
  for key in sorted(OPT_N4_BIAS_FIELD_CORRECTION):
    ants_options.add_argument('-ants.'+key, metavar=OPT_N4_BIAS_FIELD_CORRECTION[key][0], help='N4BiasFieldCorrection option -%s. %s' % (key,OPT_N4_BIAS_FIELD_CORRECTION[key][1]))
  parser.add_argument('input',  help='The input image series to be corrected')
  parser.add_argument('output', help='The output corrected image series')



def check_output_paths(): #pylint: disable=unused-variable
  pass



def get_inputs(): #pylint: disable=unused-variable
  pass



def execute(): #pylint: disable=unused-variable
  if not shutil.which('N4BiasFieldCorrection'):
    raise MRtrixError('Could not find ANTS program N4BiasFieldCorrection; please check installation')

  for key in sorted(OPT_N4_BIAS_FIELD_CORRECTION):
    if hasattr(app.ARGS, 'ants.' + key):
      val = getattr(app.ARGS, 'ants.' + key)
      if val is not None:
        OPT_N4_BIAS_FIELD_CORRECTION[key] = (val, 'user defined')
  ants_options = ' '.join(['-%s %s' %(k, v[0]) for k, v in OPT_N4_BIAS_FIELD_CORRECTION.items()])

  # Generate a mean b=0 image
  run.command('dwiextract in.mif - -bzero | mrmath - mean mean_bzero.mif -axis 3')

  # Use the brain mask as a weights image rather than a mask; means that voxels at the edge of the mask
  #   will have a smoothly-varying bias field correction applied, rather than multiplying by 1.0 outside the mask
  run.command('mrconvert mean_bzero.mif mean_bzero.nii -strides +1,+2,+3')
  run.command('mrconvert mask.mif mask.nii -strides +1,+2,+3')
  init_bias_path = 'init_bias.nii'
  corrected_path = 'corrected.nii'
  run.command('N4BiasFieldCorrection -d 3 -i mean_bzero.nii -w mask.nii -o [' + corrected_path + ',' + init_bias_path + '] ' + ants_options)

  # N4 can introduce large differences between subjects via a global scaling of the bias field
  # Estimate this scaling based on the total integral of the pre- and post-correction images within the brain mask
  input_integral  = float(run.command('mrcalc mean_bzero.mif mask.mif -mult - | mrmath - sum - -axis 0 | mrmath - sum - -axis 1 | mrmath - sum - -axis 2 | mrdump -').stdout)
  output_integral = float(run.command('mrcalc ' + corrected_path + ' mask.mif -mult - | mrmath - sum - -axis 0 | mrmath - sum - -axis 1 | mrmath - sum - -axis 2 | mrdump -').stdout)
  multiplier = output_integral / input_integral
  app.debug('Integrals: Input = ' + str(input_integral) + '; Output = ' + str(output_integral) + '; resulting multiplier = ' + str(multiplier))
  run.command('mrcalc ' + init_bias_path + ' ' + str(multiplier) + ' -mult bias.mif')

  # Common final steps for all algorithms
  run.command('mrcalc in.mif bias.mif -div result.mif')
  run.command('mrconvert result.mif ' + path.from_user(app.ARGS.output), mrconvert_keyval=path.from_user(app.ARGS.input, False), force=app.FORCE_OVERWRITE)
  if app.ARGS.bias:
    run.command('mrconvert bias.mif ' + path.from_user(app.ARGS.bias), mrconvert_keyval=path.from_user(app.ARGS.input, False), force=app.FORCE_OVERWRITE)
