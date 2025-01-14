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
import argparse 
from mrtrix3 import MRtrixError
from mrtrix3 import app, image, run

def usage(base_parser, subparsers):  #pylint: disable=unused-variable
  parser = subparsers.add_parser('deep_atropos', parents=[base_parser])
  parser.set_author('Lucius S. Fekonja (lucius.fekonja[at]charite.de) and Robert E. Smith (robert.smith@florey.edu.au)')
  parser.set_synopsis('Generate the 5TT image based on a Deep Atropos segmentation or probabilities image')
  parser.add_citation('Use of the ANTsX ecosystem should be accompanied by the following citation:\n'
                     'N.J. Tustison, P.A. Cook, A.J. Holbrook, H.J. Johnson, J. Muschelli, G.A. Devenyi, J.T. Duda, S.R. Das, '
                     'N.C. Cullen, D.L. Gillen, M.A. Yassa, J.R. Stone, J.C. Gee, and B.B. Avants. '
                     'The ANTsX ecosystem for quantitative biological and medical imaging. '
                     'Scientific Reports, 11(1):9068 (2021), pp. 1-13.',
                     is_external=True)
  parser.add_description('This algorithm accepts either:')
  parser.add_description('1. A 3D segmentation image from Deep Atropos with labels:')
  parser.add_description('   - 0: Background - 1: CSF - 2: Gray Matter - 3: White Matter - 4: Deep Gray Matter - 5: Brain Stem - 6: Cerebellum')
  parser.add_description('2. A 4D probability image (dimensions [x,y,z,7]) containing probability maps for each tissue class in the same order as above')
  parser.add_description('To generate these input images using antspynet\'s deep_atropos function:')
  parser.add_description('1. Install required packages: pip install antspynet nibabel')
  parser.add_description('2. Use Python commands:')
  parser.add_description('   # Load the T1-weighted image')
  parser.add_description('   >>> import ants, antspynet')
  parser.add_description('   >>> import nibabel as nib')
  parser.add_description('   >>> import numpy as np')
  parser.add_description('   >>> t1_image = ants.image_read(\'input_t1.nii.gz\')')
  parser.add_description('   # Run Deep Atropos segmentation')
  parser.add_description('   >>> segments = antspynet.deep_atropos(t1_image)')
  parser.add_description('   # Save the segmentation')
  parser.add_description('   >>> ants.image_write(segments[\'segmentation_image\'], \'segmentation.nii.gz\')')
  parser.add_description('   # Stack and save probability maps')
  parser.add_description('   >>> prob_maps = np.stack([np.array(img.numpy()) for img in segments[\'probability_images\']], axis=-1)')
  parser.add_description('   >>> nib.save(nib.Nifti1Image(prob_maps, t1_image.affine), \'probabilities.nii.gz\')')
  parser.add_description('The generated files can then be used with MRtrix3:')
  parser.add_description('   5ttgen deep_atropos segmentation.nii.gz 5tt.mif')
  parser.add_description('   5ttgen deep_atropos probabilities.nii.gz 5tt.mif')
  parser.add_argument('input',
    type=app.Parser.ImageIn(),
    help='The input Deep Atropos segmentation image')
  parser.add_argument('output',
    type=app.Parser.ImageOut(),
    help='The output 5TT image')
  parser.add_argument('-white_stem',
    action='store_true',
    default=None,
    help='Classify the brainstem as white matter')

def execute():  #pylint: disable=unused-variable
  if app.ARGS.sgm_amyg_hipp:
    app.warn('Option -sgm_amyg_hipp has no effect on deep_atropos algorithm')

  dim = image.Header(app.ARGS.input).size()
  if not(len(dim) == 3 or (len(dim) == 4 and dim[3] == 7)):
    raise MRtrixError(f'Image \'{str(app.ARGS.input)}\' does not look like Deep Atropos segmentation'
                      f' (expected either a 3D image, or a 4D image with 7 volumes; input image is {dim})')

  run.command(['mrconvert', app.ARGS.input, 'input.mif'])

  if len(dim) == 3:
    # Generate tissue-specific masks
    run.command('mrcalc input.mif 2 -eq cGM.mif')
    run.command('mrcalc input.mif 4 -eq input.mif 6 -eq -add sGM.mif')
    run.command(f'mrcalc input.mif 3 -eq{" input.mif 5 -eq -add" if app.ARGS.white_stem else ""} WM.mif')
    run.command('mrcalc input.mif 1 -eq CSF.mif')
    run.command(f'mrcalc input.mif {("0 -mult" if app.ARGS.white_stem else "5 -eq")} path.mif')
  else:
    # Brain mask = non-brain probability is <50%
    run.command('mrconvert input.mif -coord 3 0 -axes 0,1,2 - | '
      'mrthreshold - -abs 0.5 -comparison le mask.mif')
    # Need to rescale model probabilities so that, excluding the non-brain component,
    #   the sum across all tissues will be 1.0
    run.command('mrconvert input.mif -coord 3 1:end - | '
      'mrmath - sum -axis 3 - | '
      'mrcalc 1 - -div multiplier.mif')
    # Generate tissue-specific probability maps
    run.command('mrconvert input.mif -coord 3 2 -axes 0,1,2 cGM.mif')
    run.command('mrconvert input.mif -coord 3 4,6 - | '
      'mrmath - sum -axis 3 sGM.mif')
    if app.ARGS.white_stem:
      run.command('mrconvert input.mif -coord 3 3,5 - | '
        'mrmath - sum -axis 3 WM.mif')
    else:
      run.command('mrconvert input.mif -coord 3 3 -axes 0,1,2 WM.mif')
    run.command('mrconvert input.mif -coord 3 1 -axes 0,1,2 CSF.mif')
    if app.ARGS.white_stem:
      run.command('mrcalc cGM.mif 0 -mult path.mif')
    else:
      run.command('mrconvert input.mif -coord 3 5 -axes 0,1,2 path.mif')

  # Concatenate into the 5TT image
  run.command('mrcat cGM.mif sGM.mif WM.mif CSF.mif path.mif - -axis 3 | '
    f'{"mrcalc - multiplier.mif -mult mask.mif -mult - | " if len(dim) == 4 else ""}'
    'mrconvert - combined_precrop.mif -strides +2,+3,+4,+1')

  # Apply cropping unless disabled
  if app.ARGS.nocrop:
    run.function(os.rename, 'combined_precrop.mif', 'result.mif')
  else:
    run.command('mrmath combined_precrop.mif sum - -axis 3 | '
      'mrthreshold - - -abs 0.5 | '
      'mrgrid combined_precrop.mif crop result.mif -mask -')

  run.command(['mrconvert', 'result.mif', app.ARGS.output],
    mrconvert_keyval=app.ARGS.input,
    force=app.FORCE_OVERWRITE)
