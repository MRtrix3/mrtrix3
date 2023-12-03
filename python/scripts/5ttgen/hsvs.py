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



import glob, os, re, shutil
from mrtrix3 import MRtrixError
from mrtrix3 import app, fsl, image, path, run



HIPPOCAMPI_CHOICES = [ 'subfields', 'first', 'aseg' ]
THALAMI_CHOICES = [ 'nuclei', 'first', 'aseg' ]

# Have not had success segmenting the posterior commissure
# FAST just doesn't run well there, at least with a severely reduced window of data
# Though it may actually get envelloped within the brain stem, in which case it's not as much of a concern
ATTEMPT_PC = False


def usage(base_parser, subparsers): #pylint: disable=unused-variable
  parser = subparsers.add_parser('hsvs', parents=[base_parser])
  parser.set_author('Robert E. Smith (robert.smith@florey.edu.au)')
  parser.set_synopsis('Generate a 5TT image based on Hybrid Surface and Volume Segmentation (HSVS), using FreeSurfer and FSL tools')
  parser.add_argument('input', help='The input FreeSurfer subject directory')
  parser.add_argument('output', help='The output 5TT image')
  parser.add_argument('-template', help='Provide an image that will form the template for the generated 5TT image')
  parser.add_argument('-hippocampi', choices=HIPPOCAMPI_CHOICES, help='Select method to be used for hippocampi (& amygdalae) segmentation; options are: ' + ','.join(HIPPOCAMPI_CHOICES))
  parser.add_argument('-thalami', choices=THALAMI_CHOICES, help='Select method to be used for thalamic segmentation; options are: ' + ','.join(THALAMI_CHOICES))
  parser.add_argument('-white_stem', action='store_true', help='Classify the brainstem as white matter')
  parser.add_citation('Smith, R.; Skoch, A.; Bajada, C.; Caspers, S.; Connelly, A. Hybrid Surface-Volume Segmentation for improved Anatomically-Constrained Tractography. In Proc OHBM 2020')
  parser.add_citation('Fischl, B. Freesurfer. NeuroImage, 2012, 62(2), 774-781', is_external=True)
  parser.add_citation('Iglesias, J.E.; Augustinack, J.C.; Nguyen, K.; Player, C.M.; Player, A.; Wright, M.; Roy, N.; Frosch, M.P.; Mc Kee, A.C.; Wald, L.L.; Fischl, B.; and Van Leemput, K. A computational atlas of the hippocampal formation using ex vivo, ultra-high resolution MRI: Application to adaptive segmentation of in vivo MRI. NeuroImage, 2015, 115, 117-137', condition='If FreeSurfer hippocampal subfields module is utilised', is_external=True)
  parser.add_citation('Saygin, Z.M. & Kliemann, D.; Iglesias, J.E.; van der Kouwe, A.J.W.; Boyd, E.; Reuter, M.; Stevens, A.; Van Leemput, K.; Mc Kee, A.; Frosch, M.P.; Fischl, B.; Augustinack, J.C. High-resolution magnetic resonance imaging reveals nuclei of the human amygdala: manual segmentation to automatic atlas. NeuroImage, 2017, 155, 370-382', condition='If FreeSurfer hippocampal subfields module is utilised and includes amygdalae segmentation', is_external=True)
  parser.add_citation('Iglesias, J.E.; Insausti, R.; Lerma-Usabiaga, G.; Bocchetta, M.; Van Leemput, K.; Greve, D.N.; van der Kouwe, A.; ADNI; Fischl, B.; Caballero-Gaudes, C.; Paz-Alonso, P.M. A probabilistic atlas of the human thalamic nuclei combining ex vivo MRI and histology. NeuroImage, 2018, 183, 314-326', condition='If -thalami nuclei is used', is_external=True)
  parser.add_citation('Ardekani, B.; Bachman, A.H. Model-based automatic detection of the anterior and posterior commissures on MRI scans. NeuroImage, 2009, 46(3), 677-682', condition='If ACPCDetect is installed', is_external=True)







ASEG_STRUCTURES = [ ( 5,  4, 'Left-Inf-Lat-Vent'),
                    (14,  4, '3rd-Ventricle'),
                    (15,  4, '4th-Ventricle'),
                    (24,  4, 'CSF'),
                    (25,  5, 'Left-Lesion'),
                    (30,  4, 'Left-vessel'),
                    (44,  4, 'Right-Inf-Lat-Vent'),
                    (57,  5, 'Right-Lesion'),
                    (62,  4, 'Right-vessel'),
                    (72,  4, '5th-Ventricle'),
                    (250, 3, 'Fornix') ]


