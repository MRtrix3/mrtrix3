def initParser(subparsers, base_parser):
  import argparse  
  parser = subparsers.add_parser('fsl', parents=[base_parser], help='Use FSL commands to generate the 5TT image based on a T1-weighted image')
  options = parser.add_argument_group('options specific to the \'fsl\' algorithm')
  masking = options.add_mutually_exclusive_group()
  masking.add_argument('-mask', help='Manually provide a brain mask, rather than deriving one in the script')
  masking.add_argument('-premasked', action='store_true', default=False, help='Indicate that brain masking has already been applied to the input image')
  parser.set_defaults(algorithm='fsl')
  
  
  
def checkOutputFiles():
  pass



def getInputFiles():
  import os
  import lib.app
  from lib.runCommand   import runCommand
  if hasattr(lib.app.args, 'mask') and lib.app.args.mask is not None:
    runCommand('mrconvert ' + lib.app.args.mask + ' ' + os.path.join(lib.app.tempDir, 'mask.mif') + ' -datatype bit -stride +1,+2,+3')



def execute():
  import os
  import lib.app
  from lib.binaryInPath  import binaryInPath
  from lib.errorMessage  import errorMessage
  from lib.getFSLSuffix  import getFSLSuffix
  from lib.getHeaderInfo import getHeaderInfo
  from lib.isWindows     import isWindows
  from lib.runCommand    import runCommand
  from lib.warnMessage   import warnMessage
  
  if isWindows():
    errorMessage('Script cannot run in FSL mode on Windows')

  fsl_path = os.environ.get('FSLDIR', '')
  if not fsl_path:
    errorMessage('Environment variable FSLDIR is not set; please run appropriate FSL configuration script')

  ssroi_cmd = 'standard_space_roi'
  if not binaryInPath(ssroi_cmd):
    ssroi_cmd = 'fsl5.0-standard_space_roi'
    if not binaryInPath(ssroi_cmd):
      errorMessage('Could not find FSL program standard_space_roi; please verify FSL install')

  bet_cmd = 'bet'
  if not binaryInPath(bet_cmd):
    bet_cmd = 'fsl5.0-bet'
    if not binaryInPath(bet_cmd):
      errorMessage('Could not find FSL program bet; please verify FSL install')

  fast_cmd = 'fast'
  if not binaryInPath(fast_cmd):
    fast_cmd = 'fsl5.0-fast'
    if not binaryInPath(fast_cmd):
      errorMessage('Could not find FSL program fast; please verify FSL install')

  first_cmd = 'run_first_all'
  if not binaryInPath(first_cmd):
    first_cmd = "fsl5.0-run_first_all"
    if not binaryInPath(first_cmd):
      errorMessage('Could not find FSL program run_first_all; please verify FSL install')

  first_atlas_path = os.path.join(fsl_path, 'data', 'first', 'models_336_bin')

  if not os.path.isdir(first_atlas_path):
    errorMessage('Atlases required for FSL\'s FIRST program not installed;\nPlease install fsl-first-data using your relevant package manager')

  fsl_suffix = getFSLSuffix()

  sgm_structures = [ 'L_Accu', 'R_Accu', 'L_Caud', 'R_Caud', 'L_Pall', 'R_Pall', 'L_Puta', 'R_Puta', 'L_Thal', 'R_Thal' ]
  
  runCommand('mrconvert input.mif T1.nii')  

  # Decide whether or not we're going to do any brain masking
  if os.path.exists('mask.mif'):
    
    # Check to see if the dimensions match the T1 image
    T1_size = getHeaderInfo('input.mif', 'size')
    mask_size = getHeaderInfo('mask.mif', 'size')
    if mask_size == T1_size:
      runCommand('mrcalc input.mif mask.mif -mult T1_masked.nii')
    else:
      runCommand('mrtransform mask.mif mask_regrid.mif -template input.mif')
      runCommand('mrcalc input.mif mask_regrid.mif -mult T1_masked.nii')
      
  elif lib.app.args.premasked:
  
    runCommand('mrconvert input.mif T1_masked.nii')
    
  else:

    # Use FSL command standard_space_roi to do an initial masking of the image before BET
    # Also reduce the FoV of the image
    # Using MNI 1mm dilated brain mask rather than the -b option in standard_space_roi (which uses the 2mm mask); the latter looks 'buggy' to me... Unfortunately even with the 1mm 'dilated' mask, it can still cut into some brain areas, hence the explicit dilation
    mni_mask_path = os.path.join(fsl_path, 'data', 'standard', 'MNI152_T1_1mm_brain_mask_dil.nii.gz')
    mni_mask_dilation = 0;
    if os.path.exists (mni_mask_path):
      mni_mask_dilation = 4;
    else:
      mni_mask_path = os.path.join(fsl_path, 'data', 'standard', 'MNI152_T1_2mm_brain_mask_dil.nii.gz')
      if os.path.exists (mni_mask_path):
        mni_mask_dilation = 2;

    # standard_space_roi can sometimes crash; if this happens, try to allow the script to continue
    if mni_mask_dilation:
      runCommand('maskfilter ' + mni_mask_path + ' dilate mni_mask.nii -npass ' + str(mni_mask_dilation))
      if lib.app.args.nocrop:
        ssroi_roi_option = ' -roiNONE'
      else:
        ssroi_roi_option = ' -roiFOV'
      runCommand(ssroi_cmd + ' T1.nii T1_preBET' + fsl_suffix + ' -maskMASK mni_mask.nii' + ssroi_roi_option, False)
    else:
      runCommand(ssroi_cmd + ' T1.nii T1_preBET' + fsl_suffix + ' -b', False)

    if not os.path.isfile('T1_preBET' + fsl_suffix):
      warnMessage('FSL command ' + ssroi_cmd + ' appears to have failed; passing T1 directly to BET')
      runCommand('mrconvert input.mif T1_preBET' + fsl_suffix)

    # BET
    runCommand(bet_cmd + ' T1_preBET' + fsl_suffix + ' T1_masked' + fsl_suffix + ' -f 0.15 -R')

  # Finish branching based on brain masking  

  # FAST
  runCommand(fast_cmd + ' T1_masked' + fsl_suffix)

  # FIRST
  first_input_is_brain_extracted = ''
  if lib.app.args.premasked:
    first_input_is_brain_extracted = ' -b'
  runCommand(first_cmd + ' -s ' + ','.join(sgm_structures) + ' -i T1.nii -o first' + first_input_is_brain_extracted)

  # Convert FIRST meshes to partial volume images
  pve_image_list = [ ]
  for struct in sgm_structures:
    pve_image_path = 'mesh2pve_' + struct + '.nii'
    vtk_in_path = 'first-' + struct + '_first.vtk'
    vtk_temp_path = struct + '.vtk'
    if not os.path.exists(vtk_in_path):
      errorMessage('Missing .vtk file for structure ' + struct + '; run_first_all must have failed')
    runCommand('meshconvert ' + vtk_in_path + ' ' + vtk_temp_path + ' -transform_first2real input.mif')
    runCommand('mesh2pve ' + vtk_temp_path + ' T1_preBET' + fsl_suffix + ' ' + pve_image_path)
    pve_image_list.append(pve_image_path)
  pve_cat = ' '.join(pve_image_list)
  runCommand('mrmath ' + pve_cat + ' sum - | mrcalc - 1.0 -min all_sgms.nii')

  # Looks like FAST in 5.0 ignores FSLOUTPUTTYPE when writing the PVE images
  # Will have to wait and see whether this changes, and update the script accordingly
  if fast_cmd == 'fast':
    fast_suffix = fsl_suffix
  else:
    fast_suffix = '.nii.gz'

  # Combine the tissue images into the 5TT format within the script itself
  # Step 1: Run LCC on the WM image
  runCommand('mrthreshold T1_masked_pve_2' + fast_suffix + ' - -abs 0.001 | maskfilter - connect wm_mask.mif -largest')
  # Step 2: Generate the images in the same fashion as the 5ttgen command
  runCommand('mrconvert T1_masked_pve_0' + fast_suffix + ' csf.mif')
  runCommand('mrcalc 1 csf.mif -sub all_sgms.nii -min sgm.mif')
  runCommand('mrcalc 1.0 csf.mif sgm.mif -add -sub T1_masked_pve_1' + fast_suffix + ' T1_masked_pve_2' + fast_suffix + ' -add -div multiplier.mif')
  runCommand('mrcalc multiplier.mif -finite multiplier.mif 0.0 -if multiplier_noNAN.mif')
  runCommand('mrcalc T1_masked_pve_1' + fast_suffix + ' multiplier_noNAN.mif -mult cgm.mif')
  runCommand('mrcalc T1_masked_pve_2' + fast_suffix + ' multiplier_noNAN.mif -mult wm_mask.mif -mult wm.mif')
  runCommand('mrcalc 0 wm.mif -min path.mif')
  runCommand('mrcat cgm.mif sgm.mif wm.mif csf.mif path.mif - -axis 3 | mrconvert - combined_precrop.mif -stride +2,+3,+4,+1')

  # Use mrcrop to reduce file size (improves caching of image data during tracking)
  if lib.app.args.nocrop:
    runCommand('mrconvert combined_precrop.mif result.mif')
  else:
    runCommand('mrmath combined_precrop.mif sum - -axis 3 | mrthreshold - - -abs 0.5 | mrcrop combined_precrop.mif result.mif -mask -')

