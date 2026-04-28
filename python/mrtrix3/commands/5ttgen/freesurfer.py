# Copyright (c) 2008-2026 the MRtrix3 contributors.
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

import os, pathlib, shutil
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
  parser.add_argument('input',
                      type=app.Parser.ImageIn(),
                      help='The input FreeSurfer parcellation image '
                           '(any image containing "aseg" in its name)')
  parser.add_argument('output',
                      type=app.Parser.ImageOut(),
                      help='The output 5TT image')
  parser.add_description('Incorporating sub-cortical segmentations from FSL FIRST can be done in one of two ways.'
                         ' If image "*_all_*_firstseg.nii[.gz]" is provided,'
                         ' then the hard segmentations from that image will be imported directly.'
                         ' If instead the FIRST output directory is provided,'
                         ' then the surfaces will be used to produce partial volume estimates'
                         ' (without explicit smoothing for computational efficiency).'
                         ' The former can slightly over-estimate the sizes of these structures,'
                         ' and produces a binary tissue segmentation;'
                         ' the latter is more precise and provides partial volume fractions'
                         ' but is more computationall expensive.')
  options = parser.add_argument_group('Options specific to the "freesurfer" algorithm')
  options.add_argument('-lut',
                       type=app.Parser.FileIn(),
                       help='Manually provide path to the lookup table on which the input parcellation image is based '
                            '(e.g. FreeSurferColorLUT.txt)')
  options.add_argument('-sclimbic',
                       type=app.Parser.ImageIn(),
                       help='Incorporate results of subcortical limbic segmentation;'
                            ' path should be default label image output from FreeSurfer mri_sclimbic_seg')
  options.add_argument('-first',
                       # TODO Make a custom type that can be either a directory or an image
                       metavar=('path'),
                       help='Use sub-cortical segmentations from FSL FIRST (see Description)')

