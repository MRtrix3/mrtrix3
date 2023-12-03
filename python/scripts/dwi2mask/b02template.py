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

import os, shutil
from mrtrix3 import CONFIG, MRtrixError
from mrtrix3 import app, fsl, path, run


SOFTWARES = ['antsfull', 'antsquick', 'fsl']
DEFAULT_SOFTWARE = 'antsquick'

ANTS_REGISTERFULL_CMD = 'antsRegistration'
ANTS_REGISTERQUICK_CMD = 'antsRegistrationSyNQuick.sh'
ANTS_TRANSFORM_CMD = 'antsApplyTransforms'


# From Tustison and Avants, Frontiers in Neuroinformatics 2013:
ANTS_REGISTERFULL_OPTIONS = \
  ' --use-histogram-matching 1' \
  ' --initial-moving-transform [template_image.nii,bzero.nii,1]' \
  ' --transform Rigid[0.1]' \
  ' --metric MI[template_image.nii,bzero.nii,1,32,Regular,0.25]' \
  ' --convergence 1000x500x250x100' \
  ' --smoothing-sigmas 3x2x1x0' \
  ' --shrink-factors 8x4x2x1' \
  ' --transform Affine[0.1]' \
  ' --metric MI[template_image.nii,bzero.nii,1,32,Regular,0.25]' \
  ' --convergence 1000x500x250x100' \
  ' --smoothing-sigmas 3x2x1x0' \
  ' --shrink-factors 8x4x2x1' \
  ' --transform BSplineSyN[0.1,26,0,3]' \
  ' --metric CC[template_image.nii,bzero.nii,1,4]' \
  ' --convergence 100x70x50x20' \
  ' --smoothing-sigmas 3x2x1x0' \
  ' --shrink-factors 6x4x2x1'

ANTS_REGISTERQUICK_OPTIONS = '-j 1'


def usage(base_parser, subparsers): #pylint: disable=unused-variable
  parser = subparsers.add_parser('b02template', parents=[base_parser])
  parser.set_author('Robert E. Smith (robert.smith@florey.edu.au)')
  parser.set_synopsis('Register the mean b=0 image to a T2-weighted template to back-propagate a brain mask')
  parser.add_description('This script currently assumes that the template image provided via the -template option '
                         'is T2-weighted, and can therefore be trivially registered to a mean b=0 image.')
  parser.add_description('Command-line option -ants_options can be used with either the "antsquick" or "antsfull" software options. '
                         'In both cases, image dimensionality is assumed to be 3, and so this should be omitted from the user-specified options.'
                         'The input can be either a string (encased in double-quotes if more than one option is specified), or a path to a text file containing the requested options. '
                         'In the case of the "antsfull" software option, one will require the names of the fixed and moving images that are provided to the antsRegistration command: these are "template_image.nii" and "bzero.nii" respectively.')
  parser.add_citation('M. Jenkinson, C.F. Beckmann, T.E. Behrens, M.W. Woolrich, S.M. Smith. FSL. NeuroImage, 62:782-90, 2012',
                      condition='If FSL software is used for registration',
                      is_external=True)
  parser.add_citation('B. Avants, N.J. Tustison, G. Song, P.A. Cook, A. Klein, J.C. Jee. A reproducible evaluation of ANTs similarity metric performance in brain image registration. NeuroImage, 2011, 54, 2033-2044',
                      condition='If ANTs software is used for registration',
                      is_external=True)
  parser.add_argument('input',  help='The input DWI series')
  parser.add_argument('output', help='The output mask image')
  options = parser.add_argument_group('Options specific to the "template" algorithm')
  options.add_argument('-software', choices=SOFTWARES, help='The software to use for template registration; options are: ' + ','.join(SOFTWARES) + '; default is ' + DEFAULT_SOFTWARE)
  options.add_argument('-template', metavar='TemplateImage MaskImage', nargs=2, help='Provide the template image to which the input data will be registered, and the mask to be projected to the input image. The template image should be T2-weighted.')
  ants_options = parser.add_argument_group('Options applicable when using the ANTs software for registration')
  ants_options.add_argument('-ants_options', help='Provide options to be passed to the ANTs registration command (see Description)')
  fsl_options = parser.add_argument_group('Options applicable when using the FSL software for registration')
  fsl_options.add_argument('-flirt_options', metavar='" FlirtOptions"', help='Command-line options to pass to the FSL flirt command (provide a string within quotation marks that contains at least one space, even if only passing a single command-line option to flirt)')
  fsl_options.add_argument('-fnirt_config', metavar='FILE', help='Specify a FNIRT configuration file for registration')



