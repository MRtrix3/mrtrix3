# Copyright (c) 2008-2021 the MRtrix3 contributors.
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

import pathlib, shlex, shutil
from mrtrix3 import CONFIG, MRtrixError
from mrtrix3 import app, image, run, utils

# Labels 1004 and 2004 are not present in the M-CRIB atlas
MCRIB_CGM = list(range(1000, 1004)) + list(range(1005,1036)) + list(range(2000, 2004)) + list(range(2005,2036))
MCRIB_SGM = [9, 11, 12, 13, 26, 48, 50, 51, 52, 58]
MCRIB_WM = [2, 41, 75, 76, 90, 192]
MCRIB_BS = [28, 60, 170]
MCRIB_CSF = [4, 14, 15, 24, 43]
MCRIB_AMYG_HIPP = [17, 18, 53, 54]
MCRIB_CEREBELLAR = [91, 93]

MCRIB_SUBJECTS = 10
MCRIB_SUBDIRS = [
  'M-CRIB T1-weighted images',
  'M-CRIB T2-weighted images',
  'Original M-CRIB parcellations'
]

def usage(base_parser, subparsers): #pylint: disable=unused-variable
  parser = subparsers.add_parser('mcrib', parents=[base_parser])
  parser.set_author('Manuel Blesa (manuel.blesa@ed.ac.uk)'
                    ' and Paola Galdi (paola.galdi@gmail.com)'
                    ' and Robert E. Smith (robert.smith@florey.edu.au)')
  parser.set_synopsis('Use ANTs commands and the M-CRIB atlas to generate the 5TT image of a neonatal subject'
                      ' based on a T1-weighted or T2-weighted image')
  parser.add_description('The M-CRIB atlas is required for this command,'
                         ' as its segmentations are used to project tissue segmentation information to the input image.'
                         ' These data can be downloaded from:'
                         ' https://osf.io/2yx8u/.'
                         ' The command can be informed of the location of these data in one of two ways:'
                         ' either explicit use of the -mcrib_path option,'
                         ' or through setting the value of MCRIBPath in an MRtrix config file.')
  parser.add_description('It is a necessary component in the production of a 5TT image'
                         ' that a brain mask be determined in some manner.'
                         ' In this algorithm,'
                         ' there are multiple possible mechanisms by which such a mask may arise.'
                         ' Use of the -mask option allows the user to provide as input a pre-calculated mask.'
                         ' Use of the -premasked option indicates that the input image'
                         ' has already been zero-filled outside of the brain.'
                         ' In the absence of either of these options,'
                         ' the command will perform an internal brain mask computation.'
                         ' If the -mask_hdbet option is specified,'
                         ' then the HD-BET method will be used for this purpose;'
                         ' otherwise, the ANTs brain extraction script will be used.')
  parser.add_citation('Blesa, M.; Galdi, P.; Cox, S.R.; Sullivan, G.; Stoye, D.Q.; Lamb, G.L.; Quigley, A.J.; Thrippleton, M.J.; Escudero, J.; Bastin, M.E.; Smith, K.M. & Boardman. J.P.'
                      ' Hierarchical complexity of the macro-scale neonatal brain.'
                      ' Cerebral Cortex, 2021, 4, 2071-2084',
                      is_external=True)
  parser.add_citation('Avants, B.; Epstein, C.; Grossman, M. & Gee, J.'
                      ' Symmetric diffeomorphic image registration with cross-correlation:'
                      ' evaluating automated labeling of elderly and neurodegenerative brain.'
                      ' Medical Image Analysis, 2008, 41, 12-26',
                      is_external=True)
  parser.add_citation('Wang, H.; Suh, J.W.; Das, S.R.; Pluta, J.B.; Craige, C. & Yushkevich, P.A.'
                      ' Multi-atlas segmentation with joint label fusion.'
                      ' IEEE Trans Pattern Anal Mach Intell., 2013, 35, 611-623.',
                      is_external=True)
  parser.add_citation('Alexander, B.; Murray, A.L.; Loh, W.Y.; Matthews, L.G.; Adamson, C.; Beare, R.; Chen, J.; Kelly, C.E.; Rees, S.; Warfield, S.K.; Anderson, P.J.; Doyle, L.W.; Spittle, A.J.; Cheong, J.L.Y; Seal, M.L. & Thompson, D.K.'
                      ' A new neonatal cortical and subcortical brain atlas:'
                      ' the Melbourne Children\'s Regional Infant Brain (m-crib) atlas.'
                      ' NeuroImage, 2017, 852, 147-841.',
                      is_external=True)
  parser.add_citation('Isensee, F.; Schell, M.; Tursunova, I.; Brugnara, G.; Bonekamp, D.; Neuberger, U.; Wick, A.; Schlemmer, H.P.; Heiland, S.; Wick, W.; Bendszus, M.; Maier-Hein, K.H.; Kickingereder, P.'
                      ' Automated brain extraction of multi-sequence MRI using artificial neural networks.'
                      ' Hum Brain Mapp. 2019; 2019; 40: 4952-4964.',
                      is_external=True,
                      condition='If -mask_hdbet option is used')
  parser.add_citation('Avants, B.; Tustison, N.J.; Wu, J.; Cook, P.A. & Gee, J.C.'
                      ' An open source multivariate framework for n-tissue segmentation with evaluation on public data.'
                      ' Neuroinformatics, 2011, 9(4), 381-400',
                      is_external=True,
                      condition='If -premasked or -mask or -mask_hdbet options are not provided')
  parser.add_argument('input',
                      type=app.Parser.ImageIn(),
                      help='The input structural image')
  parser.add_argument('modality',
                      choices=["t1w", "t2w"],
                      help='Specify the modality of the input image, either "t1w" or "t2w"')
  parser.add_argument('output',
                      type=app.Parser.ImageOut(),
                      help='The output 5TT image')
  template_options = parser.add_argument_group('Option for providing template data to the \'mcrib\' algorithm')
  template_options.add_argument('-mcrib_path',
                                type=app.Parser.DirectoryIn(),
                                help='Provide the path of the M-CRIB atlas')
  mask_options = parser.add_argument_group('Options relating to masking for the \'mcrib\' algorithm')
  mask_options.add_argument('-mask',
                            type=app.Parser.ImageIn(),
                            help='Manually provide a brain mask,'
                                 ' rather than having the script generate one automatically')
  mask_options.add_argument('-premasked',
                            action='store_true',
                            help='Indicate that brain masking has already been applied to the input image')
  mask_options.add_argument('-mask_hdbet',
                            action='store_true',
                            help='Use HD-BET to compute the required brain mask from the input image')
  #parser.flag_mutually_exclusive_options( [ 'mask', 'premasked', 'mask_hdbet' ] )
  ants_options = parser.add_argument_group('Options that affect the internal execution of ANTs within the \'mcrib\' algorithm')
  ants_options.add_argument('-ants_quick',
                            action='store_true',
                            help='Specify the use of "quick" registration parameters in ANTs:'
                                 ' results are a little bit less accurate, but much faster')
  ants_options.add_argument('-ants_parallel',
                            type=app.Parser.Int(0, 5),
                            default=0,
                            help='Control for parallel computation for antsJointLabelFusion (default 0):'
                                 ' 0 == run serially;'
                                 ' 1 == SGE qsub;'
                                 ' 2 == use PEXEC (localhost);'
                                 ' 3 == Apple XGrid;'
                                 ' 4 == PBS qsub;'
                                 ' 5 == SLURM.')
  output_options = parser.add_argument_group('Options that affect the output of the \'mcrib\' algorithm')
  output_options.add_argument('-white_stem',
                              action='store_true',
                              help='Classify the brainstem as white matter;'
                                   ' streamlines will not be permitted to terminate within this region')
  output_options.add_argument('-parcellation',
                              type=app.Parser.ImageOut(),
                              help='Additionally export the M-CRIB parcellation warped to the subject data')
  output_options.add_argument('-hard_segmentation',
                              action='store_true',
                              help='Specify the use of hard segmentation instead of the soft segmentation to generate the 5TT'
                                   ' (not recommended)')



