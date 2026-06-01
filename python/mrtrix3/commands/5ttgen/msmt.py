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

import glob, os
from mrtrix3 import MRtrixError
from mrtrix3 import app, image, path, run


def usage(base_parser, subparsers):  # pylint: disable=unused-variable
  parser = subparsers.add_parser('msmt', parents=[base_parser])
  parser.set_author('Arkiev D\'Souza (arkiev.dsouza@sydney.edu.au) & Robert E. Smith (robert.smith@florey.edu.au)')
  parser.set_synopsis('Generate a 5TT image from ODF images')
  parser.add_description('If the user does not manually provide a brain mask using the -mask option,'
                         ' the command will automatically determine a mask for the output 5TT image;'
                         ' this is necessary to conform to the expectations of the 5TT format.'
                         ' This mask will be computed based on where the sum of the ODFs exceeds 50%'
                         ' of what is expected from a voxel that contains a DWI signal'
                         ' that exactly matches one of the response functions.')

  parser.add_argument('odf_wm', help='The input white matter ODF')
  parser.add_argument('odf_gm', help='The input grey matter ODF')
  parser.add_argument('odf_csf', help='The input cerebrospinal fluid ODF')
  parser.add_argument('output', help='The output 5TT image')

  options = parser.add_argument_group('Options specific to the \'msmt\' algorithm')
  options.add_argument('-mask', help='An input binary brain mask image')


def check_output_paths(): # pylint: disable=unused-variable
  app.check_output_path(app.ARGS.output)


def get_inputs(): # pylint: disable=unused-variable
  run.command('mrconvert ' + path.from_user(app.ARGS.odf_wm)  + ' ' + path.to_scratch('odf_wm.mif'))
  run.command('mrconvert ' + path.from_user(app.ARGS.odf_gm)  + ' ' + path.to_scratch('odf_gm.mif'))
  run.command('mrconvert ' + path.from_user(app.ARGS.odf_csf) + ' ' + path.to_scratch('odf_csf.mif'))
  if app.ARGS.mask:
    run.command('mrconvert ' + path.from_user(app.ARGS.mask) + ' ' + path.to_scratch('mask.mif') + ' -datatype bit')