HIPP_ASEG = [ (17,  2, 'Left-Hippocampus'),
              (53,  2, 'Right-Hippocampus') ]

AMYG_ASEG = [ (18,  2, 'Left-Amygdala'),
              (54,  2, 'Right-Amygdala') ]

THAL_ASEG = [ (10,  2, 'Left-Thalamus-Proper'),
              (49,  2, 'Right-Thalamus-Proper') ]

OTHER_SGM_ASEG = [ (11,  2, 'Left-Caudate'),
                   (12,  2, 'Left-Putamen'),
                   (13,  2, 'Left-Pallidum'),
                   (26,  2, 'Left-Accumbens-area'),
                   (50,  2, 'Right-Caudate'),
                   (51,  2, 'Right-Putamen'),
                   (52,  2, 'Right-Pallidum'),
                   (58,  2, 'Right-Accumbens-area') ]


VENTRICLE_CP_ASEG = [ [ ( 4,  4, 'Left-Lateral-Ventricle'),
                        (31,  4, 'Left-choroid-plexus')     ],
                      [ (43,  4, 'Right-Lateral-Ventricle'),
                        (63,  4, 'Right-choroid-plexus')    ] ]


CEREBELLUM_ASEG = [ ( 6,  4, 'Left-Cerebellum-Exterior'),
                    ( 7,  3, 'Left-Cerebellum-White-Matter'),
                    ( 8,  2, 'Left-Cerebellum-Cortex'),
                    (45,  4, 'Right-Cerebellum-Exterior'),
                    (46,  3, 'Right-Cerebellum-White-Matter'),
                    (47,  2, 'Right-Cerebellum-Cortex') ]


