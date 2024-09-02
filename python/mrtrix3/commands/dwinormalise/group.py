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

import os
from mrtrix3 import MRtrixError
from mrtrix3 import app, image, path, run


FA_THRESHOLD_DEFAULT = 0.4


def usage(base_parser, subparsers): #pylint: disable=unused-variable
  parser = subparsers.add_parser('group', parents=[base_parser])
  parser.set_author('David Raffelt (david.raffelt@florey.edu.au)')
  parser.set_synopsis('Performs a global DWI intensity normalisation on a group of subjects '
                      'using the median b=0 white matter value as the reference')
  parser.add_description('The white matter mask is estimated from a population average FA template '
                         'then warped back to each subject to perform the intensity normalisation. '
                         'Note that bias field correction should be performed prior to this step.')
  parser.add_description('All input DWI files must contain an embedded diffusion gradient table; '
                         'for this reason, '
                         'these images must all be in either .mif or .mif.gz format.')
  parser.add_argument('input_dir',
                      type=app.Parser.DirectoryIn(),
                      help='The input directory containing all DWI images')
  parser.add_argument('mask_dir',
                      type=app.Parser.DirectoryIn(),
                      help='Input directory containing brain masks, '
                           'corresponding to one per input image '
                           '(with the same file name prefix)')
  parser.add_argument('output_dir',
                      type=app.Parser.DirectoryOut(),
                      help='The output directory containing all of the intensity normalised DWI images')
  parser.add_argument('fa_template',
                      type=app.Parser.ImageOut(),
                      help='The output population-specific FA template, '
                           'which is thresholded to estimate a white matter mask')
  parser.add_argument('wm_mask',
                      type=app.Parser.ImageOut(),
                      help='The output white matter mask (in template space), '
                           'used to estimate the median b=0 white matter value for normalisation')
  parser.add_argument('-fa_threshold',
                      type=app.Parser.Float(0.0, 1.0),
                      default=FA_THRESHOLD_DEFAULT,
                      help='The threshold applied to the Fractional Anisotropy group template '
                           'used to derive an approximate white matter mask '
                           f'(default: {FA_THRESHOLD_DEFAULT})')



def execute(): #pylint: disable=unused-variable

  class Input:
    def __init__(self, filename, prefix, mask_filename = ''):
      self.filename = filename
      self.prefix = prefix
      self.mask_filename = mask_filename


  in_files = path.all_in_dir(app.ARGS.input_dir, dir_path=False)
  if len(in_files) <= 1:
    raise MRtrixError('not enough images found in input directory: '
                      'more than one image is needed to perform a group-wise intensity normalisation')

  app.console(f'Performing global intensity normalisation on {len(in_files)} input images')

  mask_files = path.all_in_dir(app.ARGS.mask_dir, dir_path=False)
  if len(mask_files) != len(in_files):
    raise MRtrixError('the number of images in the mask directory does not equal the '
                      'number of images in the input directory')
  mask_common_postfix = os.path.commonprefix([i[::-1] for i in mask_files])[::-1]
  mask_prefixes = []
  for mask_file in mask_files:
    mask_prefixes.append(mask_file.split(mask_common_postfix)[0])

  common_postfix = os.path.commonprefix([i[::-1] for i in in_files])[::-1]
  input_list = []
  for i in in_files:
    subj_prefix = i.split(common_postfix)[0]
    if subj_prefix not in mask_prefixes:
      raise MRtrixError (f'no matching mask image was found for input image {i}')
    image.check_3d_nonunity(os.path.join(app.ARGS.input_dir, i))
    index = mask_prefixes.index(subj_prefix)
    input_list.append(Input(i, subj_prefix, mask_files[index]))

  app.activate_scratch_dir()

  os.makedirs('fa')
  progress = app.ProgressBar('Computing FA images', len(input_list))
  for i in input_list:
    run.command(['dwi2tensor', os.path.join(app.ARGS.input_dir, i.filename), '-mask', os.path.join(app.ARGS.mask_dir, i.mask_filename), '-', '|',
                 'tensor2metric', '-', '-fa', os.path.join('fa', f'{i.prefix}.mif')])
    progress.increment()
  progress.done()

  app.console('Generating FA population template')
  run.command(['population_template', 'fa', 'fa_template.mif',
               '-mask_dir', app.ARGS.mask_dir,
               '-type', 'rigid_affine_nonlinear',
               '-rigid_scale', '0.25,0.5,0.8,1.0',
               '-affine_scale', '0.7,0.8,1.0,1.0',
               '-nl_scale', '0.5,0.75,1.0,1.0,1.0',
               '-nl_niter', '5,5,5,5,5',
               '-warp_dir', 'warps',
               '-linear_no_pause',
               '-scratch', app.SCRATCH_DIR]
               + ([] if app.DO_CLEANUP else ['-nocleanup']))

  app.console('Generating WM mask in template space')
  run.command(f'mrthreshold fa_template.mif -abs {app.ARGS.fa_threshold} template_wm_mask.mif')

  progress = app.ProgressBar('Intensity normalising subject images', len(input_list))
  app.ARGS.output_dir.mkdir()
  os.makedirs('wm_mask_warped')
  os.makedirs('dwi_normalised')
  for i in input_list:
    in_path = os.path.join(app.ARGS.input_dir, i.filename)
    warp_path = os.path.join('warps', f'{i.prefix}.mif')
    fa_path = os.path.join('fa', f'{i.prefix}.mif')
    wm_mask_warped_path = os.path.join('wm_mask_warped', f'{i.prefix}.mif')
    dwi_normalised_path = os.path.join('dwi_normalised', f'{i.prefix}.mif')
    run.command(['mrtransform', 'template_wm_mask.mif', wm_mask_warped_path,
                 '-interp', 'nearest',
                 '-warp_full', warp_path,
                 '-from', '2',
                 '-template', fa_path])
    run.command(['dwinormalise', 'manual',
                 in_path,
                 wm_mask_warped_path,
                 dwi_normalised_path])
    run.command(['mrconvert', dwi_normalised_path, app.ARGS.output_dir / i.filename],
                mrconvert_keyval=in_path,
                force=app.FORCE_OVERWRITE)
    app.cleanup([warp_path, fa_path, wm_mask_warped_path, dwi_normalised_path])
    progress.increment()
  progress.done()

  app.console('Exporting template images to user locations')
  run.command(['mrconvert', 'template_wm_mask.mif', app.ARGS.wm_mask],
              mrconvert_keyval='NULL',
              force=app.FORCE_OVERWRITE)
  run.command(['mrconvert', 'fa_template.mif', app.ARGS.fa_template],
              mrconvert_keyval='NULL',
              force=app.FORCE_OVERWRITE)
