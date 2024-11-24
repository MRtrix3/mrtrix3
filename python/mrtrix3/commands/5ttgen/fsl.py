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

import math, os, shutil
from mrtrix3 import MRtrixError
from mrtrix3 import app, fsl, image, run, utils



def usage(base_parser, subparsers): #pylint: disable=unused-variable
  parser = subparsers.add_parser('fsl', parents=[base_parser])
  parser.set_author('Robert E. Smith (robert.smith@florey.edu.au)')
  parser.set_synopsis('Use FSL commands to generate the 5TT image based on a T1-weighted image')
  parser.add_citation('Smith, S. M. '
                      'Fast robust automated brain extraction. '
                      'Human Brain Mapping, 2002, 17, 143-155',
                      is_external=True)
  parser.add_citation('Zhang, Y.; Brady, M. & Smith, S. '
                      'Segmentation of brain MR images through a hidden Markov random field model and the expectation-maximization algorithm. '
                      'IEEE Transactions on Medical Imaging, 2001, 20, 45-57',
                      is_external=True)
  parser.add_citation('Patenaude, B.; Smith, S. M.; Kennedy, D. N. & Jenkinson, M. '
                      'A Bayesian model of shape and appearance for subcortical brain segmentation. '
                      'NeuroImage, 2011, 56, 907-922',
                      is_external=True)
  parser.add_citation('Smith, S. M.; Jenkinson, M.; Woolrich, M. W.; Beckmann, C. F.; Behrens, T. E.; Johansen-Berg, H.; Bannister, P. R.; De Luca, M.; Drobnjak, I.; Flitney, D. E.; Niazy, R. K.; Saunders, J.; Vickers, J.; Zhang, Y.; De Stefano, N.; Brady, J. M. & Matthews, P. M. '
                      'Advances in functional and structural MR image analysis and implementation as FSL. '
                      'NeuroImage, 2004, 23, S208-S219',
                      is_external=True)
  parser.add_argument('input',
                      type=app.Parser.ImageIn(),
                      help='The input T1-weighted image')
  parser.add_argument('output',
                      type=app.Parser.ImageOut(),
                      help='The output 5TT image')
  options = parser.add_argument_group('Options specific to the "fsl" algorithm')
  options.add_argument('-t2',
                       type=app.Parser.ImageIn(),
                       help='Provide a T2-weighted image in addition to the default T1-weighted image; '
                            'this will be used as a second input to FSL FAST')
  options.add_argument('-mask',
                       type=app.Parser.ImageIn(),
                       help='Manually provide a brain mask, '
                            'rather than deriving one in the script')
  options.add_argument('-premasked',
                       action='store_true',
                       default=None,
                       help='Indicate that brain masking has already been applied to the input image')
  options.add_argument('-first_dir',
                       type=app.Parser.DirectoryIn(),
                       metavar='/path/to/first/dir',
                       help='use pre-calculated output of FSL FIRST previously run on input T1-weighted image; '
                            'data must be defined in the same space as input T1w')
  options.add_argument('-fast_dir',
                       type=app.Parser.DirectoryIn(),
                       metavar='/path/to/fast/dir',
                       help='use pre-calculated output of FSL FAST previously run on input T1-weighted image; '
                            'data must be defined in the same space as input T1w; '
                            'filename prefix must be "T1_BET"')
  parser.flag_mutually_exclusive_options( [ 'mask', 'premasked' ] )



