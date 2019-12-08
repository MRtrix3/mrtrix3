# Copyright (c) 2008-2019 the MRtrix3 contributors.
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



import glob, os, re
from mrtrix3 import MRtrixError
from mrtrix3 import app, fsl, path, run



# Potential future improvements:
# - Automatic segmentation of anterior commissure (potentially even posterior commissure...)
# - Accept T1-weighted image as input; run all FreeSurfer steps
# - Operate on HCP data
# - Rather than hard-coding structure indices, read file FreeSurferColorLUT.txt from the FreeSurfer installation, find structures by name, then extract the integer index
# - Option to utilise output of FreeSurfer thalamic nuclei segmentation module; provide explicit command-line control of segmentation mechanism similar to how hippocampi are handled
# - Improve logic w.r.t. tissue combination, in order to reduce prevalence of "gaps" between structures caused by independent voxel2mesh | meshfilter smooth operations on adjacent structures




HIPPOCAMPI_CHOICES = [ 'subfields', 'first', 'aseg' ]



def usage(base_parser, subparsers): #pylint: disable=unused-variable
  parser = subparsers.add_parser('hsvs', parents=[base_parser])
  parser.set_author('Robert E. Smith (robert.smith@florey.edu.au)')
  parser.set_synopsis('Generate a 5TT image based on Hybrid Surface and Volume Segmentation (HSVS), using FreeSurfer and FSL tools')
  parser.add_argument('input',  help='The input FreeSurfer subject directory')
  parser.add_argument('output', help='The output 5TT image')
  parser.add_argument('-template', help='Provide an image that will form the template for the generated 5TT image')
  parser.add_argument('-hippocampi', choices=HIPPOCAMPI_CHOICES, help='Select method to be used for hippocampi (& amygdalae) segmentation; options are: ' + ','.join(HIPPOCAMPI_CHOICES))
  parser.add_argument('-white_stem', action='store_true', help='Classify the brainstem as white matter')



ASEG_STRUCTURES = [ ( 4,  3, 'Left-Lateral-Ventricle'),
                    ( 5,  3, 'Left-Inf-Lat-Vent'),
                    ( 9,  1, 'Left-Thalamus'),
                    (10,  1, 'Left-Thalamus-Proper'),
                    (11,  1, 'Left-Caudate'),
                    (12,  1, 'Left-Putamen'),
                    (13,  1, 'Left-Pallidum'),
                    (14,  3, '3rd-Ventricle'),
                    (15,  3, '4th-Ventricle'),
                    (24,  3, 'CSF'),
                    (25,  4, 'Left-Lesion'),
                    (26,  1, 'Left-Accumbens-area'),
                    (30,  3, 'Left-vessel'),
                    (31,  3, 'Left-choroid-plexus'),
                    (43,  3, 'Right-Lateral-Ventricle'),
                    (44,  3, 'Right-Inf-Lat-Vent'),
                    (48,  1, 'Right-Thalamus'),
                    (49,  1, 'Right-Thalamus-Proper'),
                    (50,  1, 'Right-Caudate'),
                    (51,  1, 'Right-Putamen'),
                    (52,  1, 'Right-Pallidum'),
                    (57,  4, 'Right-Lesion'),
                    (58,  1, 'Right-Accumbens-area'),
                    (62,  3, 'Right-vessel'),
                    (63,  3, 'Right-choroid-plexus'),
                    (72,  3, '5th-Ventricle'),
                    (77,  4, 'WM-hypointensities'),
                    (78,  4, 'Left-WM-hypointensities'),
                    (79,  4, 'Right-WM-hypointensities'),
                    (250, 2, 'Fornix') ]


HIPP_ASEG = [ (17,  1, 'Left-Hippocampus'),
              (53,  1, 'Right-Hippocampus') ]


AMYG_ASEG = [ (18,  1, 'Left-Amygdala'),
              (54,  1, 'Right-Amygdala') ]


CEREBELLUM_ASEG = [ ( 6,  3, 'Left-Cerebellum-Exterior'),
                    ( 7,  2, 'Left-Cerebellum-White-Matter'),
                    ( 8,  1, 'Left-Cerebellum-Cortex'),
                    (45,  3, 'Right-Cerebellum-Exterior'),
                    (46,  2, 'Right-Cerebellum-White-Matter'),
                    (47,  1, 'Right-Cerebellum-Cortex') ]


CORPUS_CALLOSUM_ASEG = [ (251, 'CC_Posterior'),
                         (252, 'CC_Mid_Posterior'),
                         (253, 'CC_Central'),
                         (254, 'CC_Mid_Anterior'),
                         (255, 'CC_Anterior') ]


BRAIN_STEM_ASEG = [ (16, 'Brain-Stem'),
                    (27, 'Left-Substancia-Nigra'),
                    (28, 'Left-VentralDC'),
                    (59, 'Right-Substancia-Nigra'),
                    (60, 'Right-VentralDC') ]


