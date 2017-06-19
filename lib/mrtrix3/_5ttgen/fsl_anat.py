def initialise(base_parser, subparsers):
  import argparse
  from mrtrix3 import app
  parser = subparsers.add_parser('fsl_anat', author='Robert E. Smith (robert.smith@florey.edu.au)', synopsis='Use FSL script \'fsl_anat\' to generate the 5TT image from a T1-weighted image', parents=[base_parser])
  parser.addCitation('', 'Smith, S. M. Fast robust automated brain extraction. Human Brain Mapping, 2002, 17, 143-155', True)
  parser.addCitation('', 'Zhang, Y.; Brady, M. & Smith, S. Segmentation of brain MR images through a hidden Markov random field model and the expectation-maximization algorithm. IEEE Transactions on Medical Imaging, 2001, 20, 45-57', True)
  parser.addCitation('', 'Patenaude, B.; Smith, S. M.; Kennedy, D. N. & Jenkinson, M. A Bayesian model of shape and appearance for subcortical brain segmentation. NeuroImage, 2011, 56, 907-922', True)
  parser.addCitation('', 'Smith, S. M.; Jenkinson, M.; Woolrich, M. W.; Beckmann, C. F.; Behrens, T. E.; Johansen-Berg, H.; Bannister, P. R.; De Luca, M.; Drobnjak, I.; Flitney, D. E.; Niazy, R. K.; Saunders, J.; Vickers, J.; Zhang, Y.; De Stefano, N.; Brady, J. M. & Matthews, P. M. Advances in functional and structural MR image analysis and implementation as FSL. NeuroImage, 2004, 23, S208-S219', True)
  parser.add_argument('input',  help='The input T1-weighted image')
  parser.add_argument('output', help='The output 5TT image')
  options = parser.add_argument_group('Options specific to the \'fsl_anat\' algorithm')
  options.add_argument('-fsl_anat_options', metavar='\"Options\"', help='Manually provide command-line options to the fsl_anat script')



def checkOutputPaths():
  from mrtrix3 import app
  app.checkOutputPath(app.args.output)



def getInputs():
  import os
  from mrtrix3 import app, image, path, run
  image.check3DNonunity(path.fromUser(app.args.input, False))
  run.command('mrconvert ' + path.fromUser(app.args.input, True) + ' ' + path.toTemp('T1.nii', True) + ' -stride -1,+2,+3')



def execute():
  import os
  from distutils.spawn import find_executable
  from mrtrix3 import app, file, fsl, image, run

  if app.isWindows():
    app.error('\'fsl_anat\' algorithm of 5ttgen script cannot be run on Windows: FSL not available on Windows')

  fsl_path = os.environ.get('FSLDIR', '')
  if not fsl_path:
    app.error('Environment variable FSLDIR is not set; please run appropriate FSL configuration script')

  fsl_anat_cmd = 'fsl_anat'
  if not find_executable(fsl_anat_cmd):
    fsl_anat_cmd = "fsl5.0-fsl_anat"
    if not find_executable(fsl_anat_cmd):
      app.error('Could not find FSL script fsl_anat; please verify FSL install')

  first_atlas_path = os.path.join(fsl_path, 'data', 'first', 'models_336_bin')
  if not os.path.isdir(first_atlas_path):
    app.error('Atlases required for FSL\'s FIRST program not installed; please install fsl-first-data using your relevant package manager')

  fsl_suffix = fsl.suffix()

  sgm_structures = [ 'L_Accu', 'R_Accu', 'L_Caud', 'R_Caud', 'L_Pall', 'R_Pall', 'L_Puta', 'R_Puta', 'L_Thal', 'R_Thal' ]
  if app.args.sgm_amyg_hipp:
    sgm_structures.extend([ 'L_Amyg', 'R_Amyg', 'L_Hipp', 'R_Hipp' ])

  # Run the fsl_anat script
  run.command(fsl_anat_cmd + ' -i T1.nii')
  anat_dir = 'T1.anat'
  first_subdir = 'first_results'

  # fsl_anat includes cropping the inferior edge of the FoV
  # Therefore need an appropriate template image to use in mesh conversions
  mesh_template = os.path.join(anat_dir, 'T1_biascorr' + fsl_suffix)

  # Convert FIRST meshes to partial volume images
  pve_image_list = [ ]
  for struct in sgm_structures:
    pve_image_path = 'mesh2pve_' + struct + '.mif'
    vtk_in_path = os.path.join(anat_dir, first_subdir, 'T1_first-' + struct + '_first.vtk')
    vtk_temp_path = struct + '.vtk'
    # If SGE is used, run_first_all may return without error even though
    #   the output files haven't actually been created yet
    file.waitFor(vtk_in_path)
    run.command('meshconvert ' + vtk_in_path + ' ' + vtk_temp_path + ' -transform first2real ' + mesh_template)
    run.command('mesh2pve ' + vtk_temp_path + ' ' + mesh_template + ' ' + pve_image_path)
    pve_image_list.append(pve_image_path)
  pve_cat = ' '.join(pve_image_list)
  run.command('mrmath ' + pve_cat + ' sum - | mrcalc - 1.0 -min all_sgms.mif')

  # Combine the tissue images into the 5TT format within the script itself
  # Note: No longer doing the WM LCC filter, just leads to issues
  # Could alternatively get the mask, mask it from WM, then just add it to CSF?
  fast_csf_path = os.path.join(anat_dir, 'T1_fast_pve_0' + fsl_suffix)
  fast_gm_path  = os.path.join(anat_dir, 'T1_fast_pve_1' + fsl_suffix)
  fast_wm_path  = os.path.join(anat_dir, 'T1_fast_pve_2' + fsl_suffix)

  run.command('mrconvert ' + fast_csf_path + ' csf.mif')
  run.command('mrcalc 1.0 csf.mif -sub all_sgms.mif -min sgm.mif')
  run.command('mrcalc 1.0 csf.mif sgm.mif -add -sub ' + fast_gm_path + ' ' + fast_wm_path + ' -add -div multiplier.mif')
  run.command('mrcalc multiplier.mif -finite multiplier.mif 0.0 -if multiplier_noNAN.mif')
  run.command('mrcalc ' + fast_gm_path + ' multiplier_noNAN.mif -mult cgm.mif')
  run.command('mrcalc ' + fast_wm_path + ' multiplier_noNAN.mif -mult wm.mif')
  run.command('mrcalc 0 wm.mif -min path.mif')
  run.command('mrcat cgm.mif sgm.mif wm.mif csf.mif path.mif - -axis 3 | mrconvert - combined_precrop.mif -stride +2,+3,+4,+1')

  # Use mrcrop to reduce file size (improves caching of image data during tracking)
  if app.args.nocrop:
    # Since fsl_anat itself performs some cropping, need to perform an explicit re-gridding back to the original image FoV
    run.command('mrtransform combined_precrop.mif result.mif -template T1.nii -interp linear')
  else:
    run.command('mrmath combined_precrop.mif sum - -axis 3 | mrthreshold - - -abs 0.5 | mrcrop combined_precrop.mif result.mif -mask -')

