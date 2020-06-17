# Copyright (c) 2008-2019 the MRtrix3 contributors.
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
from mrtrix3 import MRtrixError
from mrtrix3 import app, fsl, image, path, run

SOFTWARES = ['ants', 'fsl']

def usage(base_parser, subparsers): #pylint: disable=unused-variable
  parser = subparsers.add_parser('template', parents=[base_parser])
  parser.set_author('Robert E. Smith (robert.smith@florey.edu.au)')
  parser.set_synopsis('Register the mean b=0 image to a T2-weighted template to back-propagate a brain mask')
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
  pass



def execute(): #pylint: disable=unused-variable

  # TODO What image to generate here depends on the template:
  # - If a good T2-weighted template is found, use the mean b=0 image
  # - If a T1-weighted template is to be used, can't use histogram
  #   matching (relies on a good brain mask), and can't use MT-CSD
  #   pseudo-T1 generation. Instead consider APM?
  #   APM doesn't seem to do particularly well for unmasked data...
  # - Could use FA or MD templates from FSL / HCP? Might also struggle
  #   with non-brain data

  if not app.ARGS.template:
    raise MRtrixError('For "template" dwi2mask algorithm, '
                      '-template command-line option is currently mandatory')

  reg_software = app.ARGS.software if app.ARGS.software else 'fsl'
  if reg_software == 'ants':
    ants_path = os.environ.get('ANTSPATH', '')
    if not ants_path:
      raise MRtrixError('Environment variable ANTSPATH is not set; '
                        'please appropriately confirure ANTs software')
    ants_brain_extraction_cmd = find_executable('antsBrainExtraction.sh')
    if not ants_brain_extraction_cmd:
      raise MRtrixError('Unable to find command "'
                        + ants_brain_extraction_cmd
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
    # fnirt_config_basename = 'T1_2_MNI152_2mm.cnf'
    # fnirt_config_path = os.path.join(fsl_path, 'etc', 'flirtsch', fnirt_config_basename)
    # if not os.path.isfile(fnirt_config_path):
    #   raise MRtrixError('Unable to find configuration file for FNI FNIRT '
    #                     + '(expected location: ' + fnirt_config_path + ')')
  else:
    assert False

  # Produce mean b=0 image
  run.command('dwiextract input.mif -bzero - | '
              'mrmath - mean - -axis 3 | '
              'mrconvert - bzero.nii -strides +1,+2,+3')

  if reg_software == 'ants':

    run.command(ants_brain_extraction_cmd
                + ' -d 3'
                + ' -c 3x3x2x1'
                + ' -a bzero.nii'
                + ' -e ' + app.ARGS.template[0]
                + ' -m ' + app.ARGS.template[1]
                + ' -o mask.nii'
                + ('' if app.DO_CLEANUP else ' -k 1')
                + (' -z' if app.VERBOSITY >= 3 else ''))

    mask_path = 'mask.nii'

  elif reg_software == 'fsl':

    # Initial affine registration to template
    run.command(flirt_cmd
                + ' -ref ' + app.ARGS.template[0]
                + ' -in bzero.nii'
                + ' -omat bzero_to_template.mat'
                + ' -dof 12'
                + (' -v' if app.VERBOSITY >= 3 else ''))

    # Non-linear registration to template
    # Note: Unmasked
    run.command(fnirt_cmd
                #+ ' --config=' + fnirt_config_path
                + ' --ref=' + app.ARGS.template[0]
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
                + ' --in=' + app.ARGS.template[1]
                + ' --warp=' + invwarp_output_path
                + ' --out=mask.nii')
    mask_path = fsl.find_image('mask.nii')

  else:
    assert False

  # Instead of neaerest-neighbour interpolation during transformation,
  #   apply a threshold of 0.5 at this point
  run.command('mrthreshold '
              + mask_path
              + ' mask.mif -abs 0.5')

  # Make relative strides of three spatial axes of output mask equivalent
  #   to input DWI; this may involve decrementing magnitude of stride
  #   if the input DWI is volume-contiguous
  strides = image.Header('input.mif').strides()[0:3]
  strides = [(abs(value) + 1 - min(abs(v) for v in strides)) * (-1 if value < 0 else 1) for value in strides]

  run.command('mrconvert mask.mif '
              + path.from_user(app.ARGS.output)
              + ' -strides ' + ','.join(str(value) for value in strides),
              mrconvert_keyval=path.from_user(app.ARGS.input, False),
              force=app.FORCE_OVERWRITE)