SGM_FIRST_MAP = { 'L_Accu':'Left-Accumbens-area',  'R_Accu':'Right-Accumbens-area',
                  'L_Amyg':'Left-Amygdala',        'R_Amyg':'Right-Amygdala',
                  'L_Caud':'Left-Caudate',         'R_Caud':'Right-Caudate',
                  'L_Hipp':'Left-Hippocampus',     'R_Hipp':'Right-Hippocampus',
                  'L_Pall':'Left-Pallidum',        'R_Pall':'Right-Pallidum',
                  'L_Puta':'Left-Putamen',         'R_Puta':'Right-Putamen',
                  'L_Thal':'Left-Thalamus-Proper', 'R_Thal':'Right-Thalamus-Proper' }




def check_file(filepath):
  if not os.path.isfile(filepath):
    raise MRtrixError('Required input file missing (expected location: ' + filepath + ')')

def check_dir(dirpath):
  if not os.path.isdir(dirpath):
    raise MRtrixError('Unable to find sub-directory \'' + dirpath + '\' within input directory')




def check_output_paths(): #pylint: disable=unused-variable
  pass



def get_inputs(): #pylint: disable=unused-variable
  # Most freeSurfer files will be accessed in-place; no need to pre-convert them into the temporary directory
  # However convert aparc image so that it does not have to be repeatedly uncompressed
  run.command('mrconvert ' + path.from_user(os.path.join(app.ARGS.input, 'mri', 'aparc+aseg.mgz'), True) + ' ' + path.to_scratch('aparc.mif', True))
  if app.ARGS.template:
    run.command('mrconvert ' + path.from_user(app.ARGS.template, True) + ' ' + path.to_scratch('template.mif', True) + ' -axes 0,1,2')



