def initialise(base_parser, subparsers):
  parser = subparsers.add_parser('hsvs', author='Robert E. Smith (robert.smith@florey.edu.au)', synopsis='Generate a 5TT image based on Hybrid Surface and Volume Segmentation (HSVS), using FreeSurfer and FSL tools', parents=[base_parser])
  # TODO Permit either FreeSurfer directory or T1 image as input
  parser.add_argument('input',  help='The input FreeSurfer subject directory')
  parser.add_argument('output', help='The output 5TT image')
  # TESTME Option to specify spatial resolution of output image?
  # Or just a template image; that way can control voxel size & axis orientations
  parser.add_argument('-template', help='Provide an image that will form the template for the generated 5TT image')
  # TODO Add references


# TODO Add file.delTemporary() throughout



def checkOutputPaths():
  pass



def getInputs():
  from mrtrix3 import app, path, run
  # FreeSurfer files will be accessed in-place; no need to pre-convert them into the temporary directory
  # TODO Pre-convert aparc image so that it doesn't have to be repeatedly uncompressed
  if app.args.template:
    run.command('mrconvert ' + path.fromUser(app.args.template, True) + ' ' + path.toTemp('template.mif', True) + ' -axes 0,1,2')



def execute():
  import glob, os, sys
  from mrtrix3 import app, file, fsl, path, run

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
  template_image = 'template.mif' if app.args.template else aparc_image

  if app.args.template:
    run.command('mrtransform ' + mask_image + ' -template template.mif - | mrthreshold - brainmask.mif -abs 0.5')
    mask_image = 'brainmask.mif'

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
      file.delTemporary(init_mesh_path)
      progress.increment()
      run.command('mesh2voxel ' + smooth_mesh_path + ' ' + template_image + ' ' + outputname + '.mif')
      file.delTemporary(smooth_mesh_path)
      progress.increment()
    progress.done()
    # If we're going to be running FIRST, then we don't want to run it on the hippocampi
    if have_first:
      sgm_first_map = { key:value for key, value in sgm_first_map.items() if value not in hipp_subfield_image_map.values() }
  else:
    app.console('No FreeSurfer hippocampal subfields module output detected; ' + ('FIRST segmentation' if have_first else 'standard FreeSurfer segmentation') + ' will instead be used')
    hipp_subfield_image_map = { }



  if have_first:
    app.console('Running FSL FIRST to segment sub-cortical grey matter structures')
    run.command(first_cmd + ' -s ' + ','.join(sgm_first_map.keys()) + ' -i T1.nii -b -o first')
    fsl.checkFirst('first', sgm_first_map.keys())
    file.delTemporary(glob.glob('T1_to_std_sub.*'))
    progress = app.progressBar('Mapping sub-cortical structures segmented by FIRST from surface to voxel representation', 2*len(sgm_first_map))
    for key, value in sgm_first_map.items():
      vtk_in_path = 'first-' + key + '_first.vtk'
      vtk_converted_path = 'first-' + key + '_transformed.vtk'
      run.command('meshconvert ' + vtk_in_path + ' ' + vtk_converted_path + ' -transform first2real T1.nii')
      file.delTemporary(vtk_in_path)
      progress.increment()
      run.command('mesh2voxel ' + vtk_converted_path + ' ' + template_image + ' ' + value + '.mif')
      file.delTemporary(vtk_converted_path)
      progress.increment()
    if not have_fast:
      file.delTemporary('T1.nii')
    file.delTemporary(glob.glob('first*'))
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
                 # TODO Need to do something about anterior commissure



  # Get the main cerebrum segments; these are already smooth
  # FIXME There may be some minor mismatch between the WM and pial segments within the medial section
  #   where no optimisation is performed, but vertices are simply defined in order to guarantee
  #   closed surfaces. Ideally these should be removed from the CGM tissue.
  progress = app.progressBar('Mapping FreeSurfer cortical reconstruction to partial volume images', 8)
  for hemi in [ 'lh', 'rh' ]:
    for basename in [ hemi+'.white', hemi+'.pial' ]:
      filepath = os.path.join(surf_dir, basename)
      checkFile(filepath)
      transformed_path = basename + '_realspace.obj'
      run.command('meshconvert ' + filepath + ' ' + transformed_path + ' -binary -transform fs2real ' + aparc_image)
      progress.increment()
      run.command('mesh2voxel ' + transformed_path + ' ' + template_image + ' ' + basename + '.mif')
      file.delTemporary(transformed_path)
      progress.increment()
  progress.done()

  # Get other structures that need to be converted from the voxel image
  progress = app.progressBar('Smoothing non-cortical structures segmented by FreeSurfer', len(structures))
  for (index, tissue, name) in structures:
    # Don't segment anything for which we have instead obtained estimates using FIRST
    # Also don't segment the hippocampi from the aparc+aseg image if we're using the hippocampal subfields module
    if not name in sgm_first_map.values() and not name in hipp_subfield_image_map.values():
      # If we're going to subsequently use fast directly on the FreeSurfer T1 image,
      #   don't bother smoothing cerebellar segmentations; we're only going to use
      #   them to produce a mask anyway
      # FIXME This will still be included in the list of images to be combined;
      #   if a template image is being used, this will lead to heartache due to image grid mismatch
      if 'Cerebellum' in name and have_fast:
        run.command('mrcalc ' + aparc_image + ' ' + str(index) + ' -eq ' + name + '.mif -datatype float32')
      else:
        init_mesh_path = name + '_init.vtk'
        smoothed_mesh_path = name + '.vtk'
        run.command('mrcalc ' + aparc_image + ' ' + str(index) + ' -eq - | mrmesh - -threshold 0.5 ' + init_mesh_path)
        run.command('meshfilter ' + init_mesh_path + ' smooth ' + smoothed_mesh_path)
        file.delTemporary(init_mesh_path)
        run.command('mesh2voxel ' + smoothed_mesh_path + ' ' + template_image + ' ' + name + '.mif')
        file.delTemporary(smoothed_mesh_path)
    progress.increment()
  progress.done()

  # Construct images with the partial volume of each tissue
  progress = app.progressBar('Combining segmentations of all structures corresponding to each tissue type', 5)
  for tissue in range(0,5):
    image_list = [ n + '.mif' for (i, t, n) in structures if t == tissue and not (have_fast and 'Cerebellum' in n) ]
    # For cortical GM and WM, need to also add the main cerebrum segments
    if tissue == 0:
      image_list.extend([ 'lh.pial.mif', 'rh.pial.mif' ])
    elif tissue == 2:
      image_list.extend([ 'lh.white.mif', 'rh.white.mif' ])
    run.command('mrmath ' + ' '.join(image_list) + ' sum - | mrcalc - 1.0 -min tissue' + str(tissue) + '_init.mif')
    # TODO Update file.delTemporary() to support list input
    for entry in image_list:
      file.delTemporary(entry)
    progress.increment()
  progress.done()


  # This can hopefully be done with a connected-component analysis: Take just the WM image, and
  #   fill in any gaps (i.e. select the inverse, select the largest connected component, invert again)
  # Make sure that floating-point values are handled appropriately
  # TODO Idea: Dilate voxelised brain stem 2 steps, add only intersection with voxelised WM,
  #   then run through mrmesh

  # Combine these images together using the appropriate logic in order to form the 5TT image
  progress = app.progressBar('Modulating segmentation images based on other tissues', 9)
  tissue_images = [ 'tissue0.mif', 'tissue1.mif', 'tissue2.mif', 'tissue3.mif', 'tissue4.mif' ]
  run.function(os.rename, 'tissue4_init.mif', 'tissue4.mif')
  progress.increment()
  run.command('mrcalc tissue3_init.mif tissue3_init.mif ' + tissue_images[4] + ' -add 1.0 -sub 0.0 -max -sub ' + tissue_images[3])
  file.delTemporary('tissue3_init.mif')
  progress.increment()
  run.command('mrmath ' + ' '.join(tissue_images[3:5]) + ' sum tissuesum_34.mif')
  progress.increment()
  run.command('mrcalc tissue1_init.mif tissue1_init.mif tissuesum_34.mif -add 1.0 -sub 0.0 -max -sub ' + tissue_images[1])
  file.delTemporary('tissue1_init.mif')
  file.delTemporary('tissuesum_34.mif')
  progress.increment()
  run.command('mrmath ' + tissue_images[1] + ' ' + ' '.join(tissue_images[3:5]) + ' sum tissuesum_134.mif')
  progress.increment()
  run.command('mrcalc tissue2_init.mif tissue2_init.mif tissuesum_134.mif -add 1.0 -sub 0.0 -max -sub ' + tissue_images[2])
  file.delTemporary('tissue2_init.mif')
  file.delTemporary('tissuesum_134.mif')
  progress.increment()
  run.command('mrmath ' + ' '.join(tissue_images[1:5]) + ' sum tissuesum_1234.mif')
  progress.increment()
  run.command('mrcalc tissue0_init.mif tissue0_init.mif tissuesum_1234.mif -add 1.0 -sub 0.0 -max -sub ' + tissue_images[0])
  file.delTemporary('tissue0_init.mif')
  file.delTemporary('tissuesum_1234.mif')
  progress.increment()
  tissue_sum_image = 'tissuesum_01234.mif'
  run.command('mrmath ' + ' '.join(tissue_images) + ' sum ' + tissue_sum_image)
  progress.done()




  # Branch depending on whether or not FSL fast will be used to re-segment the cerebellum
  if have_fast:

    # How to support -template option?
    # - Re-grid norm.mgz to template image before running FAST
    # - Re-grid FAST output to template image
    # Consider splitting, including initial mapping of cerebellar regions:
    # - If we're not using a separate template image, just map cerebellar regions to voxels to
    #   produce a mask, and run FAST within that mask
    # - If we have a template, combine cerebellar regions, convert to surfaces (one per hemisphere),
    #   map these to the template image, run FIRST on a binary mask from this, then
    #   re-combine this with the tissue maps from other sources based on the estimated PVF of
    #   cerebellum meshes
    cerebellum_volume_image = 'Cerebellum_volume.mif'
    if app.args.template:

      # If this is the case, then we haven't yet performed any cerebellar segmentation / meshing
      # What we want to do is: for each hemisphere, combine all three "cerebellar" segments from FreeSurfer,
      #   convert to a surface, map that surface to the template image
      progress = app.progressBar('Preparing images of cerebellum for intensity-based segmentation', 11)
      cerebellar_hemisphere_pvf_images = [ ]
      for hemi in [ 'Left', 'Right' ]:
        cerebellar_images = [ n + '.mif' for (i, t, n) in structures if hemi in n and 'Cerebellum' in n ]
        sum_image = hemi + '-Cerebellum-All.mif'
        init_mesh_path = hemi + '-Cerebellum-All-Init.vtk'
        smooth_mesh_path = hemi + '-Cerebellum-All-Smooth.vtk'
        pvf_image_path = hemi + '-Cerebellum-PVF-Template.mif'
        run.command('mrmath ' + ' '.join(cerebellar_images) + ' sum ' + sum_image)
        # TODO Update with new file.delTemporary()
        for entry in cerebellar_images:
          file.delTemporary(entry)
        progress.increment()
        run.command('mrmesh ' + sum_image + ' ' + init_mesh_path)
        file.delTemporary(sum_image)
        progress.increment()
        run.command('meshfilter ' + init_mesh_path + ' smooth ' + smooth_mesh_path)
        file.delTemporary(init_mesh_path)
        progress.increment()
        run.command('mesh2voxel ' + smooth_mesh_path + ' ' + template_image + ' ' + pvf_image_path)
        file.delTemporary(smooth_mesh_path)
        cerebellar_hemisphere_pvf_images.append(pvf_image_path)
        progress.increment()

      # Combine the two hemispheres together into:
      # - An image in preparation for running FAST
      # - A combined total partial volume fraction image that will be later used for tissue recombination
      run.command('mrcalc ' + ' '.join(cerebellar_hemisphere_pvf_images) + ' -add 1.0 -min ' + cerebellum_volume_image)
      # TODO Update with new file.delTemporary()
      for entry in cerebellar_hemisphere_pvf_images:
        file.delTemporary(entry)
      progress.increment()
      T1_cerebellum_mask_image = 'T1_cerebellum_mask.mif'
      run.command('mrthreshold ' + cerebellum_volume_image + ' ' + T1_cerebellum_mask_image + ' -abs 1e-6')
      progress.increment()
      run.command('mrtransform ' + norm_image + ' -template ' + template_image + ' - | ' \
                  'mrcalc - ' + T1_cerebellum_mask_image + ' -mult - | ' \
                  'mrconvert - T1_cerebellum_precrop.mif')
      progress.done()

    else:

      progress = app.progressBar('Preparing images of cerebellum for intensity-based segmentation', 2)
      # Generate a mask of all voxels classified as cerebellum by FreeSurfer
      cerebellum_mask_images = [ n + '.mif' for (i, t, n) in structures if 'Cerebellum' in n ]
      run.command('mrmath ' + ' '.join(cerebellum_mask_images) + ' sum ' + cerebellum_volume_image)
      for entry in cerebellum_mask_images:
        file.delTemporary(entry)
      progress.increment()
      # FAST image input needs to be pre-masked
      T1_cerebellum_mask_image = cerebellum_volume_image
      run.command('mrcalc T1.nii ' + T1_cerebellum_mask_image + ' -mult - | mrconvert - T1_cerebellum_precrop.mif')
      progress.done()

    file.delTemporary('T1.nii')

    # Any code below here should be compatible with cerebellum_volume_image.mif containing partial volume fractions
    #   (in the case of no explicit template image, it's a mask, but the logic still applies)

    app.console('Running FSL fast to segment the cerebellum based on intensity information')

    # Run FSL FAST just within the cerebellum
    # TESTME Should bias field estimation be disabled within fast?
    # FAST memory usage can also be huge when using a high-resolution template image:
    #   Crop T1 image around the cerebellum before feeding to FAST, then re-sample to full template image FoV
    fast_input_image = 'T1_cerebellum.nii'
    run.command('mrcrop T1_cerebellum_precrop.mif -mask ' + T1_cerebellum_mask_image + ' ' + fast_input_image)
    # TODO Store this name in a variable
    file.delTemporary('T1_cerebellum_precrop.mif')
    # FIXME Cleanup of T1_cerebellum_mask_image: May be same image as cerebellum_volume_image
    run.command(fast_cmd + ' -N ' + fast_input_image)
    file.delTemporary(fast_input_image)

    # Use glob to clean up unwanted FAST outputs
    fast_output_prefix = os.path.splitext(fast_input_image)[0]
    fast_pve_output_prefix = fast_output_prefix + '_pve_'
    file.delTemporary([ entry for entry in glob.glob(fast_output_prefix + '*') if not fast_pve_output_prefix in entry ])

    progress = app.progressBar('Introducing intensity-based cerebellar segmentation into the 5TT image', 10)
    fast_outputs_cropped = [ fast_pve_output_prefix + str(n) + fast_suffix for n in range(0,3) ]
    fast_outputs_template = [ 'FAST_' + str(n) + '.mif' for n in range(0,3) ]
    for inpath, outpath in zip(fast_outputs_cropped, fast_outputs_template):
      run.command('mrtransform ' + inpath + ' -interp nearest -template ' + template_image + ' ' + outpath)
      file.delTemporary(inpath)
      progress.increment()
    if app.args.template:
      file.delTemporary(template_image)

    # Generate the revised tissue images, using output from FAST inside the cerebellum and
    #   output from previous processing everywhere else
    # Note that the middle intensity (grey matter) in the FAST output here gets assigned
    #   to the sub-cortical grey matter component

    # Some of these voxels may have existing non-zero tissue components.
    #   In that case, let's find a multiplier to apply to cerebellum tissues such that the sum does not exceed 1.0
    new_tissue_images = [ 'tissue0_fast.mif', 'tissue1_fast.mif', 'tissue2_fast.mif', 'tissue3_fast.mif', 'tissue4_fast.mif' ]
    new_tissue_sum_image = 'tissuesum_01234_fast.mif'
    cerebellum_multiplier_image = 'Cerebellar_multiplier.mif'
    run.command('mrcalc ' + cerebellum_volume_image + ' 0.0 -gt 1.0 ' + tissue_sum_image + ' -sub ' + cerebellum_volume_image + ' -div ' + cerebellum_volume_image + ' -min 0.0 -if  ' + cerebellum_multiplier_image)
    file.delTemporary(cerebellum_volume_image)
    progress.increment()
    run.command('mrconvert ' + tissue_images[0] + ' ' + new_tissue_images[0])
    file.delTemporary(tissue_images[0])
    progress.increment()
    run.command('mrcalc ' + tissue_images[1] + ' ' + cerebellum_multiplier_image + ' ' + fast_outputs_template[1] + ' -mult -add ' + new_tissue_images[1])
    file.delTemporary(tissue_images[1])
    file.delTemporary(fast_outputs_template[1])
    progress.increment()
    run.command('mrcalc ' + tissue_images[2] + ' ' + cerebellum_multiplier_image + ' ' + fast_outputs_template[2] + ' -mult -add ' + new_tissue_images[2])
    file.delTemporary(tissue_images[2])
    file.delTemporary(fast_outputs_template[2])
    progress.increment()
    run.command('mrcalc ' + tissue_images[3] + ' ' + cerebellum_multiplier_image + ' ' + fast_outputs_template[0] + ' -mult -add ' + new_tissue_images[3])
    file.delTemporary(tissue_images[3])
    file.delTemporary(fast_outputs_template[0])
    file.delTemporary(cerebellum_multiplier_image)
    progress.increment()
    run.command('mrconvert ' + tissue_images[4] + ' ' + new_tissue_images[4])
    file.delTemporary(tissue_images[4])
    progress.increment()
    run.command('mrmath ' + ' '.join(new_tissue_images) + ' sum ' + new_tissue_sum_image)
    file.delTemporary(tissue_sum_image)
    progress.done()
    tissue_images = new_tissue_images
    tissue_sum_image = new_tissue_sum_image



  # For all voxels within FreeSurfer's brain mask, add to the CSF image in order to make the sum 1.0
  # TESTME Rather than doing this blindly, look for holes in the brain, and assign the remainder to WM;
  #   only within the mask but outside the brain should the CSF fraction be filled
  # TODO Can definitely do better than just an erosion step here; still some hyper-intensities at GM-WW interface

  progress = app.progressBar('Performing fill operations to preserve unity tissue volume', 2)
  # TODO Connected-component analysis at high template image resolution is taking up huge amounts of memory
  # Crop beforehand? It's because it's filling everything outside the brain...
  # TODO Consider performing CSF fill before WM fill? Does this solve any problems?
  #   Well, it might mean I can use connected-components to select the CSF fill, rather than using an erosion
  #   This may also reduce the RAM usage of the connected-components analysis, since it'd be within
  #   the brain mask rather than using the whole image FoV.
  # Also: Potentially use mask cleaning filter
  # FIXME Appears we can't rely on a connected-component filter: Still some bits e.g. between cortex & cerebellum
  # Maybe need to look into techniques for specifically filling in between structure pairs

  # TESTME This appears to be correct in the template case, but wrong in the default case
  # OK, what's happening is: Some voxels are getting a non-zero cortical GM fraction due to native use
  #   of the surface representation, but these voxels are actually outside FreeSurfer's own provided brain
  #   mask. So what we need to do here is get the union of the tissue sum nonzero image and the mask image,
  #   and use that at the -mult step of the mrcalc call.

  # Required image: (tissue_sum_image > 0.0) || mask_image
  # tissue_sum_image 0.0 -gt mask_image -add 1.0 -min


  new_tissue_images = [ tissue_images[0],
                        tissue_images[1],
                        tissue_images[2],
                        os.path.splitext(tissue_images[3])[0] + '_filled.mif',
                        tissue_images[4] ]
  csf_fill_image = 'csf_fill.mif'
  run.command('mrcalc 1.0 ' + tissue_sum_image + ' -sub ' + tissue_sum_image + ' 0.0 -gt ' + mask_image + ' -add 1.0 -min -mult ' + csf_fill_image)
  file.delTemporary(tissue_sum_image)
  # If no template is specified, this file is part of the FreeSurfer output; hence don't modify
  if app.args.template:
    file.delTemporary(mask_image)
  progress.increment()
  run.command('mrcalc ' + tissue_images[3] + ' ' + csf_fill_image + ' -add ' + new_tissue_images[3])
  file.delTemporary(csf_fill_image)
  file.delTemporary(tissue_images[3])
  progress.done()
  tissue_images = new_tissue_images



  # TODO Make attempt at setting non-brain voxels at bottom of brain stem
  # Note that this needs to be compatible with -template option
  # - Generate gradient image of norm.mgz - should be bright at CSF edges, less so at arbitrary cuts through the WM
  #   Note: Hopefully keeping directionality information
  # - Generate gradient image of brain stem partial volume image
  #   Note: If using -template option, this will likely need to be re-generated in native space
  #   (Actually, may have been erased by file.delTemporary() earlier...)
  # - Get (absolute) inner product of two gradient images - should be bright where brain stem segmentation and
  #   gradients in T1 intensity colocalise, dark where either is absent
  #   Actually this needs to be more carefully considered: Ideally want something that appears bright at the
  #   bottom edge of the brain stem. E.g. min(abs(Gradient of brain stem PVF +- gradient of T1 image))
  #   Will need some kind of appropriate scaling of T1 gradient image in order to enable an addition / subtraction
  #   Does norm.mgz get a standardised intensity range?
  # - Perform automatic threshold & connected-component analysis
  #   Perhaps prior to this point, could set the image FoV based on the brain stem segmentation, then
  #   retain only the lower half of the FoV, such that the largest connected component is the bottom part
  # - Maybe dilate this a little bit
  # - Multiply all tissues by 0 in these voxels (may require conversion back to template image)



  # Finally, concatenate the volumes to produce the 5TT image
  precrop_result_image = '5TT.mif'
  run.command('mrcat ' + ' '.join(tissue_images) + ' ' + precrop_result_image + ' -axis 3')
  # TODO Use new file.delTemporary()
  for entry in tissue_images:
    file.delTemporary(entry)


  # Maybe don't go off all tissues here, since FreeSurfer's mask can be fairly liberal;
  #   instead get just a voxel clearance from all other tissue types (maybe two)
  if app.args.nocrop:
    run.function(os.rename, precrop_result_image, 'result.mif')
  else:
    app.console('Cropping final 5TT image')
    crop_mask_image = 'crop_mask.mif'
    run.command('mrconvert ' + precrop_result_image + ' -coord 3 0,1,2,4 - | mrmath - sum - -axis 3 | mrthreshold - - -abs 0.001 | maskfilter - dilate ' + crop_mask_image)
    run.command('mrcrop ' + precrop_result_image + ' result.mif -mask ' + crop_mask_image)
    file.delTemporary(crop_mask_image)
    file.delTemporary(precrop_result_image)





  app.warn('Script algorithm is not yet capable of performing requisite image modifications in order to '
           'permit streamlines travelling from the brain stem down the spinal column. Recommend using '
           '5ttedit -none, in conjunction with a manually-drawn ROI labelling the bottom part of the '
           'brain stem, such that streamlines in this region are characterised by ACT as exiting the image.')
