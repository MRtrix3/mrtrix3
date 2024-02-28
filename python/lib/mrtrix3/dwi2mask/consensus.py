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

from mrtrix3 import CONFIG, MRtrixError
from mrtrix3 import algorithm, app, run

NEEDS_MEAN_BZERO = False # pylint: disable=unused-variable
DEFAULT_THRESHOLD = 0.501

def usage(base_parser, subparsers): #pylint: disable=unused-variable
  parser = subparsers.add_parser('consensus', parents=[base_parser])
  parser.set_author('Robert E. Smith (robert.smith@florey.edu.au)')
  parser.set_synopsis('Generate a brain mask based on the consensus of all dwi2mask algorithms')
  parser.add_argument('input',
                      type=app.Parser.ImageIn(),
                      help='The input DWI series')
  parser.add_argument('output',
                      type=app.Parser.ImageOut(),
                      help='The output mask image')
  options = parser.add_argument_group('Options specific to the "consensus" algorithm')
  options.add_argument('-algorithms',
                       type=str,
                       nargs='+',
                       help='Provide a (space- or comma-separated) list of dwi2mask algorithms that are to be utilised')
  options.add_argument('-masks',
                       type=app.Parser.ImageOut(),
                       metavar='image',
                       help='Export a 4D image containing the individual algorithm masks')
  options.add_argument('-template',
                       type=app.Parser.ImageIn(),
                       metavar=('TemplateImage', 'MaskImage'),
                       nargs=2,
                       help='Provide a template image and corresponding mask for those algorithms requiring such')
  options.add_argument('-threshold',
                       type=app.Parser.Float(0.0, 1.0),
                       default=DEFAULT_THRESHOLD,
                       help='The fraction of algorithms that must include a voxel for that voxel to be present in the final mask '
                            f'(default: {DEFAULT_THRESHOLD})')



def execute(): #pylint: disable=unused-variable

  algorithm_list = [item for item in algorithm.get_list() if item != 'consensus']
  app.debug(str(algorithm_list))

  if app.ARGS.algorithms:
    user_algorithms = app.ARGS.algorithms
    if len(user_algorithms) == 1:
      user_algorithms = user_algorithms[0].split(',')
    if 'consensus' in user_algorithms:
      raise MRtrixError('Cannot provide "consensus" in list of dwi2mask algorithms to utilise')
    invalid_algs = [entry for entry in user_algorithms if entry not in algorithm_list]
    if invalid_algs:
      raise MRtrixError(f'Requested dwi2mask {"algorithms" if len(invalid_algs) > 1 else "algorithm"} not available: '
                        f'{invalid_algs}')
    algorithm_list = user_algorithms

  # For "template" algorithm, can run twice with two different softwares
  # Ideally this would be determined based on the help page,
  #   rather than pre-programmed
  # Don't use "-software antsquick"; we're assuming that "antsfull" is superior
  if 'b02template' in algorithm_list:
    algorithm_list = [item for item in algorithm_list if item != 'b02template']
    algorithm_list.append('b02template -software antsfull')
    algorithm_list.append('b02template -software fsl')
  app.debug(str(algorithm_list))

  if any(any(item in alg for item in ('ants', 'b02template')) for alg in algorithm_list):
    if app.ARGS.template:
      run.command(['mrconvert', app.ARGS.template[0], 'template_image.nii',
                  '-strides', '+1,+2,+3'])
      run.command(['mrconvert', app.ARGS.template[1], 'template_mask.nii',
                  '-strides', '+1,+2,+3', '-datatype', 'uint8'])
    elif all(item in CONFIG for item in ['Dwi2maskTemplateImage', 'Dwi2maskTemplateMask']):
      run.command(['mrconvert', CONFIG['Dwi2maskTemplateImage'], 'template_image.nii',
                  '-strides', '+1,+2,+3'])
      run.command(['mrconvert', CONFIG['Dwi2maskTemplateMask'], 'template_mask.nii',
                  '-strides', '+1,+2,+3', '-datatype', 'uint8'])
    else:
      raise MRtrixError('Cannot include within consensus algorithms that necessitate use of a template image '
                      'if no template image is provided via command-line or configuration file')

  mask_list = []
  for alg in algorithm_list:
    alg_string = alg.replace(' -software ', '_')
    mask_path = f'{alg_string}.mif'
    cmd = f'dwi2mask {alg} input.mif {mask_path}'
    # Ideally this would be determined based on the presence of this option
    #   in the command's help page
    if any(item in alg for item in ['ants', 'b02template']):
      cmd += ' -template template_image.nii template_mask.nii'
    cmd += f' -scratch {app.SCRATCH_DIR}'
    if not app.DO_CLEANUP:
      cmd += ' -nocleanup'
    try:
      run.command(cmd)
      mask_list.append(mask_path)
    except run.MRtrixCmdError as e_dwi2mask:
      app.warn('"dwi2mask ' + alg + '" failed; will be omitted from consensus')
      app.debug(str(e_dwi2mask))
      with open(f'error_{alg_string}.txt', 'w', encoding='utf-8') as f_error:
        f_error.write(str(e_dwi2mask) + '\n')

  app.debug(str(mask_list))
  if not mask_list:
    raise MRtrixError('No dwi2mask algorithms were successful; cannot generate mask')
  if len(mask_list) == 1:
    app.warn('Only one dwi2mask algorithm was successful; '
             'output mask will be this result and not a "consensus"')
    if app.ARGS.masks:
      run.command(['mrconvert', mask_list[0], app.ARGS.masks],
                  mrconvert_keyval=app.ARGS.input,
                  force=app.FORCE_OVERWRITE)
    return mask_list[0]
  final_mask = 'consensus.mif'
  app.console(f'Computing consensus from {len(mask_list)} of {len(algorithm_list)} algorithms')
  run.command(['mrcat', mask_list, '-axis', '3', 'all_masks.mif'])
  run.command(f'mrmath all_masks.mif mean - -axis 3 | '
              f'mrthreshold - -abs {app.ARGS.threshold} -comparison ge {final_mask}')

  if app.ARGS.masks:
    run.command(['mrconvert', 'all_masks.mif', app.ARGS.masks],
                mrconvert_keyval=app.ARGS.input,
                force=app.FORCE_OVERWRITE)

  return final_mask