def execute(): #pylint: disable=unused-variable

  subject_dir = os.path.abspath(path.from_user(app.ARGS.input, False))
  if not os.path.isdir(subject_dir):
    raise MRtrixError('Input to hsvs algorithm must be a directory')
  surf_dir = os.path.join(subject_dir, 'surf')
  mri_dir = os.path.join(subject_dir, 'mri')
  check_dir(surf_dir)
  check_dir(mri_dir)
  #aparc_image = os.path.join(mri_dir, 'aparc+aseg.mgz')
  aparc_image = 'aparc.mif'
  mask_image = os.path.join(mri_dir, 'brainmask.mgz')
  reg_file = os.path.join(mri_dir, 'transforms', 'talairach.xfm')
  check_file(aparc_image)
  check_file(mask_image)
  check_file(reg_file)
  template_image = 'template.mif' if app.ARGS.template else aparc_image

  have_first = False
  have_fast = False
  fsl_path = os.environ.get('FSLDIR', '')
  if fsl_path:
    # Use brain-extracted, bias-corrected image for FSL tools
    norm_image = os.path.join(mri_dir, 'norm.mgz')
    check_file(norm_image)
    run.command('mrconvert ' + norm_image + ' T1.nii -stride -1,+2,+3')
    # Verify FAST availability
    try:
      fast_cmd = fsl.exe_name('fast')
    except MRtrixError:
      fast_cmd = None
    if fast_cmd:
      have_fast = True
      if fast_cmd == 'fast':
        fast_suffix = fsl.suffix()
      else:
        fast_suffix = '.nii.gz'
    else:
      app.warn('Could not find FSL program fast; script will not use fast for cerebellar tissue segmentation')
    # Verify FIRST availability
    try:
      first_cmd = fsl.exe_name('run_first_all')
    except MRtrixError:
      first_cmd = None
    first_atlas_path = os.path.join(fsl_path, 'data', 'first', 'models_336_bin')
    have_first = first_cmd and os.path.isdir(first_atlas_path)
  else:
    app.warn('Environment variable FSLDIR is not set; script will run without FSL components')

  # Need to perform a better search for hippocampal subfield output: names & version numbers may change
  have_hipp_subfields = False
  hipp_subfield_has_amyg = False
  # Could result in multiple matches
  hipp_subfield_regex = re.compile(r'[lr]h\.hippo[a-zA-Z]*Labels-[a-zA-Z0-9]*\.v[0-9]+\.?[a-zA-Z0-9]*\.mg[hz]')
  hipp_subfield_all_images = sorted(list(filter(hipp_subfield_regex.match, os.listdir(mri_dir))))
  # Remove any images that provide segmentations in FreeSurfer voxel space; we want the high-resolution versions
  hipp_subfield_all_images = [ item for item in hipp_subfield_all_images if 'FSvoxelSpace' not in item ]
  # Arrange the images into lr pairs
  hipp_subfield_paired_images = [ ]
  for lh_filename in [ item for item in hipp_subfield_all_images if item[0] == 'l' ]:
    if 'r' + lh_filename[1:] in hipp_subfield_all_images:
      hipp_subfield_paired_images.append(lh_filename[1:])
  # Choose which of these image pairs we are going to use
  for code in [ '.CA.', '.FS60.' ]:
    if any([ code in filename for filename in hipp_subfield_paired_images ]):
      hipp_subfield_image_suffix = [ filename for filename in hipp_subfield_paired_images if code in filename ][0]
      have_hipp_subfields = True
      break
  # Choose the pair with the shortest filename string if we have no other criteria
  if not have_hipp_subfields and hipp_subfield_paired_images:
    hipp_subfield_paired_images = sorted(hipp_subfield_paired_images, key=len)
    if hipp_subfield_paired_images:
      hipp_subfield_image_suffix = hipp_subfield_paired_images[0]
      have_hipp_subfields = True
  if have_hipp_subfields:
    hipp_subfield_has_amyg = 'Amyg' in hipp_subfield_image_suffix


  # If particular hippocampal segmentation method is requested, make sure we can perform such;
  #   if not, decide how to segment hippocampus based on what's available
  hippocampi_method = app.ARGS.hippocampi
  if hippocampi_method:
    if hippocampi_method == 'subfields':
      if not have_hipp_subfields:
        raise MRtrixError('Could not isolate hippocampal subfields module output (candidate images: ' + str(hipp_subfield_all_images) + ')')
    elif hippocampi_method == 'first':
      if not have_first:
        raise MRtrixError('Cannot use "first" method for hippocampi segmentation; check FSL installation')
  else:
    if have_hipp_subfields:
      hippocampi_method = 'subfields'
      app.console('Hippocampal subfields module output detected; will utilise for hippocampi ' + \
                  ('and amygdalae ' if hipp_subfield_has_amyg else '') + \
                  'segmentation')
    elif have_first:
      hippocampi_method = 'first'
      app.console('No hippocampal subfields module output detected, but FSL FIRST is installed; ' + \
                  'will utilise latter for hippocampi segmentation')
    else:
      hippocampi_method = 'aseg'
      app.console('Neither hippocampal subfields module output nor FSL FIRST detected; ' + \
                  'FreeSurfer aseg will be used for hippocampi segmentation')

  if hippocampi_method == 'subfields':
    if 'FREESURFER_HOME' not in os.environ:
      raise MRtrixError('FREESURFER_HOME environment variable not set; required for use of hippocampal subfields module')
    freesurfer_lut_file = os.path.join(os.environ['FREESURFER_HOME'], 'FreeSurferColorLUT.txt')
    check_file(freesurfer_lut_file)
    hipp_lut_file = os.path.join(path.shared_data_path(), path.script_subdir_name(), 'hsvs', 'HippSubfields.txt')
    check_file(hipp_lut_file)
    if hipp_subfield_has_amyg:
      amyg_lut_file = os.path.join(path.shared_data_path(), path.script_subdir_name(), 'hsvs', 'AmygSubfields.txt')
      check_file(amyg_lut_file)

  if app.ARGS.sgm_amyg_hipp:
    app.warn('Option -sgm_amyg_hipp ignored '
             + '(hsvs algorithm always assigns hippocampi & ampygdalae as sub-cortical grey matter)')




  ###########################
  # Commencing segmentation #
  ###########################

  tissue_images = [ [ 'lh.pial.mif', 'rh.pial.mif' ],
                    [],
                    [ 'lh.white.mif', 'rh.white.mif' ],
                    [],
                    [] ]

  # Get the main cerebrum segments; these are already smooth
  # FIXME There may be some minor mismatch between the WM and pial segments within the medial section
  #   where no optimisation is performed, but vertices are simply defined in order to guarantee
  #   closed surfaces. Ideally these should be removed from the CGM tissue.
  progress = app.ProgressBar('Mapping FreeSurfer cortical reconstruction to partial volume images', 8)
  for hemi in [ 'lh', 'rh' ]:
    for basename in [ hemi+'.white', hemi+'.pial' ]:
      filepath = os.path.join(surf_dir, basename)
      check_file(filepath)
      transformed_path = basename + '_realspace.obj'
      run.command('meshconvert ' + filepath + ' ' + transformed_path + ' -binary -transform fs2real ' + aparc_image)
      progress.increment()
      run.command('mesh2voxel ' + transformed_path + ' ' + template_image + ' ' + basename + '.mif')
      app.cleanup(transformed_path)
      progress.increment()
  progress.done()



  # Get other structures that need to be converted from the aseg voxel image
  from_aseg = ASEG_STRUCTURES.copy()
  if hippocampi_method == 'subfields':
    if not hipp_subfield_has_amyg:
      from_aseg.extend(AMYG_ASEG)
    if have_first:
      from_aseg = [ entry for entry in from_aseg if entry[2] not in SGM_FIRST_MAP.values() ]
  elif hippocampi_method == 'first':
    # Wouldn't still be executing if first were not installed
    from_aseg = [ entry for entry in from_aseg if entry[2] not in SGM_FIRST_MAP.values() ]
  elif hippocampi_method == 'aseg':
    from_aseg.extend(AMYG_ASEG)
    if have_first:
      from_aseg = [ entry for entry in from_aseg if entry[2] not in SGM_FIRST_MAP.values() ]
    from_aseg.extend(HIPP_ASEG)
  progress = app.ProgressBar('Smoothing non-cortical structures segmented by FreeSurfer', len(from_aseg))
  for (index, tissue, name) in from_aseg:
    init_mesh_path = name + '_init.vtk'
    smoothed_mesh_path = name + '.vtk'
    run.command('mrcalc ' + aparc_image + ' ' + str(index) + ' -eq - | voxel2mesh - -threshold 0.5 ' + init_mesh_path)
    run.command('meshfilter ' + init_mesh_path + ' smooth ' + smoothed_mesh_path)
    app.cleanup(init_mesh_path)
    run.command('mesh2voxel ' + smoothed_mesh_path + ' ' + template_image + ' ' + name + '.mif')
    app.cleanup(smoothed_mesh_path)
    tissue_images[tissue].append(name + '.mif')
    progress.increment()
  progress.done()



  # Combine corpus callosum segments before smoothing
  progress = app.ProgressBar('Combining and smoothing corpus callosum segmentation', len(CORPUS_CALLOSUM_ASEG) + 3)
  for (index, name) in CORPUS_CALLOSUM_ASEG:
    run.command('mrcalc ' + aparc_image + ' ' + str(index) + ' -eq ' + name + '.mif -datatype bit')
    progress.increment()
  cc_init_mesh_path = 'combined_corpus_callosum_init.vtk'
  cc_smoothed_mesh_path = 'combined_corpus_callosum.vtk'
  run.command('mrmath ' + ' '.join([ name + '.mif' for (index, name) in CORPUS_CALLOSUM_ASEG ]) + ' sum - | voxel2mesh - -threshold 0.5 ' + cc_init_mesh_path)
  for name in [ n for i, n in CORPUS_CALLOSUM_ASEG ]:
    app.cleanup(name + '.mif')
  progress.increment()
  run.command('meshfilter ' + cc_init_mesh_path + ' smooth ' + cc_smoothed_mesh_path)
  app.cleanup(cc_init_mesh_path)
  progress.increment()
  run.command('mesh2voxel ' + cc_smoothed_mesh_path + ' ' + template_image + ' combined_corpus_callosum.mif')
  app.cleanup(cc_smoothed_mesh_path)
  progress.done()
  tissue_images[2].append('combined_corpus_callosum.mif')



  # Deal with brain stem, including determining those voxels that should
  #   be erased from the 5TT image in order for streamlines traversing down
  #   the spinal column to be terminated & accepted
  bs_fullmask_path = 'Brain_stem_init.mif'
  bs_cropmask_path = ''
  progress = app.ProgressBar('Segmenting and cropping brain stem', 6 + (2 if app.ARGS.template else 0))

  run.command('mrcalc ' + aparc_image + ' ' + str(BRAIN_STEM_ASEG[0][0]) + ' -eq '
              + ' -add '.join([ aparc_image + ' ' + str(index) + ' -eq' for index, name in BRAIN_STEM_ASEG[1:] ]) + ' -add '
              + bs_fullmask_path + ' -datatype bit')
  progress.increment()
  fourthventricle_zmin = min([ int(line.split()[2]) for line in run.command('maskdump 4th-Ventricle.mif')[0].splitlines() ])
  bs_voxel2mesh_input = bs_fullmask_path
  if fourthventricle_zmin:
    bs_wmmask_path = 'Brain_stem_preserve.mif'
    bs_cropmask_path = 'Brain_stem_crop.mif'
    run.command('mredit ' + bs_fullmask_path + ' ' + bs_wmmask_path + ' ' + ' '.join([ '-plane 2 ' + str(index) + ' 0' for index in range(0, fourthventricle_zmin) ]))
    progress.increment()
    run.command('mrcalc ' + bs_fullmask_path + ' ' + bs_wmmask_path + ' -sub -1 -mult ' + bs_cropmask_path + ' -datatype bit')
    progress.increment()
    if app.ARGS.template:
      bs_cropmesh_path = 'Brain_stem_crop.vtk'
      bs_new_cropmask_path = 'Brain_stem_crop_template.mif'
      run.command('voxel2mesh ' + bs_cropmask_path + ' ' + bs_cropmesh_path + ' -blocky')
      progress.increment()
      run.command('mesh2voxel ' + bs_cropmesh_path + ' ' + template_image + ' - | ' + \
                  'mrthreshold - -abs 1e-6 ' + bs_new_cropmask_path)
      progress.increment()
      app.cleanup(bs_cropmesh_path)
      app.cleanup(bs_cropmask_path)
      bs_cropmask_path = bs_new_cropmask_path
    bs_voxel2mesh_input = bs_wmmask_path

  app.cleanup(bs_fullmask_path)
  bs_init_mesh_path = 'brain_stem_init.vtk'
  bs_smoothed_mesh_path = 'brain_stem.vtk'
  run.command('voxel2mesh ' + bs_voxel2mesh_input + ' ' + bs_init_mesh_path)
  app.cleanup(bs_voxel2mesh_input)
  progress.increment()
  run.command('meshfilter ' + bs_init_mesh_path + ' smooth ' + bs_smoothed_mesh_path)
  app.cleanup(bs_init_mesh_path)
  progress.increment()
  run.command('mesh2voxel ' + bs_smoothed_mesh_path + ' ' + template_image + ' brain_stem.mif')
  app.cleanup(bs_smoothed_mesh_path)
  progress.done()
  tissue_images[2 if app.ARGS.white_stem else 4].append('brain_stem.mif')


  if hippocampi_method == 'subfields':
    progress = app.ProgressBar('Using detected FreeSurfer hippocampal subfields module output', 64 if hipp_subfield_has_amyg else 32)

    subfields = [ ( hipp_lut_file, 'hipp' ) ]
    if hipp_subfield_has_amyg:
      subfields.append(( amyg_lut_file, 'amyg' ))

    for subfields_lut_file, structure_name in subfields:
      for hemi, filename in zip([ 'Left', 'Right'], [ prefix + hipp_subfield_image_suffix for prefix in [ 'l', 'r' ] ]):
        # Extract individual components from image and assign to different tissues
        subfields_all_tissues_image = hemi + '_' + structure_name + '_subfields.mif'
        run.command('labelconvert ' + os.path.join(mri_dir, filename) + ' ' + freesurfer_lut_file + ' ' + subfields_lut_file + ' ' + subfields_all_tissues_image)
        progress.increment()
        for tissue in range(0, 5):
          init_mesh_path = hemi + '_' + structure_name + '_subfield_' + str(tissue) + '_init.vtk'
          smooth_mesh_path = hemi + '_' + structure_name + '_subfield_' + str(tissue) + '.vtk'
          subfield_tissue_image = hemi + '_' + structure_name + '_subfield_' + str(tissue) + '.mif'
          run.command('mrcalc ' + subfields_all_tissues_image + ' ' + str(tissue+1) + ' -eq - | ' + \
                      'voxel2mesh - ' + init_mesh_path)
          progress.increment()
          # Since the hippocampal subfields segmentation can include some fine structures, reduce the extent of smoothing
          run.command('meshfilter ' + init_mesh_path + ' smooth ' + smooth_mesh_path + ' -smooth_spatial 2 -smooth_influence 2')
          app.cleanup(init_mesh_path)
          progress.increment()
          run.command('mesh2voxel ' + smooth_mesh_path + ' ' + template_image + ' ' + subfield_tissue_image)
          app.cleanup(smooth_mesh_path)
          progress.increment()
          tissue_images[tissue].append(subfield_tissue_image)
        app.cleanup(subfields_all_tissues_image)
    progress.done()




  if have_first:
    app.console('Running FSL FIRST to segment sub-cortical grey matter structures')
    from_first = SGM_FIRST_MAP.copy()
    if hippocampi_method == 'subfields':
      from_first = { key: value for key, value in from_first.items() if 'Hippocampus' not in value }
      if hipp_subfield_has_amyg:
        from_first = { key: value for key, value in from_first.items() if 'Amygdala' not in value }
    elif hippocampi_method == 'aseg':
      from_first = { key: value for key, value in from_first.items() if 'Hippocampus' not in value }
    run.command(first_cmd + ' -s ' + ','.join(from_first.keys()) + ' -i T1.nii -b -o first')
    fsl.check_first('first', from_first.keys())
    app.cleanup(glob.glob('T1_to_std_sub.*'))
    progress = app.ProgressBar('Mapping FIRST segmentations to image', 2*len(from_first))
    for key, value in from_first.items():
      vtk_in_path = 'first-' + key + '_first.vtk'
      vtk_converted_path = 'first-' + key + '_transformed.vtk'
      run.command('meshconvert ' + vtk_in_path + ' ' + vtk_converted_path + ' -transform first2real T1.nii')
      app.cleanup(vtk_in_path)
      progress.increment()
      run.command('mesh2voxel ' + vtk_converted_path + ' ' + template_image + ' ' + value + '.mif')
      app.cleanup(vtk_converted_path)
      tissue_images[1].append(value + '.mif')
      progress.increment()
    if not have_fast:
      app.cleanup('T1.nii')
    app.cleanup(glob.glob('first*'))
    progress.done()



  # If we don't have FAST, do cerebellar segmentation in a comparable way to the cortical GM / WM:
  #   Generate one 'pial-like' surface containing the GM and WM of the cerebellum,
  #   and another with just the WM
  if not have_fast:
    progress = app.ProgressBar('Adding FreeSurfer cerebellar segmentations directly', 6)
    for hemi in [ 'Left-', 'Right-' ]:
      wm_index = [ index for index, tissue, name in CEREBELLUM_ASEG if name.startswith(hemi) and 'White' in name ][0]
      gm_index = [ index for index, tissue, name in CEREBELLUM_ASEG if name.startswith(hemi) and 'Cortex' in name ][0]
      run.command('mrcalc ' + aparc_image + ' ' + str(wm_index) + ' -eq ' + aparc_image + ' ' + str(gm_index) + ' -eq -add - | ' + \
                  'voxel2mesh - ' + hemi + 'cerebellum_all_init.vtk')
      progress.increment()
      run.command('mrcalc ' + aparc_image + ' ' + str(gm_index) + ' -eq - | ' + \
                  'voxel2mesh - ' + hemi + 'cerebellum_grey_init.vtk')
      progress.increment()
      for name, tissue in { 'all':2, 'grey':1 }.items():
        run.command('meshfilter ' + hemi + 'cerebellum_' + name + '_init.vtk smooth ' + hemi + 'cerebellum_' + name + '.vtk')
        app.cleanup(hemi + 'cerebellum_' + name + '_init.vtk')
        progress.increment()
        run.command('mesh2voxel ' + hemi + 'cerebellum_' + name + '.vtk ' + template_image + ' ' + hemi + 'cerebellum_' + name + '.mif')
        app.cleanup(hemi + 'cerebellum_' + name + '.vtk')
        progress.increment()
        tissue_images[tissue].append(hemi + 'cerebellum_' + name + '.mif')
    progress.done()


  # Construct images with the partial volume of each tissue
  progress = app.ProgressBar('Combining segmentations of all structures corresponding to each tissue type', 5)
  for tissue in range(0,5):
    run.command('mrmath ' + ' '.join(tissue_images[tissue]) + ' sum - | mrcalc - 1.0 -min tissue' + str(tissue) + '_init.mif')
    app.cleanup(tissue_images[tissue])
    progress.increment()
  progress.done()


  # This can hopefully be done with a connected-component analysis: Take just the WM image, and
  #   fill in any gaps (i.e. select the inverse, select the largest connected component, invert again)
  # Make sure that floating-point values are handled appropriately
  # TODO Idea: Dilate voxelised brain stem 2 steps, add only intersection with voxelised WM,
  #   then run through voxel2mesh

  # Combine these images together using the appropriate logic in order to form the 5TT image
  progress = app.ProgressBar('Modulating segmentation images based on other tissues', 9)
  tissue_images = [ 'tissue0.mif', 'tissue1.mif', 'tissue2.mif', 'tissue3.mif', 'tissue4.mif' ]
  run.function(os.rename, 'tissue4_init.mif', 'tissue4.mif')
  progress.increment()
  run.command('mrcalc tissue3_init.mif tissue3_init.mif ' + tissue_images[4] + ' -add 1.0 -sub 0.0 -max -sub ' + tissue_images[3])
  app.cleanup('tissue3_init.mif')
  progress.increment()
  run.command('mrmath ' + ' '.join(tissue_images[3:5]) + ' sum tissuesum_34.mif')
  progress.increment()
  run.command('mrcalc tissue1_init.mif tissue1_init.mif tissuesum_34.mif -add 1.0 -sub 0.0 -max -sub ' + tissue_images[1])
  app.cleanup('tissue1_init.mif')
  app.cleanup('tissuesum_34.mif')
  progress.increment()
  run.command('mrmath ' + tissue_images[1] + ' ' + ' '.join(tissue_images[3:5]) + ' sum tissuesum_134.mif')
  progress.increment()
  run.command('mrcalc tissue2_init.mif tissue2_init.mif tissuesum_134.mif -add 1.0 -sub 0.0 -max -sub ' + tissue_images[2])
  app.cleanup('tissue2_init.mif')
  app.cleanup('tissuesum_134.mif')
  progress.increment()
  run.command('mrmath ' + ' '.join(tissue_images[1:5]) + ' sum tissuesum_1234.mif')
  progress.increment()
  run.command('mrcalc tissue0_init.mif tissue0_init.mif tissuesum_1234.mif -add 1.0 -sub 0.0 -max -sub ' + tissue_images[0])
  app.cleanup('tissue0_init.mif')
  app.cleanup('tissuesum_1234.mif')
  progress.increment()
  tissue_sum_image = 'tissuesum_01234.mif'
  run.command('mrmath ' + ' '.join(tissue_images) + ' sum ' + tissue_sum_image)
  progress.done()


  if app.ARGS.template:
    run.command('mrtransform ' + mask_image + ' -template template.mif - | mrthreshold - brainmask.mif -abs 0.5')
    mask_image = 'brainmask.mif'


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
    cerebellum_mask_image = 'Cerebellum_mask.mif'
    t1_cerebellum_masked = 'T1_cerebellum_precrop.mif'
    if app.ARGS.template:

      # If this is the case, then we haven't yet performed any cerebellar segmentation / meshing
      # What we want to do is: for each hemisphere, combine all three "cerebellar" segments from FreeSurfer,
      #   convert to a surface, map that surface to the template image
      progress = app.ProgressBar('Preparing images of cerebellum for intensity-based segmentation', 9)
      cerebellar_hemi_pvf_images = [ ]
      for hemi in [ 'Left', 'Right' ]:
        init_mesh_path = hemi + '-Cerebellum-All-Init.vtk'
        smooth_mesh_path = hemi + '-Cerebellum-All-Smooth.vtk'
        pvf_image_path = hemi + '-Cerebellum-PVF-Template.mif'
        cerebellum_aseg_hemi = [ entry for entry in CEREBELLUM_ASEG if hemi in entry[2] ]
        run.command('mrcalc ' + aparc_image + ' ' + str(cerebellum_aseg_hemi[0][0]) + ' -eq ' + \
                    ' -add '.join([ aparc_image + ' ' + str(index) + ' -eq' for index, tissue, name in cerebellum_aseg_hemi[1:] ]) + ' -add - | ' + \
                    'voxel2mesh - ' + init_mesh_path)
        progress.increment()
        run.command('meshfilter ' + init_mesh_path + ' smooth ' + smooth_mesh_path)
        app.cleanup(init_mesh_path)
        progress.increment()
        run.command('mesh2voxel ' + smooth_mesh_path + ' ' + template_image + ' ' + pvf_image_path)
        app.cleanup(smooth_mesh_path)
        cerebellar_hemi_pvf_images.append(pvf_image_path)
        progress.increment()

      # Combine the two hemispheres together into:
      # - An image in preparation for running FAST
      # - A combined total partial volume fraction image that will be later used for tissue recombination
      run.command('mrcalc ' + ' '.join(cerebellar_hemi_pvf_images) + ' -add 1.0 -min ' + cerebellum_volume_image)
      app.cleanup(cerebellar_hemi_pvf_images)
      progress.increment()

      run.command('mrthreshold ' + cerebellum_volume_image + ' ' + cerebellum_mask_image + ' -abs 1e-6')
      progress.increment()
      run.command('mrtransform ' + norm_image + ' -template ' + template_image + ' - | ' + \
                  'mrcalc - ' + cerebellum_mask_image + ' -mult ' + t1_cerebellum_masked)
      progress.done()

    else:
      app.console('Preparing images of cerebellum for intensity-based segmentation')
      run.command('mrcalc ' + aparc_image + ' ' + str(CEREBELLUM_ASEG[0][0]) + ' -eq ' + \
                  ' -add '.join([ aparc_image + ' ' + str(index) + ' -eq' for index, tissue, name in CEREBELLUM_ASEG[1:] ]) + ' -add ' + \
                  cerebellum_volume_image)
      cerebellum_mask_image = cerebellum_volume_image
      run.command('mrcalc T1.nii ' + cerebellum_mask_image + ' -mult ' + t1_cerebellum_masked)

    app.cleanup('T1.nii')

    # Any code below here should be compatible with cerebellum_volume_image.mif containing partial volume fractions
    #   (in the case of no explicit template image, it's a mask, but the logic still applies)

    app.console('Running FSL fast to segment the cerebellum based on intensity information')

    # Run FSL FAST just within the cerebellum
    # FAST memory usage can also be huge when using a high-resolution template image:
    #   Crop T1 image around the cerebellum before feeding to FAST, then re-sample to full template image FoV
    fast_input_image = 'T1_cerebellum.nii'
    run.command('mrgrid ' + t1_cerebellum_masked + ' crop -mask ' + cerebellum_mask_image + ' ' + fast_input_image)
    app.cleanup(t1_cerebellum_masked)
    # Cleanup of cerebellum_mask_image:
    #   May be same image as cerebellum_volume_image, which is required later
    if cerebellum_mask_image != cerebellum_volume_image:
      app.cleanup(cerebellum_mask_image)
    run.command(fast_cmd + ' -N ' + fast_input_image)
    app.cleanup(fast_input_image)

    # Use glob to clean up unwanted FAST outputs
    fast_output_prefix = os.path.splitext(fast_input_image)[0]
    fast_pve_output_prefix = fast_output_prefix + '_pve_'
    app.cleanup([ entry for entry in glob.glob(fast_output_prefix + '*') if not fast_pve_output_prefix in entry ])

    progress = app.ProgressBar('Introducing intensity-based cerebellar segmentation into the 5TT image', 10)
    fast_outputs_cropped = [ fast_pve_output_prefix + str(n) + fast_suffix for n in range(0,3) ]
    fast_outputs_template = [ 'FAST_' + str(n) + '.mif' for n in range(0,3) ]
    for inpath, outpath in zip(fast_outputs_cropped, fast_outputs_template):
      run.command('mrtransform ' + inpath + ' -interp nearest -template ' + template_image + ' ' + outpath)
      app.cleanup(inpath)
      progress.increment()
    if app.ARGS.template:
      app.cleanup(template_image)

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
    app.cleanup(cerebellum_volume_image)
    progress.increment()
    run.command('mrconvert ' + tissue_images[0] + ' ' + new_tissue_images[0])
    app.cleanup(tissue_images[0])
    progress.increment()
    run.command('mrcalc ' + tissue_images[1] + ' ' + cerebellum_multiplier_image + ' ' + fast_outputs_template[1] + ' -mult -add ' + new_tissue_images[1])
    app.cleanup(tissue_images[1])
    app.cleanup(fast_outputs_template[1])
    progress.increment()
    run.command('mrcalc ' + tissue_images[2] + ' ' + cerebellum_multiplier_image + ' ' + fast_outputs_template[2] + ' -mult -add ' + new_tissue_images[2])
    app.cleanup(tissue_images[2])
    app.cleanup(fast_outputs_template[2])
    progress.increment()
    run.command('mrcalc ' + tissue_images[3] + ' ' + cerebellum_multiplier_image + ' ' + fast_outputs_template[0] + ' -mult -add ' + new_tissue_images[3])
    app.cleanup(tissue_images[3])
    app.cleanup(fast_outputs_template[0])
    app.cleanup(cerebellum_multiplier_image)
    progress.increment()
    run.command('mrconvert ' + tissue_images[4] + ' ' + new_tissue_images[4])
    app.cleanup(tissue_images[4])
    progress.increment()
    run.command('mrmath ' + ' '.join(new_tissue_images) + ' sum ' + new_tissue_sum_image)
    app.cleanup(tissue_sum_image)
    progress.done()
    tissue_images = new_tissue_images
    tissue_sum_image = new_tissue_sum_image



  # For all voxels within FreeSurfer's brain mask, add to the CSF image in order to make the sum 1.0
  # TESTME Rather than doing this blindly, look for holes in the brain, and assign the remainder to WM;
  #   only within the mask but outside the brain should the CSF fraction be filled
  # TODO Can definitely do better than just an erosion step here; still some hyper-intensities at GM-WW interface

  progress = app.ProgressBar('Performing fill operations to preserve unity tissue volume', 2)
  # TODO Connected-component analysis at high template image resolution is taking up huge amounts of memory
  # Crop beforehand? It's because it's filling everything outside the brain...
  # TODO Consider performing CSF fill before WM fill? Does this solve any problems?
  #   Well, it might mean I can use connected-components to select the CSF fill, rather than using an erosion
  #   This may also reduce the RAM usage of the connected-components analysis, since it'd be within
  #   the brain mask rather than using the whole image FoV.
  # Also: Potentially use mask cleaning filter
  # FIXME Appears we can't rely on a connected-component filter: Still some bits e.g. between cortex & cerebellum
  # Maybe need to look into techniques for specifically filling in between structure pairs

  # Some voxels may get a non-zero cortical GM fraction due to native use of the surface representation, yet
  #   these voxels are actually outside FreeSurfer's own provided brain mask. So what we need to do here is
  #   get the union of the tissue sum nonzero image and the mask image, and use that at the -mult step of the
  #   mrcalc call.
  # Required image: (tissue_sum_image > 0.0) || mask_image
  # tissue_sum_image 0.0 -gt mask_image -add 1.0 -min

  new_tissue_images = [ tissue_images[0],
                        tissue_images[1],
                        tissue_images[2],
                        os.path.splitext(tissue_images[3])[0] + '_filled.mif',
                        tissue_images[4] ]
  csf_fill_image = 'csf_fill.mif'
  run.command('mrcalc 1.0 ' + tissue_sum_image + ' -sub ' + tissue_sum_image + ' 0.0 -gt ' + mask_image + ' -add 1.0 -min -mult ' + csf_fill_image)
  app.cleanup(tissue_sum_image)
  # If no template is specified, this file is part of the FreeSurfer output; hence don't modify
  if app.ARGS.template:
    app.cleanup(mask_image)
  progress.increment()
  run.command('mrcalc ' + tissue_images[3] + ' ' + csf_fill_image + ' -add ' + new_tissue_images[3])
  app.cleanup(csf_fill_image)
  app.cleanup(tissue_images[3])
  progress.done()
  tissue_images = new_tissue_images


  # Finally, concatenate the volumes to produce the 5TT image
  precrop_result_image = '5TT.mif'
  if bs_cropmask_path:
    run.command('mrcat ' + ' '.join(tissue_images) + ' - -axis 3 | ' + \
                '5ttedit - ' + precrop_result_image + ' -none ' + bs_cropmask_path)
    app.cleanup(bs_cropmask_path)
  else:
    run.command('mrcat ' + ' '.join(tissue_images) + ' ' + precrop_result_image + ' -axis 3')
  app.cleanup(tissue_images)


  # Maybe don't go off all tissues here, since FreeSurfer's mask can be fairly liberal;
  #   instead get just a voxel clearance from all other tissue types (maybe two)
  if app.ARGS.nocrop:
    run.function(os.rename, precrop_result_image, 'result.mif')
  else:
    app.console('Cropping final 5TT image')
    crop_mask_image = 'crop_mask.mif'
    run.command('mrconvert ' + precrop_result_image + ' -coord 3 0,1,2,4 - | mrmath - sum - -axis 3 | mrthreshold - - -abs 0.001 | maskfilter - dilate ' + crop_mask_image)
    run.command('mrgrid ' + precrop_result_image + ' crop result.mif -mask ' + crop_mask_image)
    app.cleanup(crop_mask_image)
    app.cleanup(precrop_result_image)

  app.warn('Algorithm not capable of segmenting anterior commissure; '
           'recommend performing manual segmentation and using "5ttedit -wm"')