def execute(): #pylint: disable=unused-variable
  if utils.is_windows():
    raise MRtrixError('"fsl" algorithm of 5ttgen script cannot be run on Windows: '
                      'FSL not available on Windows')

  fsl_path = os.environ.get('FSLDIR', '')
  if not fsl_path:
    raise MRtrixError('Environment variable FSLDIR is not set; '
                      'please run appropriate FSL configuration script')

  sgm_structures = [ 'L_Accu', 'R_Accu', 'L_Caud', 'R_Caud', 'L_Pall', 'R_Pall', 'L_Puta', 'R_Puta', 'L_Thal', 'R_Thal' ]
  if app.ARGS.sgm_amyg_hipp:
    sgm_structures.extend([ 'L_Amyg', 'R_Amyg', 'L_Hipp', 'R_Hipp' ])

  bet_cmd = fsl.exe_name('bet')
  ssroi_cmd = fsl.exe_name('standard_space_roi')

  fast_cmd = None
  fast_pve_images = []
  if app.ARGS.fast_dir:
    pve_candidates = sorted(app.ARGS.fast_dir.glob('*_pve_*.nii*'))
    if len(pve_candidates) != 3:
      raise MRtrixError(f'Expected three partial volume images in FAST directory {app.ARGS.fast_dir}; '
                        f'found {len(pve_candidates)}')
    for index, filepath in enumerate(pve_candidates):
      if not f'_pve_{index}.nii' in filepath:
        raise MRtrixError('Expected FAST PVE image "*_pve_*.nii*"; '
                          f'found "{filepath.name}"')
      fast_pve_images.append(filepath.name)
  else:
    try:
      fast_cmd = fsl.exe_name('fast')
    except MRtrixError as e:
      raise MRtrixError('FSL program "fast" is requisite if -fast_dir option is not used') from e

  first_cmd = None
  first_object_files = []
  if app.ARGS.first_dir:
    first_prefix = fsl.check_first_input(app.ARGS.first_dir, sgm_structures)
    first_object_files = [app.ARGS.first_dir / f'{first_prefix}{struct}_first.vtk' for struct in sgm_structures]
  else:
    try:
      first_cmd = fsl.exe_name('run_first_all')
    except MRtrixError as e:
      raise MRtrixError('FSL program "run_first_all" is requisite if -first_dir option is not used') from e

  first_atlas_path = os.path.join(fsl_path, 'data', 'first', 'models_336_bin')
  if not os.path.isdir(first_atlas_path):
    raise MRtrixError('Atlases required for FSL\'s FIRST program not installed; '
                      'please install fsl-first-data using your relevant package manager')

  fsl_suffix = fsl.suffix()

  image.check_3d_nonunity(app.ARGS.input)
  run.command(['mrconvert', app.ARGS.input, 'input.mif'],
              preserve_pipes=True)
  if app.ARGS.mask:
    run.command(['mrconvert', app.ARGS.mask, 'mask.mif', '-datatype', 'bit', '-strides', '-1,+2,+3'],
                preserve_pipes=True)
  elif not app.ARGS.premasked and not shutil.which('dc'):
    app.warn('Unix command "dc" not found; '
             'FSL script "standard_space_roi" may fail')
  if app.ARGS.t2:
    if not image.match('input.mif', app.ARGS.t2):
      raise MRtrixError('Provided T2w image does not match input T1w image')
    run.command(['mrconvert', app.ARGS.t2, 'T2.nii', '-strides', '-1,+2,+3'],
                preserve_pipes=True)
  if app.ARGS.fast_dir:
    for name in fast_pve_images:
      shutil.copyfile(app.ARGS.fast_dir / name, app.SCRATCH_DIR / name)
  if app.ARGS.first_dir:
    progress = app.ProgressBar('Importing pre-calculated FIRST segmentations', len(sgm_structures))
    for filename in first_object_files:
      run.function(shutil.copyfile, app.ARGS.first_dir / filename, app.SCRATCH_DIR / filename)
      progress.increment()
    progress.done()

  t1_spacing = image.Header('input.mif').spacing()
  upsample_for_first = False
  # If voxel size is 1.25mm or larger, make a guess that the user has erroneously re-gridded their data
  if not app.ARGS.first_dir and math.pow(t1_spacing[0] * t1_spacing[1] * t1_spacing[2], 1.0/3.0) > 1.225:
    app.warn(f'Voxel size larger than expected for T1-weighted images ({t1_spacing}); '
             f'note that ACT does not require re-gridding of T1 image to DWI space, and indeed '
             f'retaining the original higher resolution of the T1 image is preferable')
    upsample_for_first = True

  run.command('mrconvert input.mif T1.nii -strides -1,+2,+3')

  fast_t1_input = 'T1.nii'
  fast_t2_input = ''

  # Decide whether or not we're going to do any brain masking
  if app.ARGS.mask:

    fast_t1_input = f'T1_masked{fsl_suffix}'

    # Check to see if the mask matches the T1 image
    if image.match('T1.nii', 'mask.mif'):
      run.command(f'mrcalc T1.nii mask.mif -mult {fast_t1_input}')
      mask_path = 'mask.mif'
    else:
      app.warn('Mask image does not match input image - re-gridding')
      run.command('mrtransform mask.mif mask_regrid.mif -template T1.nii -datatype bit')
      run.command(f'mrcalc T1.nii mask_regrid.mif -mult {fast_t1_input}')
      mask_path = 'mask_regrid.mif'

    if os.path.exists('T2.nii'):
      fast_t2_input = f'T2_masked{fsl_suffix}'
      run.command(f'mrcalc T2.nii {mask_path} -mult {fast_t2_input}')

  elif app.ARGS.premasked:

    fast_t1_input = 'T1.nii'
    if os.path.exists('T2.nii'):
      fast_t2_input = 'T2.nii'

  else:

    # Use FSL command standard_space_roi to do an initial masking of the image before BET
    # Also reduce the FoV of the image
    # Using MNI 1mm dilated brain mask rather than the -b option in standard_space_roi (which uses the 2mm mask); the latter looks 'buggy' to me... Unfortunately even with the 1mm 'dilated' mask, it can still cut into some brain areas, hence the explicit dilation
    mni_mask_path = os.path.join(fsl_path, 'data', 'standard', 'MNI152_T1_1mm_brain_mask_dil.nii.gz')
    mni_mask_dilation = 0
    if os.path.exists (mni_mask_path):
      mni_mask_dilation = 4
    else:
      mni_mask_path = os.path.join(fsl_path, 'data', 'standard', 'MNI152_T1_2mm_brain_mask_dil.nii.gz')
      if os.path.exists (mni_mask_path):
        mni_mask_dilation = 2
    try:
      if mni_mask_dilation:
        run.command(f'maskfilter {mni_mask_path} dilate mni_mask.nii -npass {mni_mask_dilation}')
        if app.ARGS.nocrop:
          ssroi_roi_option = ' -roiNONE'
        else:
          ssroi_roi_option = ' -roiFOV'
        run.command(f'{ssroi_cmd} T1.nii T1_preBET{fsl_suffix} -maskMASK mni_mask.nii {ssroi_roi_option}')
      else:
        run.command(f'{ssroi_cmd} T1.nii T1_preBET{fsl_suffix} -b')
    except run.MRtrixCmdError:
      pass
    try:
      pre_bet_image = fsl.find_image('T1_preBET')
    except MRtrixError:
      dc_warning = '' if shutil.which('dc') else ' (possibly due to program "dc" not being installed'
      app.warn(f'FSL script "standard_space_roi" did not complete successfully{dc_warning}; '
               'attempting to continue by providing un-cropped image to BET')
      pre_bet_image = 'T1.nii'

    # BET
    run.command(f'{bet_cmd} {pre_bet_image} T1_BET{fsl_suffix} -f 0.15 -R')
    fast_t1_input = fsl.find_image(f'T1_BET{fsl_suffix}')

    if os.path.exists('T2.nii'):
      if app.ARGS.nocrop:
        fast_t2_input = 'T2.nii'
      else:
        # Just a reduction of FoV, no sub-voxel interpolation going on
        run.command(f'mrtransform T2.nii T2_cropped.nii -template {fast_t1_input} -interp nearest')
        fast_t2_input = 'T2_cropped.nii'

  # Finish branching based on brain masking

  # FAST
  if not app.ARGS.fast_dir:
    assert not fast_pve_images
    if fast_t2_input:
      run.command(f'{fast_cmd} -S 2 {fast_t2_input} {fast_t1_input}')
    else:
      run.command(f'{fast_cmd} {fast_t1_input}')
    for index in range(0, 3):
      fast_pve_images.append(fsl.find_image(f'{fast_t1_input}_pve_{index}'))

  # FIRST
  # TODO Preferably find a suitable reference image inside the FIRST directory
  first_reference = 'T1.nii'
  if not app.ARGS.first_dir:
    assert not first_object_files
    if upsample_for_first:
      app.warn('Generating 1mm isotropic T1 image for FIRST in hope of preventing failure, '
              'since input image is of lower resolution')
      run.command('mrgrid T1.nii regrid T1_1mm.nii -voxel 1.0 -interp sinc')
      first_reference = 'T1_1mm.nii'
    first_brain_extracted_option = ['-b'] if app.ARGS.premasked else []
    first_debug_option = [] if app.DO_CLEANUP else ['-d']
    first_verbosity_option = ['-v'] if app.VERBOSITY == 3 else []
    run.command([first_cmd, '-m', 'none', '-s', ','.join(sgm_structures), '-i', first_reference, '-o', 'first']
                + first_brain_extracted_option
                + first_debug_option
                + first_verbosity_option)
    fsl.check_first_output('first', sgm_structures)
    first_object_files = [f'first-{struct}_first.vtk' for struct in sgm_structures]

  # Convert FIRST meshes to partial volume images
  pve_image_list = [ ]
  progress = app.ProgressBar('Generating partial volume images for SGM structures', len(sgm_structures))
  for struct, vtk_in_path in zip(sgm_structures, first_object_files):
    pve_image_path = f'mesh2voxel_{struct}.mif'
    vtk_temp_path = f'{struct}.vtk'
    run.command(['meshconvert', vtk_in_path, vtk_temp_path, '-transform', 'first2real', first_reference])
    run.command(['mesh2voxel', vtk_temp_path, fast_t1_input, pve_image_path])
    pve_image_list.append(pve_image_path)
    progress.increment()
  progress.done()
  run.command(['mrmath', pve_image_list, 'sum', '-', '|',
               'mrcalc', '-', '1.0', '-min', 'all_sgms.mif'])

  # Combine the tissue images into the 5TT format within the script itself
  # Step 1: Run LCC on the WM image
  run.command(f'mrthreshold {fast_pve_images[2]} - -abs 0.001 | '
              f'maskfilter - connect - -connectivity | '
              f'mrcalc 1 - 1 -gt -sub remove_unconnected_wm_mask.mif -datatype bit')
  # Step 2: Generate the images in the same fashion as the old 5ttgen binary used to:
  #   - Preserve CSF as-is
  #   - Preserve SGM, unless it results in a sum of volume fractions greater than 1, in which case clamp
  #   - Multiply the FAST volume fractions of GM and CSF, so that the sum of CSF, SGM, CGM and WM is 1.0
  run.command(f'mrcalc {fast_pve_images[0]} remove_unconnected_wm_mask.mif -mult csf.mif')
  run.command('mrcalc 1.0 csf.mif -sub all_sgms.mif -min sgm.mif')
  run.command(f'mrcalc 1.0 csf.mif sgm.mif -add -sub {fast_pve_images[1]} {fast_pve_images[2]} -add -div multiplier.mif')
  run.command('mrcalc multiplier.mif -finite multiplier.mif 0.0 -if multiplier_noNAN.mif')
  run.command(f'mrcalc {fast_pve_images[1]} multiplier_noNAN.mif -mult remove_unconnected_wm_mask.mif -mult cgm.mif')
  run.command(f'mrcalc {fast_pve_images[2]} multiplier_noNAN.mif -mult remove_unconnected_wm_mask.mif -mult wm.mif')
  run.command('mrcalc 0 wm.mif -min path.mif')
  run.command('mrcat cgm.mif sgm.mif wm.mif csf.mif path.mif - -axis 3 | '
              'mrconvert - combined_precrop.mif -strides +2,+3,+4,+1')

  # Crop to reduce file size (improves caching of image data during tracking)
  if app.ARGS.nocrop:
    run.function(os.rename, 'combined_precrop.mif', 'result.mif')
  else:
    run.command('mrmath combined_precrop.mif sum - -axis 3 | '
                'mrthreshold - - -abs 0.5 | '
                'mrgrid combined_precrop.mif crop result.mif -mask -')

  run.command(['mrconvert', 'result.mif', app.ARGS.output],
              mrconvert_keyval=app.ARGS.input,
              force=app.FORCE_OVERWRITE,
              preserve_pipes=True)

  return 'result.mif'
