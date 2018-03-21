def initialise(base_parser, subparsers):
  import argparse
  from mrtrix3 import app
  parser = subparsers.add_parser('hsvs', author='Robert E. Smith (robert.smith@florey.edu.au)', synopsis='Generate a 5TT image based on Hybrid Surface and Volume Segmentation (HSVS), using FreeSurfer and FSL tools', parents=[base_parser])
  # TODO Permit either FreeSurfer directory or T1 image as input
  parser.add_argument('input',  help='The input FreeSurfer subject directory')
  parser.add_argument('output', help='The output 5TT image')
  # TODO Option to specify spatial resolution of output image?
  # Or just a template image; that way can control voxel size & axis orientations



def checkOutputPaths():
  pass



def getInputs():
  # FreeSurfer files will be accessed in-place; no need to pre-convert them into the temporary directory
  pass



def execute():
  import os, sys
  from mrtrix3 import app, fsl, path, run
  from distutils.spawn import find_executable

  def checkFile(filepath):
    import os
    if not os.path.isfile(filepath):
      app.error('Required input file missing (expected location: ' + filepath + ')')

  def checkDir(dirpath):
    import os
    if not os.path.isdir(dirpath):
      app.error('Unable to find sub-directory \'' + dirpath + '\' within input directory')

  subject_dir = os.path.abspath(path.fromUser(app.args.input, False))
  if not os.path.isdir(subject_dir):
    app.error('Input to hsvs algorithm must be a directory')
  surf_dir = os.path.join(subject_dir, 'surf')
  mri_dir = os.path.join(subject_dir, 'mri')
  checkDir(surf_dir)
  checkDir(mri_dir)
  aparc_image = os.path.join(mri_dir, 'aparc+aseg.mgz')
  mask_image = os.path.join(mri_dir, 'brainmask.mgz')
  reg_file = os.path.join(mri_dir, 'transforms', 'talairach.xfm')
  checkFile(aparc_image)
  checkFile(mask_image)
  checkFile(reg_file)

  sgm_first_map = { }
  have_first = True
  have_fast = True
  fsl_path = os.environ.get('FSLDIR', '')
  if fsl_path:
    # Use brain-extracted, bias-corrected image for FSL tools
    norm_image = os.path.join(mri_dir, 'norm.mgz')
    checkFile(norm_image)
    run.command('mrconvert ' + norm_image + ' T1.nii -stride -1,+2,+3')
    # Verify FAST availability
    fast_cmd = fsl.exeName('fast', False)
    if fast_cmd:
      if fast_cmd == 'fast':
        fast_suffix = fsl.suffix()
      else:
        fast_suffix = '.nii.gz'
    else:
      have_fast = False
      app.warn('Could not find FSL program fast; script will not use fast for cerebellar tissue segmentation')
    # Verify FIRST availability
    first_cmd = fsl.exeName('run_first_all', False)
    if first_cmd:
      first_atlas_path = os.path.join(fsl_path, 'data', 'first', 'models_336_bin')
      if os.path.isdir(first_atlas_path):
        sgm_first_map = { 'L_Accu':'Left-Accumbens-area',  'R_Accu':'Right-Accumbens-area', \
                          'L_Amyg':'Left-Amygdala',        'R_Amyg':'Right-Amygdala', \
                          'L_Caud':'Left-Caudate',         'R_Caud':'Right-Caudate', \
                          'L_Hipp':'Left-Hippocampus',     'R_Hipp':'Right-Hippocampus', \
                          'L_Pall':'Left-Pallidum',        'R_Pall':'Right-Pallidum', \
                          'L_Puta':'Left-Putamen',         'R_Puta':'Right-Putamen', \
                          'L_Thal':'Left-Thalamus-Proper', 'R_Thal':'Right-Thalamus-Proper' }
      else:
        app.warn('Atlases required for FSL\'s FIRST program not installed; script will proceed without using FIRST for sub-cortical grey matter segmentation')
    else:
      have_first = False
      app.warn('Could not find FSL program run_first_all; script will proceed without using FIRST for sub-cortical grey matter segmentation')
  else:
    have_first = have_fast = False
    app.warn('Environment variable FSLDIR is not set; script will run without FSL components')


  hipp_subfield_image_map = { os.path.join(mri_dir, 'lh.hippoSfLabels-T1.v10.mgz'): 'Left-Hippocampus',
                              os.path.join(mri_dir, 'rh.hippoSfLabels-T1.v10.mgz'): 'Right-Hippocampus' }

  if all(os.path.isfile(entry) for entry in hipp_subfield_image_map.keys()):
    progress = app.progressBar('Using detected FreeSurfer hippocampal subfields module output', 6)
    for filename, outputname in hipp_subfield_image_map.items():
      init_mesh_path = os.path.basename(filename)[0:2] + '_hipp_init.vtk'
      smooth_mesh_path = os.path.basename(filename)[0:2] + '_hipp_smooth.vtk'
      run.command('mrthreshold ' + filename + ' - -abs 0.5 | mrmesh - ' + init_mesh_path)
      progress.increment()
      # Since the hippocampal subfields segmentation can include some fine structures, reduce the extent of smoothing
      run.command('meshfilter ' + init_mesh_path + ' smooth ' + smooth_mesh_path + ' -smooth_spatial 5 -smooth_influence 5')
      progress.increment()
      run.command('mesh2voxel ' + smooth_mesh_path + ' ' + aparc_image + ' ' + outputname + '.mif')
      progress.increment()
    progress.done()
    # If we're going to be running FIRST, then we don't want to run it on the hippocampi
    if have_first:
      sgm_first_map = { key:value for key, value in sgm_first_map.items() if value not in hipp_subfield_image_map.values() }
  else:
    hipp_subfield_image_map = { }



  if have_first:
    app.console('Running FSL FIRST to segment sub-cortical grey matter structures')
    run.command(first_cmd + ' -s ' + ','.join(sgm_first_map.keys()) + ' -i T1.nii -b -o first')
    fsl.checkFirst('first', sgm_first_map.keys())
    progress = app.progressBar('Mapping sub-cortical structures segmented by FIRST from surface to voxel representation', len(sgm_first_map))
    for key, value in sgm_first_map.items():
      vtk_in_path = 'first-' + key + '_first.vtk'
      run.command('meshconvert ' + vtk_in_path + ' first-' + key + '_transformed.vtk -transform first2real T1.nii')
      run.command('mesh2voxel first-' + key + '_transformed.vtk ' + aparc_image + ' ' + value + '.mif')
      progress.increment()
    progress.done()



  # TODO If releasing, this should ideally read from FreeSurferColorLUT.txt to get the indices
  # TODO Currently mesh2voxel assumes a single closed surface;
  #   may need to run a connected component analysis first for some structures e.g. lesions
  # TODO Need to figure out a solution for identifying the vertices at the bottom of the
  #   brain stem, and labeling those as outside the brain, so that streamlines are permitted
  #   to terminate there.

  # Honour -sgm_amyg_hipp option
  ah = 1 if app.args.sgm_amyg_hipp else 0

  structures = [ ( 4,  3, 'Left-Lateral-Ventricle'),
                 ( 5,  3, 'Left-Inf-Lat-Vent'),
                 ( 6,  3, 'Left-Cerebellum-Exterior'),
                 ( 7,  2, 'Left-Cerebellum-White-Matter'),
                 ( 8,  1, 'Left-Cerebellum-Cortex'),
                 ( 9,  1, 'Left-Thalamus'),
                 (10,  1, 'Left-Thalamus-Proper'),
                 (11,  1, 'Left-Caudate'),
                 (12,  1, 'Left-Putamen'),
                 (13,  1, 'Left-Pallidum'),
                 (14,  3, '3rd-Ventricle'),
                 (15,  3, '4th-Ventricle'),
                 (16,  2, 'Brain-Stem'),
                 (17, ah, 'Left-Hippocampus'),
                 (18, ah, 'Left-Amygdala'),
                 (24,  3, 'CSF'),
                 (25,  4, 'Left-Lesion'),
                 (26,  1, 'Left-Accumbens-area'),
                 (27,  1, 'Left-Substancia-Nigra'),
                 (28,  2, 'Left-VentralDC'),
                 (30,  3, 'Left-vessel'),
                 (31,  1, 'Left-choroid-plexus'),
                 (43,  3, 'Right-Lateral-Ventricle'),
                 (44,  3, 'Right-Inf-Lat-Vent'),
                 (45,  3, 'Right-Cerebellum-Exterior'),
                 (46,  2, 'Right-Cerebellum-White-Matter'),
                 (47,  1, 'Right-Cerebellum-Cortex'),
                 (48,  1, 'Right-Thalamus'),
                 (49,  1, 'Right-Thalamus-Proper'),
                 (50,  1, 'Right-Caudate'),
                 (51,  1, 'Right-Putamen'),
                 (52,  1, 'Right-Pallidum'),
                 (53, ah, 'Right-Hippocampus'),
                 (54, ah, 'Right-Amygdala'),
                 (57,  4, 'Right-Lesion'),
                 (58,  1, 'Right-Accumbens-area'),
                 (59,  1, 'Right-Substancia-Nigra'),
                 (60,  2, 'Right-VentralDC'),
                 (62,  3, 'Right-vessel'),
                 (63,  1, 'Right-choroid-plexus'),
                 (72,  3, '5th-Ventricle'),
                 (192, 2, 'Corpus_Callosum'),
                 (250, 2, 'Fornix'),
                 # TODO Would rather combine CC segments into a single mask before converting to mesh
                 (251, 2, 'CC_Posterior'),
                 (252, 2, 'CC_Mid_Posterior'),
                 (253, 2, 'CC_Central'),
                 (254, 2, 'CC_Mid_Anterior'),
                 (255, 2, 'CC_Anterior') ]



  # Get the main cerebrum segments; these are already smooth
  # FIXME There may be some minor mismatch between the WM and pial segments within the medial section
  #   where no optimisation is performed, but vertices are simply defined in order to guarantee
  #   closed surfaces. Ideally these should be removed from the CGM tissue.
  progress = app.progressBar('Mapping FreeSurfer cortical reconstruction to partial volume images', 4)
  for hemi in [ 'lh', 'rh' ]:
    for basename in [ hemi+'.white', hemi+'.pial' ]:
      filepath = os.path.join(surf_dir, basename)
      checkFile(filepath)
      transformed_path = basename + '_realspace.obj'
      run.command('meshconvert ' + filepath + ' ' + transformed_path + ' -binary -transform fs2real ' + aparc_image)
      run.command('mesh2voxel ' + transformed_path + ' ' + aparc_image + ' ' + basename + '.mif')
      progress.increment()
  progress.done()

  # Get other structures that need to be converted from the voxel image
  progress = app.progressBar('Smoothing non-cortical structures segmented by FreeSurfer', len(structures))
  for (index, tissue, name) in structures:
    # Don't segment anything for which we have instead obtained estimates using FIRST
    # Also don't segment the hippocampi from the aparc+aseg image if we're using the hippocampal subfields module
    if not name in sgm_first_map.values() and not name in hipp_subfield_image_map.values():
      # If we're going to subsequently use fast, don't bother smoothing cerebellar segmentations;
      #   we're only going to use them to produce a mask anyway
      if 'Cerebellum' in name and have_fast:
        run.command('mrcalc ' + aparc_image + ' ' + str(index) + ' -eq ' + name + '.mif -datatype float32')
      else:
        run.command('mrcalc ' + aparc_image + ' ' + str(index) + ' -eq - | mrmesh - -threshold 0.5 ' + name + '_init.obj')
        run.command('meshfilter ' + name + '_init.obj smooth ' + name + '.obj')
        run.command('mesh2voxel ' + name + '.obj ' + aparc_image + ' ' + name + '.mif')
    progress.increment()
  progress.done()

  # Construct images with the partial volume of each tissue
  progress = app.progressBar('Combining segmentations of all structures corresponding to each tissue type', 5)
  for tissue in range(0,5):
    image_list = [ n + '.mif' for (i, t, n) in structures if t == tissue ]
    # For cortical GM and WM, need to also add the main cerebrum segments
    if tissue == 0:
      image_list.extend([ 'lh.pial.mif', 'rh.pial.mif' ])
    elif tissue == 2:
      image_list.extend([ 'lh.white.mif', 'rh.white.mif' ])
    run.command('mrmath ' + ' '.join(image_list) + ' sum - | mrcalc - 1.0 -min tissue' + str(tissue) + '_init.mif')
    progress.increment()
  progress.done()

  # TODO Need to fill in any potential gaps in the WM image in the centre of the brain
  # This can hopefully be done with a connected-component analysis: Take just the WM image, and
  #   fill in any gaps (i.e. select the inverse, select the largest connected component, invert again)
  # Make sure that floating-point values are handled appropriately
  # TODO Idea: Dilate voxelised brain stem 2 steps, add only intersection with voxelised WM,
  #   then run through mrmesh

  # Combine these images together using the appropriate logic in order to form the 5TT image
  progress = app.progressBar('Combining tissue images in appropriate manner to preserve 5TT format requirements', 14)
  run.command('mrconvert tissue4_init.mif tissue4.mif')
  progress.increment()
  run.command('mrcalc tissue3_init.mif tissue3_init.mif tissue4.mif -add 1.0 -sub 0.0 -max -sub tissue3.mif')
  progress.increment()
  run.command('mrmath tissue3.mif tissue4.mif sum tissuesum_34.mif')
  progress.increment()
  run.command('mrcalc tissue1_init.mif tissue1_init.mif tissuesum_34.mif -add 1.0 -sub 0.0 -max -sub tissue1.mif')
  progress.increment()
  run.command('mrmath tissuesum_34.mif tissue1.mif sum tissuesum_134.mif')
  progress.increment()
  run.command('mrcalc tissue2_init.mif tissue2_init.mif tissuesum_134.mif -add 1.0 -sub 0.0 -max -sub tissue2.mif')
  progress.increment()
  run.command('mrmath tissuesum_134.mif tissue2.mif sum tissuesum_1234.mif')
  progress.increment()
  run.command('mrcalc tissue0_init.mif tissue0_init.mif tissuesum_1234.mif -add 1.0 -sub 0.0 -max -sub tissue0.mif')
  progress.increment()
  run.command('mrmath tissuesum_1234.mif tissue0.mif sum tissuesum_01234.mif')
  progress.increment()

  # For all voxels within FreeSurfer's brain mask, add to the CSF image in order to make the sum 1.0
  # TESTME Rather than doing this blindly, look for holes in the brain, and assign the remainder to WM;
  #   only within the mask but outside the brain should the CSF fraction be filled
  # TODO Can definitely do better than just an erosion step here
  run.command('mrthreshold tissuesum_01234.mif -abs 0.5 - | maskfilter - erode - | mrcalc 1.0 - -sub - | maskfilter - connect -largest - | mrcalc 1.0 - -sub wm_fill_mask.mif')
  progress.increment()
  run.command('mrcalc 1.0 tissuesum_01234.mif -sub wm_fill_mask.mif -mult wm_fill.mif')
  progress.increment()
  run.command('mrcalc tissue2.mif wm_fill.mif -add tissue2_filled.mif')
  progress.increment()
  run.command('mrcalc 1.0 tissuesum_01234.mif wm_fill.mif -add -sub ' + mask_image + ' 0.0 -gt -mult csf_fill.mif')
  progress.increment()
  run.command('mrcalc tissue3.mif csf_fill.mif -add tissue3_filled.mif')
  progress.done()

  # Branch depending on whether or not FSL fast will be used to re-segment the cerebellum
  if have_fast:

    # Generate a mask of all voxels classified as cerebellum by FreeSurfer
    cerebellar_indices = [ i for (i, t, n) in structures if 'Cerebellum' in n ]
    cerebellar_submask_list = [ ]
    progress = app.progressBar('Generating whole-cerebellum mask from FreeSurfer segmentations', len(cerebellar_indices)+1)
    for index in cerebellar_indices:
      filename = 'Cerebellum_' + str(index) + '.mif'
      run.command('mrcalc ' + aparc_image + ' ' + str(index) + ' -eq ' + filename + ' -datatype bit')
      cerebellar_submask_list.append(filename)
      progress.increment()
    run.command('mrmath ' + ' '.join(cerebellar_submask_list) + ' sum Cerebellar_mask.mif')
    progress.done()

    app.console('Running FSL fast to segment the cerebellum based on intensity information')

    # FAST image input needs to be pre-masked
    run.command('mrcalc T1.nii Cerebellar_mask.mif -mult - | mrconvert - T1_cerebellum.nii -stride -1,+2,+3')

    # Run FSL FAST just within the cerebellum
    # TESTME Should bias field estimation be disabled within fast?
    run.command(fast_cmd + ' -N T1_cerebellum.nii')
    fast_output_prefix = 'T1_cerebellum'

    # Generate the revised tissue images, using output from FAST inside the cerebellum and
    #   output from previous processing everywhere else
    # Note that the middle intensity (grey matter) in the FAST output here gets assigned
    #   to the sub-cortical grey matter component
    progress = app.progressBar('Introducing intensity-based cerebellar segmentation into the 5TT image', 5)

    # Some of these voxels may have a non-zero cortical GM component.
    #   In that case, let's find a multiplier to apply to all tissues (including CGM) such that the sum is 1.0
    run.command('mrcalc 1.0 tissue0.mif 1.0 -add -div Cerebellar_mask.mif -mult Cerebellar_multiplier.mif')
    progress.increment()
    run.command('mrcalc Cerebellar_mask.mif Cerebellar_multiplier.mif 1.0 -if tissue0.mif -mult tissue0_fast.mif')
    progress.increment()
    run.command('mrcalc Cerebellar_mask.mif ' + fast_output_prefix + '_pve_0' + fast_suffix + ' Cerebellar_multiplier.mif -mult tissue3_filled.mif -if tissue3_filled_fast.mif')
    progress.increment()
    run.command('mrcalc Cerebellar_mask.mif ' + fast_output_prefix + '_pve_1' + fast_suffix + ' Cerebellar_multiplier.mif -mult tissue1.mif -if tissue1_fast.mif')
    progress.increment()
    run.command('mrcalc Cerebellar_mask.mif ' + fast_output_prefix + '_pve_2' + fast_suffix + ' Cerebellar_multiplier.mif -mult tissue2_filled.mif -if tissue2_filled_fast.mif')
    progress.done()

    # Finally, concatenate the volumes to produce the 5TT image
    run.command('mrcat tissue0_fast.mif tissue1_fast.mif tissue2_filled_fast.mif tissue3_filled_fast.mif tissue4.mif 5TT.mif')

  else:
    run.command('mrcat tissue0.mif tissue1.mif tissue2_filled.mif tissue3_filled.mif tissue4.mif 5TT.mif')

  # Maybe don't go off all tissues here, since FreeSurfer's mask can be fairly liberal;
  #   instead get just a voxel clearance from all other tissue types (maybe two)
  if app.args.nocrop:
    run.function(os.rename, '5TT.mif', 'result.mif')
  else:
    app.console('Cropping final 5TT image')
    run.command('mrconvert 5TT.mif -coord 3 0,1,2,4 - | mrmath - sum - -axis 3 | mrthreshold - - -abs 0.001 | maskfilter - dilate crop_mask.mif')
    run.command('mrcrop 5TT.mif result.mif -mask crop_mask.mif')



  app.warn('Script algorithm is not yet capable of performing requisite image modifications in order to '
           'permit streamlines travelling from the brain stem down the spinal column. Recommend using '
           '5ttedit -none, in conjunction with a manually-drawn ROI labelling the bottom part of the '
           'brain stem, such that streamlines in this region are characterised by ACT as exiting the image.')
