def initParser(subparsers, base_parser):
  import argparse
  import lib.app
  import lib.cmdlineParser
  parser = subparsers.add_parser('fsl', parents=[base_parser], add_help=False, description='Use FSL commands to generate the 5TT image based on a T1-weighted image')
  lib.app.addCitation('If using \'fsl\' algorithm', 'Smith, S. M. Fast robust automated brain extraction. Human Brain Mapping, 2002, 17, 143-155', True)
  lib.app.addCitation('If using \'fsl\' algorithm', 'Zhang, Y.; Brady, M. & Smith, S. Segmentation of brain MR images through a hidden Markov random field model and the expectation-maximization algorithm. IEEE Transactions on Medical Imaging, 2001, 20, 45-57', True)
  lib.app.addCitation('If using \'fsl\' algorithm', 'Patenaude, B.; Smith, S. M.; Kennedy, D. N. & Jenkinson, M. A Bayesian model of shape and appearance for subcortical brain segmentation. NeuroImage, 2011, 56, 907-922', True)
  lib.app.addCitation('If using \'fsl\' algorithm', 'Smith, S. M.; Jenkinson, M.; Woolrich, M. W.; Beckmann, C. F.; Behrens, T. E.; Johansen-Berg, H.; Bannister, P. R.; De Luca, M.; Drobnjak, I.; Flitney, D. E.; Niazy, R. K.; Saunders, J.; Vickers, J.; Zhang, Y.; De Stefano, N.; Brady, J. M. & Matthews, P. M. Advances in functional and structural MR image analysis and implementation as FSL. NeuroImage, 2004, 23, S208-S219', True)
  parser.add_argument('input',  help='The input T1-weighted image')
  parser.add_argument('output', help='The output 5TT image')
  options = parser.add_argument_group('Options specific to the \'fsl\' algorithm')
  options.add_argument('-t2', metavar='<T2 image>', help='Provide a T2-weighted image in addition to the default T1-weighted image; this will be used as a second input to FSL FAST')
  options.add_argument('-mask', help='Manually provide a brain mask, rather than deriving one in the script')
  options.add_argument('-premasked', action='store_true', help='Indicate that brain masking has already been applied to the input image')
  lib.cmdlineParser.flagMutuallyExclusiveOptions( [ 'mask', 'premasked' ] )
  parser.set_defaults(algorithm='fsl')
  
  
  
def checkOutputFiles():
  pass



def getInputFiles():
  import os
  import lib.app
  from lib.imagesMatch   import imagesMatch
  from lib.errorMessage  import errorMessage
  from lib.getHeaderInfo import getHeaderInfo
  from lib.getUserPath   import getUserPath
  from lib.runCommand    import runCommand
  if lib.app.args.mask:
    runCommand('mrconvert ' + getUserPath(lib.app.args.mask, True) + ' ' + os.path.join(lib.app.tempDir, 'mask.mif') + ' -datatype bit -stride -1,+2,+3')
  if lib.app.args.t2:
    if not imagesMatch(lib.app.args.input, lib.app.args.t2):
      errorMessage('Provided T2 image does not match input T1 image')
    runCommand('mrconvert ' + getUserPath(lib.app.args.t2, True) + ' ' + os.path.join(lib.app.tempDir, 'T2.nii') + ' -stride -1,+2,+3')