class MCRIBImagePair:
  def __init__(self, index, imagepath, parcpath):
    self.index = index
    self.image = imagepath
    self.parc = parcpath
    # Pre-compute the filename component that will be contained
    #   within the files generated by antsJointLabelFusion
    self.prefix = pathlib.PurePath(self.image)
    while self.prefix.suffix:
      self.prefix = self.prefix.with_suffix('')
    self.prefix = self.prefix.name
  def warped_image(self):
    return f'input_parcellation_{self.prefix}_{self.index-1}_Warped.nii.gz'
  def warped_parc(self):
    return f'input_parcellation_{self.prefix}_{self.index-1}_WarpedLabels.nii.gz'


def execute(): #pylint: disable=unused-variable

  if not shutil.which('antsJointLabelFusion.sh'):
    raise MRtrixError('Could not find ANTS script antsJointLabelFusion.sh;'
                      ' please check installation')
  if not app.ARGS.mask and not app.ARGS.premasked:
    if app.ARGS.mask_hdbet:
      if not shutil.which('hd-bet'):
        raise MRtrixError('Brain masking using HD-BET requested,'
                          ' but command hd-bet not found;'
                          ' check installation')
    elif not shutil.which('antsBrainExtraction.sh'):
      raise MRtrixError('Could not find ANTS script antsBrainExtraction.sh;'
                        ' check installation')

  if app.ARGS.mcrib_path:
    mcrib_dir = app.ARGS.mcrib_path
  elif 'MCRIBPath' in CONFIG:
    mcrib_dir = pathlib.Path(CONFIG.get('MCRIBPath'))
  else:
    raise MRtrixError('The MCRIB atlas path needs to be specified'
                      ' either in the config file ("MCRIBPath")'
                      ' or with the command-line option -mcrib_path')

  if sum(map(bool, [app.ARGS.mask, app.ARGS.premasked, app.ARGS.mask_hdbet])) > 1:
    raise MRtrixError('Options -mask, -premasked and -mask_hdbet are mutually exclusive')

  image.check_3d_nonunity(app.ARGS.input)
  run.command(f'mrconvert {app.ARGS.input} input_raw.mif', preserve_pipes=True)
  original_strides = image.mrinfo('input_raw.mif', 'strides')

  # Interrogate the MCRIB input directory;
  #   determine whether the files are stored preserving the original filesystem structure,
  #   or whether all files have been collapsed into a single directory
  mcrib_has_subdir_structure = all((mcrib_dir / item).is_dir() for item in MCRIB_SUBDIRS)

  modality_subdir = (mcrib_dir / \
                     f'M-CRIB {"T1" if app.ARGS.modality == "t1w" else "T2"}-weighted images' / \
                     ('M-CRIB preprocessed T1 registered to T2'
                      if app.ARGS.modality == 't1w'
                      else 'M-CRIB preprocessed T2')) \
                    if mcrib_has_subdir_structure \
                    else mcrib_dir
  modality_filename_snippet = 'T1_registered_to_T2' if app.ARGS.modality == 't1w' else 'T2'
  parc_subdir = (mcrib_dir / \
                 'Original M-CRIB parcellations') \
                if mcrib_has_subdir_structure \
                else mcrib_dir

  # It seems that antsJointLabelFusion may not tolerate spaces in file names,
  #   even if it is invoked from subprocecss via list;
  #   if that is the case, we need to do an explicit import step
  # This doesn't apply exclusively to the presence of sub-directory structure
  #   within the M-CRIB directory;
  #   there could also be whitespace in the absolute path to the M-CRIB directory
  need_import_mcrib = shlex.quote(str(modality_subdir)) != str(modality_subdir)
  if need_import_mcrib:
    mcrib_import_cmds = utils.RunList('Importing M-CRIB data to scratch directory', 2 * MCRIB_SUBJECTS)
  mcrib_image_pairs = []
  for i in range(1, MCRIB_SUBJECTS + 1):
    input_image = modality_subdir / f'M-CRIB_P{i:02d}_{modality_filename_snippet}.nii.gz'
    input_parc = parc_subdir / f'M-CRIB_orig_P{i:02d}_parc.nii.gz'
    if input_image.is_file() and input_parc.is_file():
      if need_import_mcrib:
        imported_image = pathlib.Path(f'template_{i:02d}.nii')
        imported_parc = pathlib.Path(f'template_labels_{i:02d}.nii')
        mcrib_import_cmds.command(['mrconvert',
                                  input_image,
                                  imported_image])
        mcrib_import_cmds.command(['mrconvert',
                                  input_parc,
                                  imported_parc])
        mcrib_image_pairs.append(MCRIBImagePair(i, imported_image, imported_parc))
      else:
        mcrib_image_pairs.append(MCRIBImagePair(i, input_image, input_parc))
  if not mcrib_image_pairs:
    raise MRtrixError(f'No M-CRIB input data found at location {mcrib_dir}')
  if len(mcrib_image_pairs) != MCRIB_SUBJECTS:
    app.warn('Found required M-CRIB image data'
             f' for only {len(mcrib_image_pairs)} of expected {MCRIB_SUBJECTS} subjects')

  # Decide whether or not we're going to do any brain masking
  mask_image = pathlib.Path('mask.mif')
  if app.ARGS.mask:
    run.command(f'mrconvert {app.ARGS.mask} {mask_image} -datatype bit')
    # Check to see if the mask matches the input image
    if not image.match('input_raw.mif', mask_image):
      app.warn('Mask image does not match input image; re-gridding')
      new_mask_image = pathlib.Path('mask_regrid.mif')
      run.command(f'mrgrid {mask_image} regrid -template input_raw.mif - | '
                  f'mrthreshold - -abs 0.5 {new_mask_image} -datatype bit')
      app.cleanup(mask_image)
      mask_image = new_mask_image
  elif app.ARGS.premasked:
    run.command(f'mrthreshold input_raw.mif {mask_image} -abs 0.0 -comparison gt')
  elif app.ARGS.mask_hdbet:
    run.command('mrconvert input_raw.mif input_raw.nii -strides +1,+2,+3')
    run.command('hd-bet -i input_raw.nii -o hdbet_output.nii.gz --save_bet_mask --no_bet_image')
    run.command(f'mrconvert hdbet_output_bet.nii.gz {mask_image} -datatype bit')
  else:
    # Use antsBrainExtraction.sh script to generate a brain mask
    #   based on the parcellation of the first template participant
    run.command('mrconvert input_raw.mif input_raw.nii -strides +1,+2,+3')
    run.command(f'mrthreshold {mcrib_image_pairs[0].parc} mask_template.nii -abs 0.5')
    run.command('antsBrainExtraction.sh'
                ' -d 3'
                ' -a input_raw.nii'
                f' -e {mcrib_image_pairs[0].image}'
                ' -m mask_template.nii'
                ' -o ants_mask_output')
    run.command(f'mrconvert ants_mask_outputBrainExtractionMask.nii.gz {mask_image} -datatype bit')
    app.cleanup('ants_mask_outputBrainExtractionBrain.nii.gz')
    app.cleanup('ants_mask_outputBrainExtractionMask.nii.gz')
  # Finish branching based on brain masking

  run.command(f'mrcalc input_raw.mif {mask_image} -mult - |'
              ' mrconvert - input.nii -strides +1,+2,+3')
  app.cleanup('input_raw.mif')

  ants_options = ['-q', str(1 if app.ARGS.ants_quick else 0),
                  '-c', str(app.ARGS.ants_parallel),
                  '-k', '1',
                  '-x', 'none']
  if app.ARGS.ants_parallel == 2:
    ants_options.extend(['-j', str(2 if app.ARGS.nthreads is None else app.ARGS.nthreads)])
  app.debug(f'ANTs command-line options for JointLabelFusion: "{" ".join(ants_options)}"')

  jlf_cmd = ['antsJointLabelFusion.sh',
             '-d', '3',
             '-t', 'input.nii',
             '-p', 'posterior%04d.nii.gz']
  for mcrib_image_pair in mcrib_image_pairs:
    jlf_cmd.extend(['-g', mcrib_image_pair.image,
                    '-l', mcrib_image_pair.parc])
  jlf_cmd.extend(['-o', 'input_parcellation_'])
  jlf_cmd += ants_options
  run.command(jlf_cmd)

  app.cleanup('input.nii')
  if need_import_mcrib:
    for mcrib_image_pair in mcrib_image_pairs:
      app.cleanup(mcrib_image_pair.image)
      app.cleanup(mcrib_image_pair.parc)

  tissues = {'cGM': MCRIB_CGM + ([] if app.ARGS.sgm_amyg_hipp else MCRIB_AMYG_HIPP),
             'sGM': MCRIB_SGM + MCRIB_CEREBELLAR + (MCRIB_AMYG_HIPP if app.ARGS.sgm_amyg_hipp else []),
             'WM' : MCRIB_WM + (MCRIB_BS if app.ARGS.white_stem else []),
             'CSF': MCRIB_CSF,
             'path': ([] if app.ARGS.white_stem else MCRIB_BS)}

  for tissue, indices in tissues.items():
    if not indices:
      run.command(f'mrcalc {mask_image} 0 -mult {tissue}.mif')
    elif app.ARGS.hard_segmentation:
      mrcalc_cmd = ['mrcalc']
      for index in indices:
        mrcalc_cmd.extend(['input_parcellation_Labels.nii.gz', str(index), '-eq'])
      mrcalc_cmd.extend(['-add'] * (len(indices) - 1))
      mrcalc_cmd.append(f'{tissue}.mif')
      run.command(mrcalc_cmd)
    elif len(indices) == 1:
      run.command(f'mrconvert posterior{indices[0]:04d}.nii.gz {tissue}.mif')
    else:
      run.command(['mrmath',
                   list(f'posterior{i:04d}.nii.gz' for i in indices),
                   'sum',
                   f'{tissue}.mif'])

  if app.ARGS.hard_segmentation:
    run.command(['mrcat']
                + list(f'{tissue}.mif' for tissue in tissues)
                + ['-axis', '3', 'combined_precrop.mif'])
    app.cleanup(list(f'{tissue}.mif' for tissue in tissues))
  else:
    # Force normalization
    run.command(['mrmath']
                + list(f'{tissue}.mif' for tissue in tissues)
                + ['sum', 'tissue_sum.mif'])
    normalisation = utils.RunList('Normalising tissue density images', len(tissues))
    for tissue in tissues:
      normalisation.command(f'mrcalc tissue_sum.mif 0.0 -gt {tissue}.mif tissue_sum.mif -divide 0 -if {tissue}_norm.mif')
      app.cleanup(f'{tissue}.mif')
    app.cleanup('tissue_sum.mif')
    run.command(['mrcat']
                + list(f'{tissue}_norm.mif' for tissue in tissues)
                + ['-axis', '3', 'combined_precrop.mif'])
    app.cleanup(list(f'{tissue}_norm.mif' for tissue in tissues))

  # Preserve relative strides of spatial axes,
  #   but make volume axis contiguous
  stride_restore_option = ['-strides',
                           ','.join(('-' if int(i) < 0 else '+')
                                    + str(abs(int(i))+1)
                                    for i in original_strides.split(' '))
                           + ',+1']
  if app.ARGS.nocrop:
    run.command(['mrconvert',
                 'combined_precrop.mif',
                 'result.mif']
                + stride_restore_option)
  else:
    # Crop to reduce file size (improves caching of image data during tracking)
    run.command(['mrgrid', 'combined_precrop.mif', 'crop', '-', '-mask', mask_image, '|',
                 'mrconvert', '-', 'result.mif']
                + stride_restore_option)
  app.cleanup('combined_precrop.mif')

  run.command(f'mrconvert result.mif {app.ARGS.output}',
              mrconvert_keyval=app.ARGS.input,
              force=app.FORCE_OVERWRITE,
              preserve_pipes=True)

  if app.ARGS.parcellation:
    run.command(f'mrconvert input_parcellation_Labels.nii.gz {app.ARGS.parcellation}',
                mrconvert_keyval=app.ARGS.input,
                force=app.FORCE_OVERWRITE,
                preserve_pipes=True)