def execute(): # pylint: disable=unused-variable

  target_voxel_grid = image.Header('odf_wm.mif')

  class Tissue:
    def __init__(self, inpath, name, expect_anisotropic):
      self.name = name
      header = image.Header(inpath)
      do_regrid = not image.match(header, target_voxel_grid, up_to_dim=3)
      if do_regrid:
        app.warn(f'ODF image "{inpath}" not defined on same voxel grid as WM ODF image;'
                 ' will have to resample')
      else:
        app.debug(f'{name} ODF image already on target voxel grid')
      is_anisotropic = len(header.size()) > 3 and header.size()[3] > 1
      if is_anisotropic != expect_anisotropic:
        app.warn(f'Received {"anisotropic" if is_anisotropic else "isotropic"} ODF for {name}'
                 f' but expected {"anisotropic" if expect_anisotropic else "isotropic"};'
                 ' check order of input ODF images if this was not intentional')
      self.lzeropath = f'{name}_lzero.mif'
      command = ['mrgrid', inpath, 'regrid', '-template', 'odf_wm.mif', '-', '|'] \
                if do_regrid                                                         \
                else []
      if is_anisotropic:
        command.extend(['mrconvert', '-' if command else inpath, '-coord', '3', '0', '-axes', '0,1,2', '-', '|'])
      command.extend(['mrcalc', '-' if command else inpath, '0.0', '-max', self.lzeropath])
      run.command(command)
      self.normpath = None

    def normalise(self, volsum_image):
      assert self.normpath is None
      self.normpath = f'{self.name}_fraction.mif'
      run.command(f'mrcalc {self.lzeropath} {volsum_image} -divide {self.normpath}')

  # End definition of class Tissue

  tissues = [Tissue('odf_wm.mif',  'WM',  True),
             Tissue('odf_gm.mif',  'GM',  False),
             Tissue('odf_csf.mif', 'CSF', False)]

  # Compute volume sum in each voxel
  volsum_image = 'totalvol.mif'
  run.command(['mrmath', [item.lzeropath for item in tissues], 'sum', volsum_image])
  # Normalise to tissue fractions
  for tissue in tissues:
    tissue.normalise(volsum_image)

  # Create empty volume for SGM and pathology
  empty_volume = 'empty_vol.mif'
  run.command(f'mrcalc {volsum_image} inf -gt {empty_volume}')

  # Concatenate volumes
  result_unmasked = '5TT_unmasked.mif'
  ####################################################################################################################
  # 5TT volumes:          Cortical GM      Sub-cortical GM          WM                   CSF             Pathology   #
  ####################################################################################################################
  run.command(f'mrcat {tissues[1].normpath} {empty_volume} {tissues[0].normpath} {tissues[2].normpath} {empty_volume}'
              f' {result_unmasked} -datatype float32')

  # Tidy image by masking
  result_masked = '5TT_masked.mif'
  if app.ARGS.mask:
    mask_header = image.Header('mask.mif')
    if image.match(target_voxel_grid, mask_header, up_to_dim=3):
      app.debug('Mask matches ODF voxel grid; no regridding of mask required')
      run.command(f'mrcalc {result_unmasked} mask.mif -mult {result_masked}')
    else:
      app.warn('Mask has different voxel grid to WM ODF image; regridding')
      run.command(f'mrgrid mask.mif -template {empty_volume} regrid - |'
                  ' mrcalc - 0.5 -gt - |'
                  f' mrcalc - {result_unmasked} -mult {result_masked}')
  else:
    app.console('No 5TT mask provided; generating from input ODFs')
    run.command(f'mrcalc {volsum_image} 0 -gt - |'
                f' mrcalc - {result_unmasked} -mult {result_masked}')

  # Brain mask derived from the masked 5TT — needed for CSF fill in both code paths
  run.command('mrmath ' + result_masked + ' sum - -axis 3 | mrcalc - 0.0 -gt brain_mask_derived.mif')

  if app.ARGS.sgm_amyg_hipp:
    # --- vis image: grayscale rendering used as T1-surrogate for FIRST and FAST ---
    from mrtrix3 import fsl
    app.console('Generating vis image from initial 5TT for FSL FIRST and FAST')
    run.command('5tt2vis ' + result_masked + ' - | mrconvert - vis.nii -strides -1,+2,+3')

    # --- FSL availability check ---
    have_first = False
    have_fast  = False
    first_cmd  = None
    fast_cmd   = None

    fsl_path = os.environ.get('FSLDIR', '')
    if not fsl_path:
      app.warn('FSLDIR not set; FSL FIRST and FAST will be skipped')
    else:
      try:
        first_cmd = fsl.exe_name('run_first_all')
        first_atlas_path = os.path.join(fsl_path, 'data', 'first', 'models_336_bin')
        have_first = os.path.isdir(first_atlas_path)
        if not have_first:
          app.warn('FSL FIRST atlas not found; sub-cortical GM will be empty')
      except MRtrixError:
        app.warn('FSL run_first_all not found; sub-cortical GM will be empty')
      try:
        fast_cmd = fsl.exe_name('fast')
        have_fast = True
      except MRtrixError:
        app.warn('FSL fast not found; intensity-guided tissue segmentation will be skipped')

    # --- FIRST: sub-cortical GM segmentation ---
    SGM_FIRST_MAP = {
      'L_Accu': 'Left-Accumbens-area',  'R_Accu': 'Right-Accumbens-area',
      'L_Amyg': 'Left-Amygdala',        'R_Amyg': 'Right-Amygdala',
      'L_Caud': 'Left-Caudate',         'R_Caud': 'Right-Caudate',
      'L_Hipp': 'Left-Hippocampus',     'R_Hipp': 'Right-Hippocampus',
      'L_Pall': 'Left-Pallidum',        'R_Pall': 'Right-Pallidum',
      'L_Puta': 'Left-Putamen',         'R_Puta': 'Right-Putamen',
      'L_Thal': 'Left-Thalamus-Proper', 'R_Thal': 'Right-Thalamus-Proper'
    }

    if have_first:
      app.console('Running FSL FIRST to segment sub-cortical grey matter structures')
      first_stdout = run.command(
        [first_cmd, '-s', ','.join(SGM_FIRST_MAP.keys()), '-i', 'vis.nii', '-b', '-o', 'first']
      ).stdout
      fsl.check_first('first', structures=SGM_FIRST_MAP.keys(), first_stdout=first_stdout)

      sgm_images = []
      for key, value in SGM_FIRST_MAP.items():
        vtk_in     = 'first-' + key + '_first.vtk'
        vtk_out    = 'first-' + key + '_transformed.vtk'
        sgm_img    = value + '.mif'
        sgm_filled = value + '_filled.mif'
        run.command('meshconvert ' + vtk_in + ' ' + vtk_out + ' -transform first2real vis.nii')
        run.command('mesh2voxel ' + vtk_out + ' odf_wm.mif ' + sgm_img)
        run.command('mrcalc ' + sgm_img + ' 0.5 -gt - -datatype bit | '
                    'mrcalc - 0 -eq - | '
                    'maskfilter - connect -largest - | '
                    'mrcalc - 0 -eq ' + sgm_filled + ' -datatype bit')
        app.cleanup(sgm_img)
        sgm_images.append(sgm_filled)

      run.command(['mrmath', sgm_images, 'sum', 'sgm_sum.mif'])
      run.command('mrcalc sgm_sum.mif 0.5 -gt sgm.mif -datatype float32')
      run.command('mrcalc sgm.mif brain_mask_derived.mif -mult sgm_masked.mif')
      app.cleanup(glob.glob('first*'))
      sgm_volume = 'sgm_masked.mif'
    else:
      sgm_volume = empty_volume

    # --- FAST: intensity-guided tissue segmentation using vis image ---
    # vis.nii (derived from 5TT_masked via 5tt2vis) has WM bright, CSF dark,
    #   giving FAST three classes: pve_0=CSF, pve_1=GM, pve_2=WM
    if have_fast:
      app.console('Running FSL FAST on vis image for intensity-guided tissue segmentation')
      run.command('mrcalc vis.nii brain_mask_derived.mif -mult vis_brain.nii')
      run.command(fast_cmd + ' -N vis_brain.nii')
      app.cleanup('vis_brain.nii')
      app.cleanup([e for e in glob.glob('vis_brain*') if '_pve_' not in e])

      fast_pve_prefix = 'vis_brain_pve_'
      run.command('mrconvert ' + fsl.find_image(fast_pve_prefix + '1') + ' fast_cgm.mif')
      run.command('mrconvert ' + fsl.find_image(fast_pve_prefix + '2') + ' fast_wm.mif')
      run.command('mrconvert ' + fsl.find_image(fast_pve_prefix + '0') + ' fast_csf.mif')
      app.cleanup(glob.glob(fast_pve_prefix + '*'))

      cgm_vol = 'fast_cgm.mif'
      wm_vol  = 'fast_wm.mif'
      csf_vol = 'fast_csf.mif'
    else:
      # Fall back to ODF-derived fractions from the initial masked 5TT
      run.command('mrconvert ' + result_masked + ' -coord 3 0 -axes 0,1,2 cgm_masked.mif')
      run.command('mrconvert ' + result_masked + ' -coord 3 2 -axes 0,1,2 wm_masked.mif')
      run.command('mrconvert ' + result_masked + ' -coord 3 3 -axes 0,1,2 csf_masked.mif')
      cgm_vol = 'cgm_masked.mif'
      wm_vol  = 'wm_masked.mif'
      csf_vol = 'csf_masked.mif'

    app.cleanup('vis.nii')

    # Zero out CGM, WM, CSF in SGM voxels so FIRST structures take priority
    if have_first:
      run.command('mrcalc ' + sgm_volume + ' 0 ' + cgm_vol + ' -if cgm_nosgm.mif')
      run.command('mrcalc ' + sgm_volume + ' 0 ' + wm_vol  + ' -if wm_nosgm.mif')
      run.command('mrcalc ' + sgm_volume + ' 0 ' + csf_vol + ' -if csf_nosgm.mif')
      cgm_vol = 'cgm_nosgm.mif'
      wm_vol  = 'wm_nosgm.mif'
      csf_vol = 'csf_nosgm.mif'

  else:
    # Default: use ODF-derived fractions directly from the masked 5TT
    run.command('mrconvert ' + result_masked + ' -coord 3 0 -axes 0,1,2 cgm_masked.mif')
    run.command('mrconvert ' + result_masked + ' -coord 3 2 -axes 0,1,2 wm_masked.mif')
    run.command('mrconvert ' + result_masked + ' -coord 3 3 -axes 0,1,2 csf_masked.mif')
    cgm_vol = 'cgm_masked.mif'
    wm_vol  = 'wm_masked.mif'
    csf_vol = 'csf_masked.mif'
    sgm_volume = empty_volume

  # CSF fill: add any deficit below 1.0 (within brain mask) into CSF,
  #   ensuring tissue fractions sum to exactly 1.0 in all brain voxels
  run.command('mrmath ' + cgm_vol + ' ' + sgm_volume + ' ' + wm_vol + ' ' + csf_vol + ' ' + empty_volume
              + ' sum tissuesum_prefill.mif')
  run.command('mrcalc 1.0 tissuesum_prefill.mif -sub '
              'tissuesum_prefill.mif 0.0 -gt brain_mask_derived.mif -add 1.0 -min -mult '
              '0.0 -max csf_fill.mif')
  run.command('mrcalc ' + csf_vol + ' csf_fill.mif -add csf_filled.mif')
  csf_vol = 'csf_filled.mif'
  app.cleanup('brain_mask_derived.mif')

  # Final assembly
  run.command('mrcat ' + cgm_vol + ' ' + sgm_volume + ' ' + wm_vol + ' ' + csf_vol + ' ' + empty_volume
              + ' result.mif -datatype float32')

  run.command('mrconvert result.mif ' + path.from_user(app.ARGS.output),
    force=app.FORCE_OVERWRITE)
