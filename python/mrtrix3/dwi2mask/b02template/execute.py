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
from mrtrix3 import app, fsl, run
from . import ANTS_REGISTERFULL_CMD, ANTS_REGISTERQUICK_CMD, ANTS_TRANSFORM_CMD
from . import ANTS_REGISTERFULL_OPTIONS, ANTS_REGISTERQUICK_OPTIONS
from . import DEFAULT_SOFTWARE

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
