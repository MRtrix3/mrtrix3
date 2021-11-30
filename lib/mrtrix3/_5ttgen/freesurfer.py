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

import glob, os.path, shutil
from mrtrix3 import MRtrixError
from mrtrix3 import app, image, path, run, utils


FIRST_SGM_LIST = {'L_Accu': 26, 'R_Accu': 58,
                  'L_Caud': 11, 'R_Caud': 50,
                  'L_Pall': 13, 'R_Pall': 52,
                  'L_Puta': 12, 'R_Puta': 51,
                  'L_Thal': 10, 'R_Thal': 49}

FIRST_AMYG_HIPP = {'L_Amyg': 18, 'R_Amyg': 54,
                   'L_Hipp': 17, 'R_Hipp': 53}


def usage(base_parser, subparsers): #pylint: disable=unused-variable
  parser = subparsers.add_parser('freesurfer', parents=[base_parser])
  parser.set_author('Robert E. Smith (robert.smith@florey.edu.au)')
  parser.set_synopsis('Generate the 5TT image based on a FreeSurfer parcellation image')
  parser.add_argument('input',  help='The input FreeSurfer parcellation image (any image containing \'aseg\' in its name)')
  parser.add_argument('output', help='The output 5TT image')
  options = parser.add_argument_group('Options specific to the \'freesurfer\' algorithm')
  options.add_argument('-sclimbic', metavar=('image'), help='Incorporate results of subcortical limbic segmentation')
  options.add_argument('-first', metavar=('path'), help='Use sub-cortical segmentations from FSL FIRST (see Description)')
  parser.add_description('Incorporating sub-cortical segmentations from FSL FIRST can be done in one of two ways. '
                         'If image "*_all_*_firstseg.nii[.gz]" is provided, then the hard segmentations from that image will be imported directly. '
                         'If instead the FIRST output directory is provided, then the surfaces will be used to produce partial volume estimates.')



def check_output_paths(): #pylint: disable=unused-variable
  app.check_output_path(app.ARGS.output)



def get_inputs(): #pylint: disable=unused-variable
  # Don't bother converting into scratch directory, as it's only used once
  #run.command('mrconvert ' + path.from_user(app.ARGS.input) + ' ' + path.to_scratch('input.mif'))
  if app.ARGS.first:
    first_path = path.from_user(app.ARGS.first, False)
    if os.path.isfile(first_path):
      if not image.match(path.from_user(app.ARGS.input, False), first_path):
        raise MRtrixError('Voxel grids of input index image and FIRST segmentation image do not match')
      run.command(['mrconvert', first_path, path.to_scratch('first_seg.mif', False)])
    elif os.path.isdir(first_path):
      # TODO Verify whether all requisite VTK files can be found, and have the same prefix
      sgm_list = list(FIRST_SGM_LIST.keys()) + (list(FIRST_AMYG_HIPP.keys()) if app.ARGS.sgm_amyg_hipp else [])
      vtk_filelist = []
      for first_sgm in sgm_list:
        candidate_files = glob.glob(os.path.join(first_path, '*-' + first_sgm + '_first.vtk'))
        if not candidate_files:
          raise MRtrixError('Unable to find VTK file for structure "' + first_sgm + '" in directory "' + first_path + '"')
        if len(candidate_files) > 1:
          raise MRtrixError('Multiple candidate VTK files for structure "' + first_sgm + '" in directory "' + first_path + '"')
        vtk_filelist.append(candidate_files[0])
      if not all(os.path.basename(vtk_file.split('-')[0]) == os.path.basename(vtk_filelist[0]).split('-')[0] for vtk_file in vtk_filelist[1:]):
        raise MRtrixError('VTK files in FIRST directory "' + first_path + '" do not all possess same prefix')
      # TODO Also need a template image in order to convert the VTKs
      firstseg_image = glob.glob(os.path.join(first_path, '*_all_*_firstseg.nii*'))
      if not firstseg_image:
        raise MRtrixError('Unable to find FIRST "firstseg" image (required as template for VTK conversion)')
      if len(firstseg_image) > 1:
        raise MRtrixError('Multiple candidate FIRST "firstseg" images (required as template for VTK conversion)')
      first_copy = utils.RunList('Copying FIRST .vtk files to scratch directory', len(sgm_list))
      for name, filepath in zip(sgm_list, vtk_filelist):
        first_copy.function(shutil.copyfile, filepath, path.to_scratch('first-' + name + '.vtk', False))
      run.command(['mrconvert', firstseg_image[0], path.to_scratch('first_template.mif', False)])


    else:
      raise MRtrixError('Unable to interpret input to -first option "' + app.ARGS.first + '" as filesystem path')



