# Copyright (c) 2008-2020 the MRtrix3 contributors.
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

import glob, math, os
from distutils.spawn import find_executable
from mrtrix3 import CONFIG, MRtrixError
from mrtrix3 import app, image, path, run, utils

# Labels 1004 and 2004 are not present in the M-CRIB atlas
MCRIB_CGM = list(range(1000, 1004)) + list(range(1005,1036)) + list(range(2000, 2004)) + list(range(2005,2036))
MCRIB_SGM = [9, 11, 12, 13, 26, 48, 50, 51, 52, 58]
MCRIB_WM = [2, 41, 75, 76, 90, 170, 192]
MCRIB_CSF = [4, 14, 15, 24, 43]
MCRIB_AMYG_HIPP = [17, 18, 53, 54]
MCRIB_CEREBELLAR = [91, 93]


def usage(base_parser, subparsers): #pylint: disable=unused-variable
  parser = subparsers.add_parser('mcrib', parents=[base_parser])
  parser.set_author('Manuel Blesa (manuel.blesa@ed.ac.uk), Paola Galdi (paola.galdi@ed.ac.uk) and Robert E. Smith (robert.smith@florey.edu.au)')
  parser.set_synopsis('Use ANTs commands and the M-CRIB atlas to generate the 5TT image of a neonatal subject based on a T1-weighted or T2-weighted image')
  parser.add_description('This command creates the 5TT file for human neonatal subjects. The M-CRIB atlas is used to idenity the different tissues.')
  parser.add_citation('Blesa, M.; Galdi, P.; Cox, S.R.; Sullivan, G.; Stoye, D.Q.; Lamb, G.L.; Quigley, A.J.; Thrippleton, M.J.; Escudero, J.; Bastin, M.E.; Smith, K.M. & Boardman. J.P. Hierarchical complexity of the macro-scale neonatal brain. Cerebral Cortex, 2021, 4, 2071-2084', is_external=True)
  parser.add_citation('Avants, B.; Epstein, C.; Grossman, M. & Gee, J. Symmetric diffeomorphic image registration with cross-correlation: evaluating automated labeling of elderly and neurodegenerative brain. 2008, Medical Image Analysis, 41, 12-26', is_external=True)
  parser.add_citation('Wang, H.; Suh, J.W.; Das, S.R.; Pluta, J.B.; Craige, C. & Yushkevich, P.A. Multi-atlas segmentation with joint label fusion. IEEE Trans Pattern Anal Mach Intell., 2013, 35, 611–623.', is_external=True)
  parser.add_citation('Alexander, B.; Murray, A.L.; Loh, W.Y.; Matthews, L.G.; Adamson, C.; Beare, R.; Chen, J.; Kelly, C.E.; Rees, S.; Warfield, S.K.; Anderson, P.J.; Doyle, L.W.; Spittle, A.J.; Cheong, J.L.Y; Seal, M.L. & Thompson, D.K. A new neonatal cortical and subcortical brain atlas: the Melbourne Children’s Regional Infant Brain (m-crib) atlas. NeuroImage, 2017, 852, 147-841.', is_external=True)
  parser.add_argument('input',  help='The input structural image')
  parser.add_argument('modality', choices=["t1w", "t2w"], help='Specify the modality of the input image, either "t1w" or "t2w"')
  parser.add_argument('output', help='The output 5TT image')
  options = parser.add_argument_group('Options specific to the \'mcrib\' algorithm')
  options.add_argument('-mask', type=str, help='Manually provide a brain mask, MANDATORY', required=True)
  options.add_argument('-mcrib_path', type=str, help='Provide the path of the M-CRIB atlas (note: this can alternatively be specified in the MRtrix config file as "MCRIBPath")')
  options.add_argument('-parcellation', type=str, help='Additionally export the M-CRIB parcellation warped to the subject data')
  options.add_argument('-quick', action="store_true", help='Specify the use of quick registration parameters')
  options.add_argument('-hard_segmentation', action="store_true", help='Specify the use of hard segmentation instead of the soft segmentation to generate the 5TT (NOTE: use of this option in this segmentation algorithm is not recommended)')
  options.add_argument('-ants_parallel', default=0, type=int, help='Control for parallel computation for antsJointLabelFusion (default 0) -- 0 == run serially,  1 == SGE qsub, 2 == use PEXEC (localhost), 3 == Apple XGrid, 4 == PBS qsub, 5 == SLURM.')



def check_output_paths(): #pylint: disable=unused-variable
  app.check_output_path(app.ARGS.output)



