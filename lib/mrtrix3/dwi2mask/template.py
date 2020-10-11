# Copyright (c) 2008-2020 the MRtrix3 contributors.
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
from distutils.spawn import find_executable
from mrtrix3 import CONFIG, MRtrixError
from mrtrix3 import app, fsl, path, run


SOFTWARES = ['antsfull', 'antsquick', 'fsl']

ANTS_REGISTERFULL_CMD = 'antsRegistration'
ANTS_REGISTERQUICK_CMD = 'antsRegistrationSyNQuick.sh'
ANTS_TRANSFORM_CMD = 'antsApplyTransforms'


def usage(base_parser, subparsers): #pylint: disable=unused-variable
  parser = subparsers.add_parser('template', parents=[base_parser])
  parser.set_author('Robert E. Smith (robert.smith@florey.edu.au)')
  parser.set_synopsis('Register the mean b=0 image to a T2-weighted template to back-propagate a brain mask')
  parser.add_description('This script currently assumes that the template image provided via the -template option '
                         'is T2-weighted, and can therefore be trivially registered to a mean b=0 image.')
  parser.add_citation('M. Jenkinson, C.F. Beckmann, T.E. Behrens, M.W. Woolrich, S.M. Smith. FSL. NeuroImage, 62:782-90, 2012',
                      condition='If FSL software is used for registration',
                      is_external=True)
  parser.add_citation('B. Avants, N.J. Tustison, G. Song, P.A. Cook, A. Klein, J.C. Jee. A reproducible evaluation of ANTs similarity metric performance in brain image registration. NeuroImage, 2011, 54, 2033-2044',
                      condition='If ANTs software is used for registration',
                      is_external=True)
  parser.add_argument('input',  help='The input DWI series')
  parser.add_argument('output', help='The output mask image')
  options = parser.add_argument_group('Options specific to the "template" algorithm')
  options.add_argument('-software', choices=SOFTWARES, help='The software to use for template registration; options are: ' + ','.join(SOFTWARES) + '; default is fsl')
  options.add_argument('-template', metavar='TemplateImage MaskImage', nargs=2, help='Provide the template image to which the input data will be registered, and the mask to be projected to the input image. The template image should be T2-weighted.')



def get_inputs(): #pylint: disable=unused-variable
  if app.ARGS.template:
    run.command('mrconvert ' + app.ARGS.template[0] + ' ' + path.to_scratch('template_image.nii')
                + ' -strides +1,+2,+3')
    run.command('mrconvert ' + app.ARGS.template[1] + ' ' + path.to_scratch('template_mask.nii')
                + ' -strides +1,+2,+3 -datatype uint8')
  elif all(item in CONFIG for item in ['Dwi2maskTemplateImage', 'Dwi2maskTemplateMask']):
    run.command('mrconvert ' + CONFIG['Dwi2maskTemplateImage'] + ' ' + path.to_scratch('template_image.nii')
                + ' -strides +1,+2,+3')
    run.command('mrconvert ' + CONFIG['Dwi2maskTemplateMask'] + ' ' + path.to_scratch('template_mask.nii')
                + ' -strides +1,+2,+3 -datatype uint8')
  else:
    raise MRtrixError('No template image information available from '
                      'either command-line or MRtrix configuration file(s)')



def needs_mean_bzero(): #pylint: disable=unused-variable
  return True