def execute(): #pylint: disable=unused-variable
  freesurfer_home = os.environ.get('FREESURFER_HOME', '')
  if not freesurfer_home:
    raise MRtrixError('Environment variable FREESURFER_HOME is not set; please run appropriate FreeSurfer configuration script, set this variable manually, or provide script with path to file FreeSurferColorLUT.txt using -lut option')
  lut_input_path = os.path.join(freesurfer_home, 'FreeSurferColorLUT.txt')
  if not os.path.isfile(lut_input_path):
    raise MRtrixError('Could not find FreeSurfer lookup table file (expected location: ' + lut_input_path + ')')

  if app.ARGS.sclimbic:
    sclimbic_lut_input_path = os.path.join(freesurfer_home, 'models', 'sclimbic.ctab')
    if not os.path.isfile(sclimbic_lut_input_path):
      raise MRtrixError('Could not find FreeSurfer ScLimbic module lookup table file (expected location: ' + sclimbic_lut_input_path + ')')

  if app.ARGS.sgm_amyg_hipp:
    lut_output_file_name = 'FreeSurfer2ACT_sgm_amyg_hipp.txt'
  else:
    lut_output_file_name = 'FreeSurfer2ACT.txt'
  lut_output_path = os.path.join(path.shared_data_path(), path.script_subdir_name(), lut_output_file_name)
  if not os.path.isfile(lut_output_path):
    raise MRtrixError('Could not find lookup table file for converting FreeSurfer parcellation output to tissues (expected location: ' + lut_output_path + ')')
  if app.ARGS.sclimbic:
    sclimbic_lut_output_path = os.path.join(path.shared_data_path(), path.script_subdir_name(), 'ScLimbic2ACT.txt')
    if not os.path.isfile(sclimbic_lut_output_path):
      raise MRtrixError('Could not find lookup table file for converting ScLimbic module output to tissues (expected location: ' + sclimbic_lut_output_path + ')')

  # Initial conversion from FreeSurfer parcellation to five principal tissue types
  index_image = 'indices.mif'
  run.command(['labelconvert', path.from_user(app.ARGS.input, False), lut_input_path, lut_output_path, index_image])
  if app.ARGS.sclimbic:
    sclimbic_image = 'sclimbic.mif'
    run.command(['labelconvert', path.from_user(app.ARGS.sclimbic, False), sclimbic_lut_input_path, sclimbic_lut_output_path, sclimbic_image])

  # Generate separate images per tissue
  tissue_images = ['tissue' + str(index) + '.mif' for index in range(1, 6)]
  split_vols = utils.RunList('Splitting tissue index image into per-tissue images', 5)
  for index, filename in zip(range(1, 6), tissue_images):
    if app.ARGS.sclimbic:
      split_vols.command('mrcalc ' + sclimbic_image + ' 0 ' + index_image + ' -if ' + str(index) + ' -eq ' + sclimbic_image + ' ' + str(index) + ' -eq -add ' + filename)
    else:
      split_vols.command('mrcalc ' + index_image + ' ' + str(index) + ' -eq ' + filename)

  # TODO Integrating data from FIRST requires that the tissues have been separated,
  #   since it's possible to have soft segmentations that necessitate partial volume fractions

  # Integrate data from FIRST
  if app.ARGS.first:
    sgm_list = FIRST_SGM_LIST
    if app.ARGS.sgm_amyg_hipp:
      sgm_list.update(FIRST_AMYG_HIPP)
    # Strip out SGM from FreeSurfer; put it back into WM
    # Pointless; this code now appears after conversion to 1 image per tissue
    #run.command('mrcalc ' + index_image + ' 1 -eq 2 ' + index_image + ' -if indices_nosgm.mif')
    new_sgm_image = 'sgm.mif'
    new_tissue_images = ['tissue' + str(index) + '_first.mif' for index in range(1, 6)]
    if os.path.isfile('first_seg.mif'):
      # Hard segmentation
      run.command('mrcalc '
                  + ' '.join('first_seg.mif ' + str(index) + ' -eq' for index in sgm_list.values())
                  + ' '
                  + ' '.join(['-add'] * (len(sgm_list)-1))
                  + ' 1.0 -min '
                  + new_sgm_image)
      # TODO Update WM and SGM tissue images
      # If SGM:
      #   Set to SGM; erase from all others
      # Else if old SGM (and not new SGM):
      #   Remove from SGM, set to WM
      # Else:
      #   Use old tissue
      # TODO Redo logic on a per-tissue basis
      # For SGM:
      #   Simply replace with new image
      # For WM:
      #   Take old SGM and add it in
      #   Remove anything that's present in the new SGM
      # For everything else:
      #   Remove anything that's present in the new SGM
      first_replace = utils.RunList('Incorporating FIRST segmentations into tissue images', 5)
      for index, input_image, output_image in zip(range(1, 6), tissue_images, new_tissue_images):
        if index == 2:
          first_replace.function(shutil.copyfile, new_sgm_image, output_image)
        else:
          first_replace.command('mrcalc ' + input_image + ' ' + (tissue_images[1] + ' -add ' if index == 3 else '') + new_sgm_image + ' -sub 0 -max ' + output_image)
    else:
      # Soft segmentation
      first_pve = utils.RunList('Mapping FIRST segmentations to partial volumes', 2 * len(sgm_list))
      for struct in sgm_list.keys():
        filename = 'first-' + struct + '.vtk'
        intername = 'real-' + struct + '.vtk'
        # TODO Modify first2real to read from new header transform realignment parameters and modify transform accordingly
        first_pve.command(['meshconvert', filename, intername, '-transform', 'first2real', 'first_template.mif', '-config', 'RealignTransform', 'false'])
        first_pve.command(['mesh2voxel', intername, index_image, 'first-' + struct + '.mif'])
      run.command(['mrmath', ['first-' + struct + '.mif' for struct in sgm_list.keys()], 'sum', '-', '|',
                   'mrcalc', '-', '1.0', '-min', index_image, '0', '-gt', '-mult', new_sgm_image])
      # Insert into index image
      # TODO More complex than the hard segmentation approach:
      # Rather than a direct if replacement, need to retain some fraction of the existing tissue segmentation
      # TODO Fix: Erroneous voxels at SGM edges
      run.command('mrcalc 1 sgm.mif -sub fs_multiplier.mif')
      first_modulate = utils.RunList('Incorporating FIRST segmentations into tissue images', 5)
      for index, input_image, output_image in zip(range(1, 6), tissue_images, new_tissue_images):
        if index == 2:
          first_modulate.function(shutil.copy, new_sgm_image, output_image)
        else:
          first_modulate.command('mrcalc ' + input_image + ' ' + (tissue_images[1] + ' -add ' if index == 3 else '') + 'fs_multiplier.mif -mult ' + output_image)
    tissue_images = new_tissue_images

  final_command = ['mrcat', tissue_images, '-', '-axis', '3', '|']
  if not app.ARGS.nocrop:
    final_command.extend(['mrgrid', '-', 'crop', '-mask', 'indices.mif', '-', '|'])
  final_command.extend(['mrconvert', '-', 'result.mif', '-datatype', 'float32'])
  run.command(final_command)

  run.command('mrconvert result.mif ' + path.from_user(app.ARGS.output),
              mrconvert_keyval=path.from_user(app.ARGS.input, False),
              force=app.FORCE_OVERWRITE)