def execute():
  import os
  import lib.app
  from lib.binaryInPath  import binaryInPath
  from lib.errorMessage  import errorMessage
  from lib.getFSLSuffix  import getFSLSuffix
  from lib.getHeaderInfo import getHeaderInfo
  from lib.imagesMatch   import imagesMatch
  from lib.isWindows     import isWindows
  from lib.runCommand    import runCommand
  from lib.warnMessage   import warnMessage
  
  if isWindows():
    errorMessage('\'fsl\' algorithm of 5ttgen script cannot be run on Windows: FSL not available on Windows')

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
  if lib.app.args.sgm_amyg_hipp:
    sgm_structures.extend([ 'L_Amyg', 'R_Amyg', 'L_Hipp', 'R_Hipp' ])
  
  runCommand('mrconvert input.mif T1.nii -stride -1,+2,+3')

  fast_t1_input = 'T1.nii'
  fast_t2_input = ''

  # Decide whether or not we're going to do any brain masking
  if os.path.exists('mask.mif'):

    fast_t1_input = 'T1_masked' + fsl_suffix
    
    # Check to see if the mask matches the T1 image
    if imagesMatch('T1.nii', 'mask.mif'):
      runCommand('mrcalc T1.nii mask.mif -mult ' + fast_t1_input)
      mask_path = 'mask.mif'
    else:
      warnMessage('Mask image does not match input image - re-gridding')
      runCommand('mrtransform mask.mif mask_regrid.mif -template T1.nii')
      runCommand('mrcalc T1.nii mask_regrid.mif ' + fast_t1_input)
      mask_path = 'mask_regrid.mif'

    if os.path.exists('T2.nii'):
      fast_t2_input = 'T2_masked' + fsl_suffix
      runCommand('mrcalc T2.nii ' + mask_path + ' -mult ' + fast_t2_input)
      
  elif lib.app.args.premasked:
  
    fast_t1_input = 'T1.nii'
    if os.path.exists('T2.nii'):
      fast_t2_input = 'T2.nii'
    
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

    if not os.path.exists('T1_preBET' + fsl_suffix):
      warnMessage('FSL command ' + ssroi_cmd + ' appears to have failed; passing T1 directly to BET')
      runCommand('mrconvert input.mif T1_preBET' + fsl_suffix + ' -stride -1,+2,+3')

    # BET
    fast_t1_input = 'T1_BET' + fsl_suffix
    runCommand(bet_cmd + ' T1_preBET' + fsl_suffix + ' ' + fast_t1_input + ' -f 0.15 -R')

    if os.path.exists('T2.nii'):
      if lib.app.args.nocrop:
        fast_t2_input = 'T2.nii'
      else:
        # Just a reduction of FoV, no sub-voxel interpolation going on
        runCommand('mrtransform T2.nii T2_cropped.nii -template ' + fast_t1_input + ' -interp nearest')
        fast_t2_input = 'T2_cropped.nii'

  # Finish branching based on brain masking

  # FAST
  if fast_t2_input:
    runCommand(fast_cmd + ' -S 2 ' + fast_t2_input + ' ' + fast_t1_input)
  else:
    runCommand(fast_cmd + ' ' + fast_t1_input)
  fast_output_prefix = fast_t1_input.split('.')[0]

  # FIRST
  first_input_is_brain_extracted = ''
  if lib.app.args.premasked:
    first_input_is_brain_extracted = ' -b'
  runCommand(first_cmd + ' -s ' + ','.join(sgm_structures) + ' -i T1.nii -o first' + first_input_is_brain_extracted)

  # Convert FIRST meshes to partial volume images
  pve_image_list = [ ]
  for struct in sgm_structures:
    pve_image_path = 'mesh2pve_' + struct + '.mif'
    vtk_in_path = 'first-' + struct + '_first.vtk'
    vtk_temp_path = struct + '.vtk'
    if not os.path.exists(vtk_in_path):
      errorMessage('Missing .vtk file for structure ' + struct + '; run_first_all must have failed')
    runCommand('meshconvert ' + vtk_in_path + ' ' + vtk_temp_path + ' -transform first2real T1.nii')
    runCommand('mesh2pve ' + vtk_temp_path + ' ' + fast_t1_input + ' ' + pve_image_path)
    pve_image_list.append(pve_image_path)
  pve_cat = ' '.join(pve_image_list)
  runCommand('mrmath ' + pve_cat + ' sum - | mrcalc - 1.0 -min all_sgms.mif')

  # Looks like FAST in 5.0 ignores FSLOUTPUTTYPE when writing the PVE images
  # Will have to wait and see whether this changes, and update the script accordingly
  if fast_cmd == 'fast':
    fast_suffix = fsl_suffix
  else:
    fast_suffix = '.nii.gz'

  # Combine the tissue images into the 5TT format within the script itself
  # Step 1: Run LCC on the WM image
  runCommand('mrthreshold ' + fast_output_prefix + '_pve_2' + fast_suffix + ' - -abs 0.001 | maskfilter - connect - -connectivity | mrcalc 1 - 1 -gt -sub remove_unconnected_wm_mask.mif -datatype bit')
  # Step 2: Generate the images in the same fashion as the 5ttgen command
  runCommand('mrcalc ' + fast_output_prefix + '_pve_0' + fast_suffix + ' remove_unconnected_wm_mask.mif -mult csf.mif')
  runCommand('mrcalc 1.0 csf.mif -sub all_sgms.mif -min sgm.mif')
  runCommand('mrcalc 1.0 csf.mif sgm.mif -add -sub ' + fast_output_prefix + '_pve_1' + fast_suffix + ' ' + fast_output_prefix + '_pve_2' + fast_suffix + ' -add -div multiplier.mif')
  runCommand('mrcalc multiplier.mif -finite multiplier.mif 0.0 -if multiplier_noNAN.mif')
  runCommand('mrcalc ' + fast_output_prefix + '_pve_1' + fast_suffix + ' multiplier_noNAN.mif -mult remove_unconnected_wm_mask.mif -mult cgm.mif')
  runCommand('mrcalc ' + fast_output_prefix + '_pve_2' + fast_suffix + ' multiplier_noNAN.mif -mult remove_unconnected_wm_mask.mif -mult wm.mif')
  runCommand('mrcalc 0 wm.mif -min path.mif')
  runCommand('mrcat cgm.mif sgm.mif wm.mif csf.mif path.mif - -axis 3 | mrconvert - combined_precrop.mif -stride +2,+3,+4,+1')

  # Use mrcrop to reduce file size (improves caching of image data during tracking)
  if lib.app.args.nocrop:
    runCommand('mrconvert combined_precrop.mif result.mif')
  else:
    runCommand('mrmath combined_precrop.mif sum - -axis 3 | mrthreshold - - -abs 0.5 | mrcrop combined_precrop.mif result.mif -mask -')

