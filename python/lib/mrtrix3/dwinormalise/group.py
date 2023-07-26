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

import os, shlex
from mrtrix3 import MRtrixError
from mrtrix3 import app, image, path, run



def usage(base_parser, subparsers): #pylint: disable=unused-variable
  parser = subparsers.add_parser('group', parents=[base_parser])
  parser.set_author('David Raffelt (david.raffelt@florey.edu.au)')
  parser.set_synopsis('Performs a global DWI intensity normalisation on a group of subjects using the median b=0 white matter value as the reference')
  parser.add_description('The white matter mask is estimated from a population average FA template then warped back to each subject to perform the intensity normalisation. Note that bias field correction should be performed prior to this step.')
  parser.add_description('All input DWI files must contain an embedded diffusion gradient table; for this reason, these images must all be in either .mif or .mif.gz format.')
  parser.add_argument('input_dir', help='The input directory containing all DWI images')
  parser.add_argument('mask_dir', help='Input directory containing brain masks, corresponding to one per input image (with the same file name prefix)')
  parser.add_argument('output_dir', help='The output directory containing all of the intensity normalised DWI images')
  parser.add_argument('fa_template', help='The output population specific FA template, which is threshold to estimate a white matter mask')
  parser.add_argument('wm_mask', help='The output white matter mask (in template space), used to estimate the median b=0 white matter value for normalisation')
  parser.add_argument('-fa_threshold', default='0.4', help='The threshold applied to the Fractional Anisotropy group template used to derive an approximate white matter mask (default: 0.4)')



def check_output_paths(): #pylint: disable=unused-variable
  app.check_output_path(app.ARGS.output_dir)
  app.check_output_path(app.ARGS.fa_template)
  app.check_output_path(app.ARGS.wm_mask)



def execute(): #pylint: disable=unused-variable

  class Input:
    def __init__(self, filename, prefix, mask_filename = ''):
      self.filename = filename
      self.prefix = prefix
      self.mask_filename = mask_filename


  input_dir = path.from_user(app.ARGS.input_dir, False)
  if not os.path.exists(input_dir):
    raise MRtrixError('input directory not found')
  in_files = path.all_in_dir(input_dir, dir_path=False)
  if len(in_files) <= 1:
    raise MRtrixError('not enough images found in input directory: more than one image is needed to perform a group-wise intensity normalisation')

  app.console('performing global intensity normalisation on ' + str(len(in_files)) + ' input images')

  mask_dir = path.from_user(app.ARGS.mask_dir, False)
  if not os.path.exists(mask_dir):
    raise MRtrixError('mask directory not found')
  mask_files = path.all_in_dir(mask_dir, dir_path=False)
  if len(mask_files) != len(in_files):
    raise MRtrixError('the number of images in the mask directory does not equal the number of images in the input directory')
  mask_common_postfix = os.path.commonprefix([i[::-1] for i in mask_files])[::-1]
  mask_prefixes = []
  for mask_file in mask_files:
    mask_prefixes.append(mask_file.split(mask_common_postfix)[0])

  common_postfix = os.path.commonprefix([i[::-1] for i in in_files])[::-1]
  input_list = []
  for i in in_files:
    subj_prefix = i.split(common_postfix)[0]
    if subj_prefix not in mask_prefixes:
      raise MRtrixError ('no matching mask image was found for input image ' + i)
    image.check_3d_nonunity(os.path.join(input_dir, i))
    index = mask_prefixes.index(subj_prefix)
    input_list.append(Input(i, subj_prefix, mask_files[index]))

  app.make_scratch_dir()
  app.goto_scratch_dir()

  path.make_dir('fa')
  progress = app.ProgressBar('Computing FA images', len(input_list))
  for i in input_list:
    run.command('dwi2tensor ' + shlex.quote(os.path.join(input_dir, i.filename)) + ' -mask ' + shlex.quote(os.path.join(mask_dir, i.mask_filename)) + ' - | tensor2metric - -fa ' + os.path.join('fa', i.prefix + '.mif'))
    progress.increment()
  progress.done()

  app.console('Generating FA population template')
  run.command('population_template fa fa_template.mif'
              + ' -mask_dir ' + mask_dir
              + ' -type rigid_affine_nonlinear'
              + ' -rigid_scale 0.25,0.5,0.8,1.0'
              + ' -affine_scale 0.7,0.8,1.0,1.0'
              + ' -nl_scale 0.5,0.75,1.0,1.0,1.0'
              + ' -nl_niter 5,5,5,5,5'
              + ' -warp_dir warps'
              + ' -linear_no_pause'
              + ' -scratch population_template'
              + ('' if app.DO_CLEANUP else ' -nocleanup'))

  app.console('Generating WM mask in template space')
  run.command('mrthreshold fa_template.mif -abs ' +  app.ARGS.fa_threshold + ' template_wm_mask.mif')

  progress = app.ProgressBar('Intensity normalising subject images', len(input_list))
  path.make_dir(path.from_user(app.ARGS.output_dir, False))
  path.make_dir('wm_mask_warped')
  for i in input_list:
    run.command('mrtransform template_wm_mask.mif -interp nearest -warp_full ' + os.path.join('warps', i.prefix + '.mif') + ' ' + os.path.join('wm_mask_warped', i.prefix + '.mif') + ' -from 2 -template ' + os.path.join('fa', i.prefix + '.mif'))
    run.command('dwinormalise individual ' + shlex.quote(os.path.join(input_dir, i.filename)) + ' ' + os.path.join('wm_mask_warped', i.prefix + '.mif') + ' temp.mif')
    run.command('mrconvert temp.mif ' + path.from_user(os.path.join(app.ARGS.output_dir, i.filename)), mrconvert_keyval=path.from_user(os.path.join(input_dir, i.filename), False), force=app.FORCE_OVERWRITE)
    os.remove('temp.mif')
    progress.increment()
  progress.done()

  app.console('Exporting template images to user locations')
  run.command('mrconvert template_wm_mask.mif ' + path.from_user(app.ARGS.wm_mask), mrconvert_keyval='NULL', force=app.FORCE_OVERWRITE)
  run.command('mrconvert fa_template.mif ' + path.from_user(app.ARGS.fa_template), mrconvert_keyval='NULL', force=app.FORCE_OVERWRITE)
