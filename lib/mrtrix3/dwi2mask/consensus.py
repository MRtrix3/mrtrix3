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

from mrtrix3 import CONFIG, MRtrixError
from mrtrix3 import algorithm, app, path, run

def usage(base_parser, subparsers): #pylint: disable=unused-variable
  parser = subparsers.add_parser('consensus', parents=[base_parser])
  parser.set_author('Robert E. Smith (robert.smith@florey.edu.au)')
  parser.set_synopsis('Generate a brain mask based on the consensus of all dwi2mask algorithms')
  parser.add_argument('input',  help='The input DWI series')
  parser.add_argument('output', help='The output mask image')
  options = parser.add_argument_group('Options specific to the "consensus" algorithm')
  options.add_argument('-algorithms', nargs='+', help='Provide a list of dwi2mask algorithms that are to be utilised')
  options.add_argument('-masks', help='Export a 4D image containing the individual algorithm masks')
  options.add_argument('-template', metavar='TemplateImage MaskImage', nargs=2, help='Provide a template image and corresponding mask for those algorithms requiring such')



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
  return False



def execute(): #pylint: disable=unused-variable
  if app.ARGS.masks:
    app.check_output_path(path.from_user(app.ARGS.masks, False))

  algorithm_list = [item for item in algorithm.get_list() if item != 'consensus']
  app.debug(str(algorithm_list))

  if app.ARGS.algorithms:
    if 'consensus' in app.ARGS.algorithms:
      raise MRtrixError('Cannot provide "consensus" in list of dwi2mask algorithms to utilise')
    invalid_algs = [entry for entry in app.ARGS.algorithms if entry not in algorithm_list]
    if invalid_algs:
      raise MRtrixError('Requested dwi2mask algorithm'
                        + ('s' if len(invalid_algs) > 1 else '')
                        + ' not available: '
                        + str(invalid_algs))
    algorithm_list = app.ARGS.algorithms

  # For "template" algorithm, can run twice with two different softwares
  # Ideally this would be determined based on the help page,
  #   rather than pre-programmed
  algorithm_list = [item for item in algorithm_list if item != 'template']
  algorithm_list.append('template -software ants')
  algorithm_list.append('template -software fsl')
  app.debug(str(algorithm_list))

  mask_list = []
  for alg in algorithm_list:
    alg_string = alg.replace(' -software ', '_')
    mask_path = alg_string + '.mif'
    cmd = 'dwi2mask ' + alg + ' input.mif ' + mask_path
    # Ideally this would be determined based on the presence of this option
    #   in the command's help page
    if any(item in alg for item in ['ants', 'template']):
      cmd += ' -template template_image.nii template_mask.nii'
    cmd += ' -scratch ' + app.SCRATCH_DIR
    if not app.DO_CLEANUP:
      cmd += ' -nocleanup'
    try:
      run.command(cmd)
      mask_list.append(mask_path)
    except run.MRtrixCmdError as e_dwi2mask:
      app.warn('"dwi2mask ' + alg + '" failed; will be omitted from consensus')
      app.debug(str(e_dwi2mask))
      with open('error_' + alg_string + '.txt', 'w') as f_error:
        f_error.write(str(e_dwi2mask))

  app.debug(str(mask_list))
  if not mask_list:
    raise MRtrixError('No dwi2mask algorithms were successful; cannot generate mask')
  if len(mask_list) == 1:
    app.warn('Only one dwi2mask algorithm was successful; output mask will be this result and not a consensus')
    if app.ARGS.masks:
      run.command('mrconvert ' + mask_list[0] + ' ' + path.from_user(app.ARGS.masks),
                  mrconvert_keyval=path.from_user(app.ARGS.input, False),
                  force=app.FORCE_OVERWRITE)
    return mask_list[0]
  final_mask = 'consensus.mif'
  app.console('Computing consensus from ' + str(len(mask_list)) + ' of ' + str(len(algorithm_list)) + ' algorithms')
  run.command(['mrcat', mask_list, '-axis', '3', 'all_masks.mif'])
  run.command('mrmath all_masks.mif mean - -axis 3 | '
              'mrthreshold - -abs 0.5 -comparison gt ' + final_mask)

  if app.ARGS.masks:
    run.command('mrconvert all_masks.mif ' + path.from_user(app.ARGS.masks),
                mrconvert_keyval=path.from_user(app.ARGS.input, False),
                force=app.FORCE_OVERWRITE)

  return final_mask