def execute(): #pylint: disable=unused-variable

  freesurfer_home = os.environ.get('FREESURFER_HOME', '')
  lut_input_path = app.ARGS.lut # May be None
  if not freesurfer_home and (lut_input_path is None or app.ARGS.sclimbic is not None):
    raise MRtrixError('Environment variable FREESURFER_HOME is not set;'
                      ' please run appropriate FreeSurfer configuration script'
                      + (', set this variable manually,'
                         ' or provide script with path to file FreeSurferColorLUT.txt using -lut option'
                         if app.ARGS.sclimbic is None
                         else ' or set this variable manually'))
  if lut_input_path is None:
    lut_input_path = pathlib.Path(freesurfer_home, 'FreeSurferColorLUT.txt')
    if not lut_input_path.is_file():
      raise MRtrixError('Could not find FreeSurfer lookup table file'
                        f' (expected location: {lut_input_path})')
  if app.ARGS.sclimbic:
    sclimbic_lut_input_path = pathlib.Path(freesurfer_home, 'models', 'sclimbic.ctab')
    if not sclimbic_lut_input_path.is_file():
      raise MRtrixError('Could not find FreeSurfer ScLimbic module lookup table file'
                        f' (expected location: {sclimbic_lut_input_path})')

  run.command(['mrconvert', app.ARGS.input, 'input.mif'],
              preserve_pipes=True)
  if app.VERBOSITY >= 3:
    run.command('labelvalidate input.mif')

  if app.ARGS.first is not None:
    # TODO Conversion should not be required if option type is changed
    first_path = pathlib.Path(app.WORKING_DIR, app.ARGS.first)
    if first_path.is_file():
      if not image.match('input.mif', first_path):
        raise MRtrixError('Voxel grids of input index image and FIRST segmentation image do not match')
      run.command(['mrconvert', first_path, 'first_seg.mif'],
                  preserve_pipes=True)
    elif first_path.is_dir():

      # Verify whether all requisite VTK files can be found, and have the same prefix
      sgm_list = list(FIRST_SGM_LIST.keys()) + (list(FIRST_AMYG_HIPP.keys()) if app.ARGS.sgm_amyg_hipp else [])
      vtk_filelist = []
      for first_sgm in sgm_list:
        candidate_files = list(first_path.glob(f'*-{first_sgm}_first.vtk'))
        if not candidate_files:
          raise MRtrixError(f'Unable to find VTK file for structure "{first_sgm}" in directory "{first_path}"')
        if len(candidate_files) > 1:
          raise MRtrixError(f'Multiple candidate VTK files for structure "{first_sgm}" in directory "{first_path}"')
        vtk_filelist.append(candidate_files[0])
      if not all(vtk_file.stem.split('-')[:-1] == vtk_filelist[0].stem.split('-')[:-1] for vtk_file in vtk_filelist[1:]):
        raise MRtrixError(f'VTK files in FIRST directory "{first_path}" do not all possess same prefix')
      # Also need a template image in order to convert the VTKs
      firstseg_image = list(first_path.glob('*_all_*_firstseg.nii*'))
      if not firstseg_image:
        raise MRtrixError('Unable to find FIRST "firstseg" image (required as template for VTK conversion)')
      if len(firstseg_image) > 1:
        raise MRtrixError('Multiple candidate FIRST "firstseg" images (required as template for VTK conversion)')
      first_copy = utils.RunList('Copying FIRST .vtk files to scratch directory', len(sgm_list))
      for name, filepath in zip(sgm_list, vtk_filelist):
        first_copy.function(shutil.copyfile, filepath, f'first-{name}.vtk')
      run.command(['mrconvert', firstseg_image[0], 'first_template.mif'])

    else:
      raise MRtrixError(f'Unable to interpret input to -first option "{app.ARGS.first}" as filesystem path')

  if app.ARGS.sgm_amyg_hipp:
    lut_output_file_name = 'FreeSurfer2ACT_sgm_amyg_hipp.txt'
  else:
    lut_output_file_name = 'FreeSurfer2ACT.txt'
  lut_output_path = pathlib.Path(path.shared_data_path(), '5ttgen', lut_output_file_name)
  if not lut_output_path.is_file():
    raise MRtrixError('Could not find lookup table file for converting FreeSurfer parcellation output to tissues '
                      f'(expected location: {lut_output_path})')
  if app.ARGS.sclimbic:
    sclimbic_lut_output_path = pathlib.Path(path.shared_data_path(), '5ttgen', 'ScLimbic2ACT.txt')
    if not sclimbic_lut_output_path.is_file():
      raise MRtrixError('Could not find lookup table file for converting ScLimbic module output to tissues'
                        f' (expected location: {sclimbic_lut_output_path})')

  # Initial conversion from FreeSurfer parcellation to five principal tissue types
  index_image = 'indices.mif'
  run.command(f'labelconvert input.mif {lut_input_path} {lut_output_path} {index_image}')
  if app.ARGS.sclimbic:
    sclimbic_image = 'sclimbic.mif'
    run.command(['labelconvert', app.ARGS.sclimbic, sclimbic_lut_input_path, sclimbic_lut_output_path, sclimbic_image])

  # Generate separate images per tissue
  tissue_images = [f'tissue{index}.mif' for index in range(1, 6)]

  split_vols = utils.RunList('Splitting tissue index image into per-tissue images', 5)
  for index, filename in zip(range(1, 6), tissue_images):
    if app.ARGS.sclimbic:
      split_vols.command(f'mrcalc {sclimbic_image} 0 {index_image} -if {index} -eq {sclimbic_image} {index} -eq -add {filename}')
    else:
      split_vols.command(f'mrcalc {index_image} {index} -eq {filename}')

  # Integrating data from FIRST requires that the tissues have been separated,
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
    new_tissue_images = [f'tissue{index}_first.mif' for index in range(1, 6)]
    if pathlib.Path('first_seg.mif').is_file():
      # Hard segmentation
      mrcalc_extractions = ' '.join(f'first_seg.mif {index} -eq' for index in sgm_list.values())
      mrcalc_additions = ' '.join(['-add'] * (len(sgm_list)-1))
      run.command(f'mrcalc {mrcalc_extractions} {mrcalc_additions} 1.0 -min {new_sgm_image}')
      # Update WM and SGM tissue images
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
          insert_old_sgm = f'{tissue_images[1]} -add ' if index == 3 else ''
          first_replace.command(f'mrcalc {input_image} {insert_old_sgm} {new_sgm_image} -sub 0 -max {output_image}')
    else:
      # Soft segmentation
      first_pve = utils.RunList('Mapping FIRST segmentations to partial volumes', 2 * len(sgm_list))
      for struct in sgm_list:
        filename = f'first-{struct}.vtk'
        intername = f'real-{struct}.vtk'
        # TODO Modify first2real to read from new header transform realignment parameters and modify transform accordingly
        first_pve.command(['meshconvert', filename, intername,
                           '-transform', 'first2real', 'first_template.mif',
                           '-config', 'RealignTransform', 'false'])
        first_pve.command(['mesh2voxel', intername, index_image, f'first-{struct}.mif'])
      sgm_image_list = [f'first-{struct}.mif' for struct in sgm_list]
      run.command(['mrmath', sgm_image_list, 'sum', '-', '|',
                   'mrcalc', '-', '1.0', '-min', new_sgm_image])
      # Insert into index image
      run.command('mrcalc 1 sgm.mif -sub fs_multiplier.mif')
      first_modulate = utils.RunList('Incorporating FIRST segmentations into tissue images', 5)
      for index, input_image, output_image in zip(range(1, 6), tissue_images, new_tissue_images):
        if index == 2:
          # If tissue density in index image is zero,
          #   but SGM volume fraction is non-zero,
          #   need to boost the SGM volume fraction to 1.0
          first_modulate.command(f'mrcalc {index_image} 0 -eq {new_sgm_image} 0 -gt -mult 1 {new_sgm_image} -if {output_image}')
        else:
          insert_old_sgm = f'{tissue_images[1]} -add ' if index == 3 else ''
          first_modulate.command(f'mrcalc {input_image} {insert_old_sgm} fs_multiplier.mif -mult {output_image}')
    tissue_images = new_tissue_images

  final_command = ['mrcat', tissue_images, '-', '-axis', '3', '|']
  if not app.ARGS.nocrop:
    final_command.extend(['mrgrid', '-', 'crop', '-mask', 'indices.mif', '-', '|'])
  final_command.extend(['mrconvert', '-', 'result.mif', '-datatype', 'float32'])
  run.command(final_command)

  run.command(f'mrconvert result.mif {app.ARGS.output}',
              mrconvert_keyval=app.ARGS.input,
              force=app.FORCE_OVERWRITE,
              preserve_pipes=True)

  return 'result.mif'