def execute(): #pylint: disable=unused-variable
  # What image to generate here depends on the template:
  # - If a good T2-weighted template is found, use the mean b=0 image
  # - If a T1-weighted template is to be used, can't use histogram
  #   matching (relies on a good brain mask), and can't use MT-CSD
  #   pseudo-T1 generation. Instead consider APM?
  #   APM doesn't seem to do particularly well for unmasked data...
  # - Could use FA or MD templates from FSL / HCP? Might also struggle
  #   with non-brain data
  #
  # For now, script assumes T2-weighted template image.

  reg_software = app.ARGS.software if app.ARGS.software else CONFIG.get('Dwi2maskTemplateSoftware', 'fsl')
  if reg_software.startswith('ants'):
    ants_path = os.environ.get('ANTSPATH', '')
    if not ants_path:
      raise MRtrixError('Environment variable ANTSPATH is not set; '
                        'please appropriately confirure ANTs software')
    if not find_executable(ANTS_TRANSFORM_CMD):
      raise MRtrixError('Unable to find command "'
                        + ANTS_TRANSFORM_CMD
                        + '"; please check ANTs installation')
    ants_register_cmd = ANTS_REGISTERFULL_CMD if reg_software == 'antsfull' else ANTS_REGISTERQUICK_CMD
    if not find_executable(ants_register_cmd):
      raise MRtrixError('Unable to find command "'
                        + ants_register_cmd
                        + '"; please check ANTs installation')
  elif reg_software == 'fsl':
    fsl_path = os.environ.get('FSLDIR', '')
    if not fsl_path:
      raise MRtrixError('Environment variable FSLDIR is not set; '
                        'please run appropriate FSL configuration script')
    flirt_cmd = fsl.exe_name('flirt')
    fnirt_cmd = fsl.exe_name('fnirt')
    invwarp_cmd = fsl.exe_name('invwarp')
    applywarp_cmd = fsl.exe_name('applywarp')
    fnirt_config_basename = 'T1_2_MNI152_2mm.cnf'
    fnirt_config_path = os.path.join(fsl_path, 'etc', 'flirtsch', fnirt_config_basename)
    if not os.path.isfile(fnirt_config_path):
      raise MRtrixError('Unable to find configuration file for FNI FNIRT '
                        + '(expected location: ' + fnirt_config_path + ')')
  else:
    assert False

  if reg_software.startswith('ants'):

    # Use ANTs SyN for registration to template
    # From Klein and Avants, Frontiers in Neuroinformatics 2013:
    if reg_software == 'antsfull':
      run.command(ANTS_REGISTERFULL_CMD
                  + ' --dimensionality 3'
                  + ' --output ANTS'
                  + ' --use-histogram-matching 1'
                  + ' --initial-moving-transform [template_image.nii,template_image.nii,1]'
                  + ' --transform Rigid[0.1]'
                  + ' --metric MI[template_image.nii,bzero.nii,1,32,Regular,0.25]'
                  + ' --convergence 1000x500x250x100'
                  + ' --smoothing-sigmas 3x2x1x0'
                  + ' --shrink-factors 8x4x2x1'
                  + ' --transform Affine[0.1]'
                  + ' --metric MI[template_image.nii,bzero.nii,1,32,Regular,0.25]'
                  + ' --convergence 1000x500x250x100'
                  + ' --smoothing-sigmas 3x2x1x0'
                  + ' --shrink-factors 8x4x2x1'
                  + ' --transform BSplineSyN[0.1,26,0,3]'
                  + ' --metric CC[template_image.nii,bzero.nii,1,4]'
                  + ' --convergence 100x70x50x20'
                  + ' --smoothing-sigmas 3x2x1x0'
                  + ' --shrink-factors 6x4x2x1')

    else:
      # Use ANTs SyNQuick for registration to template
      run.command(ANTS_REGISTERQUICK_CMD
                  + ' -d 3'
                  + ' -f template_image.nii'
                  + ' -m bzero.nii'
                  + ' -o ANTS')

    transformed_path = 'transformed.nii'
    # Note: Don't use nearest-neighbour interpolation;
    #   allow "partial volume fractions" in output, and threshold later
    run.command(ANTS_TRANSFORM_CMD
                + ' -d 3'
                + ' -i template_mask.nii'
                + ' -o ' + transformed_path
                + ' -r bzero.nii'
                + ' -t [ANTS0GenericAffine.mat,1]'
                + ' -t ANTS1InverseWarp.nii.gz')

  elif reg_software == 'fsl':

    # Initial affine registration to template
    run.command(flirt_cmd
                + ' -ref template_image.nii'
                + ' -in bzero.nii'
                + ' -omat bzero_to_template.mat'
                + ' -dof 12'
                + (' -v' if app.VERBOSITY >= 3 else ''))

    # Produce dilated template mask image, so that registration is not
    #   too influenced by effects at the edge of the processing mask
    run.command('maskfilter template_mask.nii dilate - -npass 3 | '
                'mrconvert - template_mask_dilated.nii -datatype uint8')

    # Non-linear registration to template
    run.command(fnirt_cmd
                + ' --config=' + os.path.splitext(os.path.basename(fnirt_config_path))[0]
                + ' --ref=template_image.nii'
                + ' --refmask=template_mask_dilated.nii'
                + ' --in=bzero.nii'
                + ' --aff=bzero_to_template.mat'
                + ' --cout=bzero_to_template.nii'
                + (' --verbose' if app.VERBOSITY >= 3 else ''))
    fnirt_output_path = fsl.find_image('bzero_to_template')

    # Invert non-linear warp from subject->template to template->subject
    run.command(invwarp_cmd
                + ' --ref=bzero.nii'
                + ' --warp=' + fnirt_output_path
                + ' --out=template_to_bzero.nii')
    invwarp_output_path = fsl.find_image('template_to_bzero')

    # Transform mask image from template to subject
    # Note: Don't use nearest-neighbour interpolation;
    #   allow "partial volume fractions" in output, and threshold later
    run.command(applywarp_cmd
                + ' --ref=bzero.nii'
                + ' --in=template_mask.nii'
                + ' --warp=' + invwarp_output_path
                + ' --out=transformed.nii')
    transformed_path = fsl.find_image('transformed.nii')


  else:
    assert False

  # Instead of neaerest-neighbour interpolation during transformation,
  #   apply a threshold of 0.5 at this point
  run.command('mrthreshold '
              + transformed_path
              + ' mask.mif -abs 0.5')
  return 'mask.mif'