def get_inputs(): #pylint: disable=unused-variable

  reg_software = app.ARGS.software if app.ARGS.software else CONFIG.get('Dwi2maskTemplateSoftware', DEFAULT_SOFTWARE)
  if reg_software.startswith('ants'):
    if not os.environ.get('ANTSPATH', ''):
      raise MRtrixError('Environment variable ANTSPATH is not set; '
                        'please appropriately configure ANTs software')
    if app.ARGS.ants_options:
      if os.path.isfile(path.from_user(app.ARGS.ants_options, False)):
        run.function(shutil.copyfile, path.from_user(app.ARGS.ants_options, False), path.to_scratch('ants_options.txt', False))
  elif reg_software == 'fsl':
    fsl_path = os.environ.get('FSLDIR', '')
    if not fsl_path:
      raise MRtrixError('Environment variable FSLDIR is not set; '
                        'please run appropriate FSL configuration script')
    if app.ARGS.fnirt_config:
      fnirt_config = path.from_user(app.ARGS.fnirt_config, False)
      if not os.path.isfile(fnirt_config):
        raise MRtrixError('No file found at -fnirt_config location "' + fnirt_config + '"')
    elif 'Dwi2maskTemplateFSLFnirtConfig' in CONFIG:
      fnirt_config = CONFIG['Dwi2maskTemplateFSLFnirtConfig']
      if not os.path.isfile(fnirt_config):
        raise MRtrixError('No file found at config file entry "Dwi2maskTemplateFSLFnirtConfig" location "' + fnirt_config + '"')
    else:
      fnirt_config = None
    if fnirt_config:
      run.function(shutil.copyfile, fnirt_config, path.to_scratch('fnirt_config.cnf', False))
  else:
    assert False

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

  reg_software = app.ARGS.software if app.ARGS.software else CONFIG.get('Dwi2maskTemplateSoftware', DEFAULT_SOFTWARE)

  if reg_software.startswith('ants'):

    def check_ants_executable(cmdname):
      if not shutil.which(cmdname):
        raise MRtrixError('Unable to find ANTs command "' + cmdname + '"; please check ANTs installation')
    check_ants_executable(ANTS_REGISTERFULL_CMD if reg_software == 'antsfull' else ANTS_REGISTERQUICK_CMD)
    check_ants_executable(ANTS_TRANSFORM_CMD)

    if app.ARGS.ants_options:
      if os.path.isfile('ants_options.txt'):
        with open('ants_options.txt', 'r', encoding='utf-8') as ants_options_file:
          ants_options = ants_options_file.readlines()
        ants_options = ' '.join(line.lstrip().rstrip('\n \\') for line in ants_options if line.strip() and not line.lstrip()[0] == '#')
      else:
        ants_options = app.ARGS.ants_options
    else:
      if reg_software == 'antsfull':
        ants_options = CONFIG.get('Dwi2maskTemplateANTsFullOptions', ANTS_REGISTERFULL_OPTIONS)
      elif reg_software == 'antsquick':
        ants_options = CONFIG.get('Dwi2maskTemplateANTsQuickOptions', ANTS_REGISTERQUICK_OPTIONS)

    # Use ANTs SyN for registration to template
    if reg_software == 'antsfull':
      run.command(ANTS_REGISTERFULL_CMD
                  + ' --dimensionality 3'
                  + ' --output ANTS'
                  + ' '
                  + ants_options)
      ants_options_split = ants_options.split()
      nonlinear = any(i for i in range(0, len(ants_options_split)-1)
                      if ants_options_split[i] == '--transform'
                      and not any(item in ants_options_split[i+1] for item in ['Rigid', 'Affine', 'Translation']))
    else:
      # Use ANTs SyNQuick for registration to template
      run.command(ANTS_REGISTERQUICK_CMD
                  + ' -d 3'
                  + ' -f template_image.nii'
                  + ' -m bzero.nii'
                  + ' -o ANTS'
                  + ' '
                  + ants_options)
      ants_options_split = ants_options.split()
      nonlinear = not [i for i in range(0, len(ants_options_split)-1)
                       if ants_options_split[i] == '-t'
                       and ants_options_split[i+1] in ['t', 'r', 'a']]

    transformed_path = 'transformed.nii'
    # Note: Don't use nearest-neighbour interpolation;
    #   allow "partial volume fractions" in output, and threshold later
    run.command(ANTS_TRANSFORM_CMD
                + ' -d 3'
                + ' -i template_mask.nii'
                + ' -o ' + transformed_path
                + ' -r bzero.nii'
                + ' -t [ANTS0GenericAffine.mat,1]'
                + (' -t ANTS1InverseWarp.nii.gz' if nonlinear else ''))

  elif reg_software == 'fsl':

    flirt_cmd = fsl.exe_name('flirt')
    fnirt_cmd = fsl.exe_name('fnirt')
    invwarp_cmd = fsl.exe_name('invwarp')
    applywarp_cmd = fsl.exe_name('applywarp')

    flirt_options = app.ARGS.flirt_options \
                    if app.ARGS.flirt_options \
                    else CONFIG.get('Dwi2maskTemplateFSLFlirtOptions', '-dof 12')

    # Initial affine registration to template
    run.command(flirt_cmd
                + ' -ref template_image.nii'
                + ' -in bzero.nii'
                + ' -omat bzero_to_template.mat'
                + ' '
                + flirt_options
                + (' -v' if app.VERBOSITY >= 3 else ''))

    # Produce dilated template mask image, so that registration is not
    #   too influenced by effects at the edge of the processing mask
    run.command('maskfilter template_mask.nii dilate - -npass 3 | '
                'mrconvert - template_mask_dilated.nii -datatype uint8')

    # Non-linear registration to template
    if os.path.isfile('fnirt_config.cnf'):
      fnirt_config_option = ' --config=fnirt_config'
    else:
      fnirt_config_option = ''
      app.console('No config file provided for FSL fnirt; it will use its internal defaults')
    run.command(fnirt_cmd
                + fnirt_config_option
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