def get_inputs(): #pylint: disable=unused-variable
  if app.ARGS.mcrib_path:
    mcrib_dir = os.path.abspath(path.from_user(app.ARGS.mcrib_path, False))
  elif 'MCRIBPath' in CONFIG:
    mcrib_dir = CONFIG.get('MCRIBPath')
  else:
    raise MRtrixError('The MCRIB atlas path needs to be specified either in the config file ("MCRIBPath") or with the command-line option -mcrib_path')
  image.check_3d_nonunity(path.from_user(app.ARGS.input, False))
  run.command('mrconvert ' + path.from_user(app.ARGS.input) + ' ' + path.to_scratch('input_raw.mif'))
  run.command('mrconvert ' + path.from_user(app.ARGS.mask) + ' ' + path.to_scratch('mask.mif') + ' -datatype bit -strides -1,+2,+3')
  mcrib_import = utils.RunList('Importing M-CRIB data to scratch directory', 20)
  for i in range(1, 11):
    mcrib_import.command(['mrconvert', os.path.join(mcrib_dir, ('M-CRIB_P%02d_' % i) + ('T1_registered_to_T2' if app.ARGS.modality == 't1w' else 'T2') + '.nii.gz'), path.to_scratch('template_%02d.nii' % i)])
    mcrib_import.command(['mrconvert', os.path.join(mcrib_dir, 'M-CRIB_orig_P%02d_parc.nii.gz' % i), path.to_scratch('template_labels_%02d.nii' % i, False)])


def execute(): #pylint: disable=unused-variable
  if not find_executable('antsJointLabelFusion.sh'):
    raise MRtrixError('Could not find ANTS script antsJointLabelFusion.sh; please check installation')

  run.command('mrcalc input_raw.mif mask.mif -mul input.mif')
  run.command('mrconvert input.mif input.nii')
  run.command('mrconvert mask.mif mask.nii -datatype bit')

  ants_options = ' -q {} -c {} -k 1'.format(1 if app.ARGS.quick else 0, app.ARGS.ants_parallel)
  if app.ARGS.ants_parallel == 2:
    ants_options += ' -j {}'.format(2 if app.ARGS.nthreads is None else app.ARGS.nthreads)
  app.debug('ANTs command-line options for JointLabelFusion: "' + ants_options + '"')

  run.command('antsJointLabelFusion.sh -d 3 -t input.nii -p posterior%04d.nii.gz '
              + ' '.join('-g template_%02d.nii -l template_labels_%02d.nii' % (i, i) for i in range(1, 11))
              + ' -o input_parcellation_'
              + ants_options)
  
  run.command('antsJointFusion -d 3 -t input.nii --verbose 1 '
              + ' '.join('-g input_parcellation_template_%02d_%d_Warped.nii.gz -l input_parcellation_template_%02d_%d_WarpedLabels.nii.gz' % (i, i-1, i, i-1) for i in range(1, 11))
              + ' -o [input_parcellation_Labels.nii.gz,input_parcellation_Intensity.nii.gz,posterior%04d.nii.gz]')

  for tissue, indices in { 'cGM': MCRIB_CGM + ([] if app.ARGS.sgm_amyg_hipp else MCRIB_AMYG_HIPP),
                           'sGM': MCRIB_SGM + MCRIB_CEREBELLAR + (MCRIB_AMYG_HIPP if app.ARGS.sgm_amyg_hipp else []),
                           'WM' : MCRIB_WM,
                           'CSF': MCRIB_CSF }.items():

    if app.ARGS.hard_segmentation:
      run.command('mrcalc ' + ' '.join('input_parcellation_Labels.nii.gz ' + str(i) + ' -eq' for i in indices) + ' ' + ' '.join(['-add'] * (len(indices) - 1)) + ' ' + tissue + '.mif')
    else:
      run.command(['mrmath', ['posterior%04d.nii.gz' % i for i in indices], 'sum', tissue + '.mif'])


  #Force normalization
  run.command('mrmath WM.mif cGM.mif sGM.mif CSF.mif sum tissue_sum.mif')
  run.command('mrcalc mask.nii cGM.mif tissue_sum.mif -divide 0 -if cgm_norm.mif')
  run.command('mrcalc mask.nii sGM.mif tissue_sum.mif -divide 0 -if sgm_norm.mif')
  run.command('mrcalc mask.nii WM.mif tissue_sum.mif -divide 0 -if wm_norm.mif')
  run.command('mrcalc mask.nii CSF.mif tissue_sum.mif -divide 0 -if csf_norm.mif')

  run.command('mrcalc csf_norm.mif 0 -mul path.mif')

  run.command('mrcat cgm_norm.mif sgm_norm.mif wm_norm.mif csf_norm.mif path.mif - -axis 3 | mrconvert - combined_precrop.mif -strides +2,+3,+4,+1')

  # Crop to reduce file size (improves caching of image data during tracking)
  if app.ARGS.nocrop:
    run.function(os.rename, 'combined_precrop.mif', 'result.mif')
  else:
    run.command('mrmath combined_precrop.mif sum - -axis 3 | mrthreshold - - -abs 0.5 | mrgrid combined_precrop.mif crop result.mif -mask -')

  run.command('mrconvert result.mif ' + path.from_user(app.ARGS.output), mrconvert_keyval=path.from_user(app.ARGS.input, False), force=app.FORCE_OVERWRITE)
  
  if app.ARGS.parcellation:
    run.command('mrconvert input_parcellation_Labels.nii.gz ' + path.from_user(app.ARGS.parcellation), mrconvert_keyval=path.from_user(app.ARGS.input, False), force=app.FORCE_OVERWRITE)