CORPUS_CALLOSUM_ASEG = [ (192, 'Corpus_Callosum'),
                         (251, 'CC_Posterior'),
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
  app.check_output_path(app.ARGS.output)



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

  acpc_string = 'anterior ' + ('& posterior commissures' if ATTEMPT_PC else 'commissure')
  have_acpcdetect = bool(shutil.which('acpcdetect')) and 'ARTHOME' in os.environ
  if have_acpcdetect:
    if have_fast:
      app.console('ACPCdetect and FSL FAST will be used for explicit segmentation of ' + acpc_string)
    else:
      app.warn('ACPCdetect is installed, but FSL FAST not found; cannot segment ' + acpc_string)
      have_acpcdetect = False
  else:
    app.warn('ACPCdetect not installed; cannot segment ' + acpc_string)

  # Need to perform a better search for hippocampal subfield output: names & version numbers may change
  have_hipp_subfields = False
  hipp_subfield_has_amyg = False
  # Could result in multiple matches
  hipp_subfield_regex = re.compile(r'^[lr]h\.hippo[a-zA-Z]*Labels-[a-zA-Z0-9]*\.v[0-9]+\.?[a-zA-Z0-9]*\.mg[hz]$')
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
    if any(code in filename for filename in hipp_subfield_paired_images):
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

  # Perform a similar search for thalamic nuclei submodule output
  thal_nuclei_image = None
  thal_nuclei_regex = re.compile(r'^ThalamicNuclei\.v[0-9]+\.?[a-zA-Z0-9]*.mg[hz]$')
  thal_nuclei_all_images = sorted(list(filter(thal_nuclei_regex.match, os.listdir(mri_dir))))
  thal_nuclei_all_images = [ item for item in thal_nuclei_all_images if 'FSvoxelSpace' not in item ]
  if thal_nuclei_all_images:
    if len(thal_nuclei_all_images) == 1:
      thal_nuclei_image = thal_nuclei_all_images[0]
    else:
      # How to choose which version to use?
      # Start with software version
      thal_nuclei_versions = [ int(item.split('.')[1].lstrip('v')) for item in thal_nuclei_all_images ]
      thal_nuclei_all_images = [ filepath for filepath, version_number in zip(thal_nuclei_all_images, thal_nuclei_versions) if version_number == max(thal_nuclei_versions) ]
      if len(thal_nuclei_all_images) == 1:
        thal_nuclei_image = thal_nuclei_all_images[0]
      else:
        # Revert to filename length
        thal_nuclei_all_images = sorted(thal_nuclei_all_images, key=len)
        thal_nuclei_image = thal_nuclei_all_images[0]

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
      app.console('Hippocampal subfields module output detected; will utilise for hippocampi '
                  + ('and amygdalae ' if hipp_subfield_has_amyg else '')
                  + 'segmentation')
    elif have_first:
      hippocampi_method = 'first'
      app.console('No hippocampal subfields module output detected, but FSL FIRST is installed; '
                  'will utilise latter for hippocampi segmentation')
    else:
      hippocampi_method = 'aseg'
      app.console('Neither hippocampal subfields module output nor FSL FIRST detected; '
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
             '(hsvs algorithm always assigns hippocampi & ampygdalae as sub-cortical grey matter)')


  # Similar logic for thalami
  thalami_method = app.ARGS.thalami
  if thalami_method:
    if thalami_method == 'nuclei':
      if not thal_nuclei_image:
        raise MRtrixError('Could not find thalamic nuclei module output')
    elif thalami_method == 'first':
      if not have_first:
        raise MRtrixError('Cannot use "first" method for thalami segmentation; check FSL installation')
  else:
    # Not happy with outputs of thalamic nuclei submodule; default to FIRST
    if have_first:
      thalami_method = 'first'
      if thal_nuclei_image:
        app.console('Thalamic nuclei submodule output ignored in favour of FSL FIRST '
                    '(can override using -thalami option)')
      else:
        app.console('Will utilise FSL FIRST for thalami segmentation')
    elif thal_nuclei_image:
      thalami_method = 'nuclei'
      app.console('Will utilise detected thalamic nuclei submodule output')
    else:
      thalami_method = 'aseg'
      app.console('Neither thalamic nuclei module output nor FSL FIRST detected; '
                  'FreeSurfer aseg will be used for thalami segmentation')


  ###########################
  # Commencing segmentation #
  ###########################

  tissue_images = [ [ 'lh.pial.mif', 'rh.pial.mif' ],
                    [],
                    [ 'lh.white.mif', 'rh.white.mif' ],
                    [],
                    [] ]

  # Get the main cerebrum segments; these are already smooth
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
  from_aseg = list(ASEG_STRUCTURES)
  if hippocampi_method == 'subfields':
    if not hipp_subfield_has_amyg and not have_first:
      from_aseg.extend(AMYG_ASEG)
  elif hippocampi_method == 'aseg':
    from_aseg.extend(HIPP_ASEG)
    from_aseg.extend(AMYG_ASEG)
  if thalami_method == 'aseg':
    from_aseg.extend(THAL_ASEG)
  if not have_first:
    from_aseg.extend(OTHER_SGM_ASEG)
  progress = app.ProgressBar('Smoothing non-cortical structures segmented by FreeSurfer', len(from_aseg) + 2)
  for (index, tissue, name) in from_aseg:
    init_mesh_path = name + '_init.vtk'
    smoothed_mesh_path = name + '.vtk'
    run.command('mrcalc ' + aparc_image + ' ' + str(index) + ' -eq - | voxel2mesh - -threshold 0.5 ' + init_mesh_path)
    run.command('meshfilter ' + init_mesh_path + ' smooth ' + smoothed_mesh_path)
    app.cleanup(init_mesh_path)
    run.command('mesh2voxel ' + smoothed_mesh_path + ' ' + template_image + ' ' + name + '.mif')
    app.cleanup(smoothed_mesh_path)
    tissue_images[tissue-1].append(name + '.mif')
    progress.increment()
  # Lateral ventricles are separate as we want to combine with choroid plexus prior to mesh conversion
  for hemi_index, hemi_name in enumerate(['Left', 'Right']):
    name = hemi_name + '_LatVent_ChorPlex'
    init_mesh_path = name + '_init.vtk'
    smoothed_mesh_path = name + '.vtk'
    run.command('mrcalc ' + ' '.join(aparc_image + ' ' + str(index) + ' -eq' for index, tissue, name in VENTRICLE_CP_ASEG[hemi_index]) + ' -add - | '
                + 'voxel2mesh - -threshold 0.5 ' + init_mesh_path)
    run.command('meshfilter ' + init_mesh_path + ' smooth ' + smoothed_mesh_path)
    app.cleanup(init_mesh_path)
    run.command('mesh2voxel ' + smoothed_mesh_path + ' ' + template_image + ' ' + name + '.mif')
    app.cleanup(smoothed_mesh_path)
    tissue_images[3].append(name + '.mif')
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
  for name in [ n for _, n in CORPUS_CALLOSUM_ASEG ]:
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
  bs_fullmask_path = 'brain_stem_init.mif'
  bs_cropmask_path = ''
  progress = app.ProgressBar('Segmenting and cropping brain stem', 5)
  run.command('mrcalc ' + aparc_image + ' ' + str(BRAIN_STEM_ASEG[0][0]) + ' -eq '
              + ' -add '.join([ aparc_image + ' ' + str(index) + ' -eq' for index, name in BRAIN_STEM_ASEG[1:] ]) + ' -add '
              + bs_fullmask_path + ' -datatype bit')
  progress.increment()
  bs_init_mesh_path = 'brain_stem_init.vtk'
  run.command('voxel2mesh ' + bs_fullmask_path + ' ' + bs_init_mesh_path)
  progress.increment()
  bs_smoothed_mesh_path = 'brain_stem.vtk'
  run.command('meshfilter ' + bs_init_mesh_path + ' smooth ' + bs_smoothed_mesh_path)
  app.cleanup(bs_init_mesh_path)
  progress.increment()
  run.command('mesh2voxel ' + bs_smoothed_mesh_path + ' ' + template_image + ' brain_stem.mif')
  app.cleanup(bs_smoothed_mesh_path)
  progress.increment()
  fourthventricle_zmin = min(int(line.split()[2]) for line in run.command('maskdump 4th-Ventricle.mif')[0].splitlines())
  if fourthventricle_zmin:
    bs_cropmask_path = 'brain_stem_crop.mif'
    run.command('mredit brain_stem.mif - ' + ' '.join([ '-plane 2 ' + str(index) + ' 0' for index in range(0, fourthventricle_zmin) ]) + ' | '
                'mrcalc brain_stem.mif - -sub 1e-6 -gt ' + bs_cropmask_path + ' -datatype bit')
  app.cleanup(bs_fullmask_path)
  progress.done()


  if hippocampi_method == 'subfields':
    progress = app.ProgressBar('Using detected FreeSurfer hippocampal subfields module output',
                               64 if hipp_subfield_has_amyg else 32)

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


  if thalami_method == 'nuclei':
    progress = app.ProgressBar('Using detected FreeSurfer thalamic nuclei module output', 6)
    for hemi in ['Left', 'Right']:
      thal_mask_path = hemi + '_Thalamus_mask.mif'
      init_mesh_path = hemi + '_Thalamus_init.vtk'
      smooth_mesh_path = hemi + '_Thalamus.vtk'
      thalamus_image = hemi + '_Thalamus.mif'
      if hemi == 'Right':
        run.command('mrthreshold ' + os.path.join(mri_dir, thal_nuclei_image) + ' -abs 8200 ' + thal_mask_path)
      else:
        run.command('mrcalc ' + os.path.join(mri_dir, thal_nuclei_image) + ' 0 -gt '
                    + os.path.join(mri_dir, thal_nuclei_image) + ' 8200 -lt '
                    + '-mult ' + thal_mask_path)
      run.command('voxel2mesh ' + thal_mask_path + ' ' + init_mesh_path)
      app.cleanup(thal_mask_path)
      progress.increment()
      run.command('meshfilter ' + init_mesh_path + ' smooth ' + smooth_mesh_path + ' -smooth_spatial 2 -smooth_influence 2')
      app.cleanup(init_mesh_path)
      progress.increment()
      run.command('mesh2voxel ' + smooth_mesh_path + ' ' + template_image + ' ' + thalamus_image)
      app.cleanup(smooth_mesh_path)
      progress.increment()
      tissue_images[1].append(thalamus_image)
    progress.done()

  if have_first:
    app.console('Running FSL FIRST to segment sub-cortical grey matter structures')
    from_first = SGM_FIRST_MAP.copy()
    if hippocampi_method == 'subfields':
      from_first = { key: value for key, value in from_first.items() if 'Hippocampus' not in value }
      if hipp_subfield_has_amyg:
        from_first = { key: value for key, value in from_first.items() if 'Amygdala' not in value }
    elif hippocampi_method == 'aseg':
      from_first = { key: value for key, value in from_first.items() if 'Hippocampus' not in value and 'Amygdala' not in value }
    if thalami_method != 'first':
      from_first = { key: value for key, value in from_first.items() if 'Thalamus' not in value }
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

  # Run ACPCdetect, use results to draw spherical ROIs on T1 that will be fed to FSL FAST,
  #   the WM components of which will then be added to the 5TT
  if have_acpcdetect:
    progress = app.ProgressBar('Using ACPCdetect and FAST to segment ' + acpc_string, 5)
    # ACPCdetect requires input image to be 16-bit
    # We also want to realign to RAS beforehand so that we can interpret the output voxel locations properly
    acpcdetect_input_image = 'T1RAS_16b.nii'
    run.command('mrconvert ' + norm_image + ' -datatype uint16 -stride +1,+2,+3 ' + acpcdetect_input_image)
    progress.increment()
    run.command('acpcdetect -i ' + acpcdetect_input_image)
    progress.increment()
    # We need the header in order to go from voxel coordinates to scanner coordinates
    acpcdetect_input_header = image.Header(acpcdetect_input_image)
    acpcdetect_output_path = os.path.splitext(acpcdetect_input_image)[0] + '_ACPC.txt'
    app.cleanup(acpcdetect_input_image)
    with open(acpcdetect_output_path, 'r', encoding='utf-8') as acpc_file:
      acpcdetect_output_data = acpc_file.read().splitlines()
    app.cleanup(glob.glob(os.path.splitext(acpcdetect_input_image)[0] + "*"))
    # Need to scan through the contents of this file,
    #   isolating the AC and PC locations
    ac_voxel = pc_voxel = None
    for index, line in enumerate(acpcdetect_output_data):
      if 'AC' in line and 'voxel location' in line:
        ac_voxel = [float(item) for item in acpcdetect_output_data[index+1].strip().split()]
      elif 'PC' in line and 'voxel location' in line:
        pc_voxel = [float(item) for item in acpcdetect_output_data[index+1].strip().split()]
    if not ac_voxel or not pc_voxel:
      raise MRtrixError('Error parsing text file from "acpcdetect"')

    def voxel2scanner(voxel, header):
      return [ voxel[0]*header.spacing()[0]*header.transform()[axis][0]
               + voxel[1]*header.spacing()[1]*header.transform()[axis][1]
               + voxel[2]*header.spacing()[2]*header.transform()[axis][2]
               + header.transform()[axis][3]
               for axis in range(0,3) ]

    ac_scanner = voxel2scanner(ac_voxel, acpcdetect_input_header)
    pc_scanner = voxel2scanner(pc_voxel, acpcdetect_input_header)

    # Generate the mask image within which FAST will be run
    acpc_prefix = 'ACPC' if ATTEMPT_PC else 'AC'
    acpc_mask_image = acpc_prefix + '_FAST_mask.mif'
    run.command('mrcalc ' + template_image + ' nan -eq - | '
                'mredit - ' + acpc_mask_image + ' -scanner '
                '-sphere ' + ','.join(str(value) for value in ac_scanner) + ' 8 1 '
                + ('-sphere ' + ','.join(str(value) for value in pc_scanner) + ' 5 1' if ATTEMPT_PC else ''))
    progress.increment()

    acpc_t1_masked_image = acpc_prefix + '_T1.nii'
    run.command('mrtransform ' + norm_image + ' -template ' + template_image + ' - | '
                'mrcalc - ' + acpc_mask_image + ' -mult ' + acpc_t1_masked_image)
    app.cleanup(acpc_mask_image)
    progress.increment()

    run.command(fast_cmd + ' -N ' + acpc_t1_masked_image)
    app.cleanup(acpc_t1_masked_image)
    progress.increment()

    # Ideally don't want to have to add these manually; instead add all outputs from FAST
    #   to the 5TT (both cerebellum and AC / PC) in a single go
    # This should involve grabbing just the WM component of these images
    # Actually, in retrospect, it may be preferable to do the AC PC segmentation
    #   earlier on, and simply add them to the list of WM structures
    acpc_wm_image = acpc_prefix + '.mif'
    run.command('mrconvert ' + fsl.find_image(acpc_prefix + '_T1_pve_2') + ' ' + acpc_wm_image)
    tissue_images[2].append(acpc_wm_image)
    app.cleanup(glob.glob(os.path.splitext(acpc_t1_masked_image)[0] + '*'))
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
    run.command('mrmath ' + ' '.join(tissue_images[tissue]) + (' brain_stem.mif' if tissue == 2 else '') + ' sum - | mrcalc - 1.0 -min tissue' + str(tissue) + '_init.mif')
    app.cleanup(tissue_images[tissue])
    progress.increment()
  progress.done()


  # This can hopefully be done with a connected-component analysis: Take just the WM image, and
  #   fill in any gaps (i.e. select the inverse, select the largest connected component, invert again)
  # Make sure that floating-point values are handled appropriately
  # Combine these images together using the appropriate logic in order to form the 5TT image
  progress = app.ProgressBar('Modulating segmentation images based on other tissues', 9)
  tissue_images = [ 'tissue0.mif', 'tissue1.mif', 'tissue2.mif', 'tissue3.mif', 'tissue4.mif' ]
  run.function(os.rename, 'tissue4_init.mif', 'tissue4.mif')
  progress.increment()
  run.command('mrcalc tissue3_init.mif tissue3_init.mif ' + tissue_images[4] + ' -add 1.0 -sub 0.0 -max -sub 0.0 -max ' + tissue_images[3])
  app.cleanup('tissue3_init.mif')
  progress.increment()
  run.command('mrmath ' + ' '.join(tissue_images[3:5]) + ' sum tissuesum_34.mif')
  progress.increment()
  run.command('mrcalc tissue1_init.mif tissue1_init.mif tissuesum_34.mif -add 1.0 -sub 0.0 -max -sub 0.0 -max ' + tissue_images[1])
  app.cleanup('tissue1_init.mif')
  app.cleanup('tissuesum_34.mif')
  progress.increment()
  run.command('mrmath ' + tissue_images[1] + ' ' + ' '.join(tissue_images[3:5]) + ' sum tissuesum_134.mif')
  progress.increment()
  run.command('mrcalc tissue2_init.mif tissue2_init.mif tissuesum_134.mif -add 1.0 -sub 0.0 -max -sub 0.0 -max ' + tissue_images[2])
  app.cleanup('tissue2_init.mif')
  app.cleanup('tissuesum_134.mif')
  progress.increment()
  run.command('mrmath ' + ' '.join(tissue_images[1:5]) + ' sum tissuesum_1234.mif')
  progress.increment()
  run.command('mrcalc tissue0_init.mif tissue0_init.mif tissuesum_1234.mif -add 1.0 -sub 0.0 -max -sub 0.0 -max ' + tissue_images[0])
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
    # In that case, let's find a multiplier to apply to cerebellum tissues such that the
    #   sum does not exceed 1.0
    new_tissue_images = [ 'tissue0_fast.mif', 'tissue1_fast.mif', 'tissue2_fast.mif', 'tissue3_fast.mif', 'tissue4_fast.mif' ]
    new_tissue_sum_image = 'tissuesum_01234_fast.mif'
    cerebellum_multiplier_image = 'Cerebellar_multiplier.mif'
    run.command('mrcalc ' + cerebellum_volume_image + ' ' + tissue_sum_image + ' -add 0.5 -gt 1.0 ' + tissue_sum_image + ' -sub 0.0 -if  ' + cerebellum_multiplier_image)
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
  progress = app.ProgressBar('Performing fill operations to preserve unity tissue volume', 2)

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
  run.command('mrcalc 1.0 ' + tissue_sum_image + ' -sub ' + tissue_sum_image + ' 0.0 -gt ' + mask_image + ' -add 1.0 -min -mult 0.0 -max ' + csf_fill_image)
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



  # Move brain stem from white matter to pathology at final step:
  #   this prevents the brain stem segmentation from overwriting other
  #   structures that it otherwise wouldn't if it were written to WM
  if not app.ARGS.white_stem:
    progress = app.ProgressBar('Moving brain stem to volume index 4', 3)
    new_tissue_images = [ tissue_images[0],
                          tissue_images[1],
                          os.path.splitext(tissue_images[2])[0] + '_no_brainstem.mif',
                          tissue_images[3],
                          os.path.splitext(tissue_images[4])[0] + '_with_brainstem.mif' ]
    run.command('mrcalc ' + tissue_images[2] + ' brain_stem.mif -min brain_stem_white_overlap.mif')
    app.cleanup('brain_stem.mif')
    progress.increment()
    run.command('mrcalc ' + tissue_images[2] + ' brain_stem_white_overlap.mif -sub ' + new_tissue_images[2])
    app.cleanup(tissue_images[2])
    progress.increment()
    run.command('mrcalc ' + tissue_images[4] + ' brain_stem_white_overlap.mif -add ' + new_tissue_images[4])
    app.cleanup(tissue_images[4])
    app.cleanup('brain_stem_white_overlap.mif')
    progress.done()
    tissue_images = new_tissue_images



  # Finally, concatenate the volumes to produce the 5TT image
  app.console('Concatenating tissue volumes into 5TT format')
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

  run.command('mrconvert result.mif ' + path.from_user(app.ARGS.output),
              mrconvert_keyval=path.from_user(os.path.join(app.ARGS.input, 'mri', 'aparc+aseg.mgz'), True),
              force=app.FORCE_OVERWRITE)
