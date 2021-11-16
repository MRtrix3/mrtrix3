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



DHCP_CGM = list(range(5, 17)) + list(range(20,40))
DHCP_SGM = [17, 18]
DHCP_WM = [19] + list(range(40, 49)) + list(range(51, 83)) + list(range(85, 88))
DHCP_CSF = [49, 50, 83]
DHCP_AMYG_HIPP = list(range(1, 5))

MCRIB_SGM = [9, 11, 12, 13, 26, 48, 50, 51, 52, 58]



def usage(base_parser, subparsers): #pylint: disable=unused-variable
  parser = subparsers.add_parser('neonatal_dhcp', parents=[base_parser])
  parser.set_author('Manuel Blesa (manuel.blesa@ed.ac.uk), Paola Galdi (paola.galdi@ed.ac.uk) and Robert E. Smith (robert.smith@florey.edu.au)')
  parser.set_synopsis('Use ANTs commands, the output of the dHCP pipeline and the M-CRIB atlas to generate the 5TT image of a neonatal subject based on a T2-weighted image')
  parser.add_description('Derivation of the 5TT image is principally based on the segmentation already performed in the dHCP pipeline. The M-CRIB atlas will only be used to introduce additional sub-cortical grey matter parcels into the tissue segmentation. For this operation to be applicable, the dHCP pipeline must have been executed with the -additional command-line flag, and the modality of the input image must be T2-weighted.')
  parser.add_citation('Blesa, M.; Galdi, P.; Cox, S.R.; Sullivan, G.; Stoye, D.Q.; Lamb, G.L.; Quigley, A.J.; Thrippleton, M.J.; Escudero, J.; Bastin, M.E.; Smith, K.M. & Boardman. J.P. Hierarchical complexity of the macro-scale neonatal brain. Cerebral Cortex, 2021, 4, 2071-2084', is_external=True)
  parser.add_citation('Avants, B.; Epstein, C.; Grossman, M. & Gee, J. Symmetric diffeomorphic image registration with cross-correlation: evaluating automated labeling of elderly and neurodegenerative brain. 2008, Medical Image Analysis, 41, 12-26', is_external=True)
  parser.add_citation('Wang, H.; Suh, J.W.; Das, S.R.; Pluta, J.B.; Craige, C. & Yushkevich, P.A. Multi-atlas segmentation with joint label fusion. IEEE Trans Pattern Anal Mach Intell., 2013, 35, 611–623.', is_external=True)
  parser.add_citation('Alexander, B.; Murray, A.L.; Loh, W.Y.; Matthews, L.G.; Adamson, C.; Beare, R.; Chen, J.; Kelly, C.E.; Rees, S.; Warfield, S.K.; Anderson, P.J.; Doyle, L.W.; Spittle, A.J.; Cheong, J.L.Y; Seal, M.L. & Thompson, D.K. A new neonatal cortical and subcortical brain atlas: the Melbourne Children’s Regional Infant Brain (m-crib) atlas. NeuroImage, 2017, 852, 147-841.', is_external=True)
  parser.add_citation('Makropoulos, A., Robinson, E.C., Schuh, A., Wright, R., Fitzgibbon, S., Bozek, J., Counsell, S.J., Steinweg, J., Vecchiato, K., Passerat-Palmbach, J., Lenz, G., Mortari, F., Tenev, T., Duff, E.P., Bastiani, M., Cordero-Grande, L., Hughes, E., Tusor, N., Tournier, J.D., Hutter, J., Price, A.N., Teixeira, R.P.A.G., Murgasova, M., Victor, S., Kelly, C., Rutherford, M.A., Smith, S.M., Edwards, A.D., Hajnal, J.V., Jenkinson, M. & Rueckert, D. The developing human connectome project: A minimal processing pipeline for neonatal cortical surface reconstruction. NeuroImage, 2018, 173, 88-112.')   
  parser.add_argument('input',  help='The input structural image')
  parser.add_argument('output', help='The output 5TT image')
  options = parser.add_argument_group('Options specific to the \'neonatal_dhcp\' algorithm')
  options.add_argument('-mask', type=str, help='Manually provide a brain mask, MANDATORY', required=True)
  options.add_argument('-dhcp_path', type=str, help='Provide the path of the tissue probability maps (posteriors/) obtained by the dHCP pipeline (see Description), MANDATORY', required=True)
  options.add_argument('-mcrib_path', type=str, help='Provide the path of the M-CRIB atlas (note: this can alternatively be specified in the MRtrix config file as "MCRIBPath")')
  options.add_argument('-parcellation', type=str, help='Additionally export the M-CRIB parcellation warped to the subject data')
  options.add_argument('-quick', action="store_true", help='Specify the use of quick registration parameters')
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
    mcrib_import.command(['mrconvert', os.path.join(mcrib_dir, 'M-CRIB_P%02d_T2.nii.gz' % i), path.to_scratch('template_%02d.nii' % i)])
    mcrib_import.command(['mrconvert', os.path.join(mcrib_dir, 'M-CRIB_orig_P%02d_parc.nii.gz' % i), path.to_scratch('template_labels_%02d.nii' % i, False)])
  if app.ARGS.dhcp_path:
    dhcp_dir = os.path.abspath(path.from_user(app.ARGS.dhcp_path, False))
    dhcp_import = utils.RunList('Importing dHCP data to scratch directory', 87)
    for i in range(1, 88):
      images = glob.glob(os.path.join(dhcp_dir, '*_drawem_seg%d.nii.gz' % i))
      if not images:
        raise MRtrixError('Unable to find dHCP image for parcel %d in location "' % i + dhcp_dir + '"')
      if len(images) > 1:
        raise MRtrixError('Multiple dHCP images found for parcel %d in location "' % i + dhcp_dir + '"')
      dhcp_import.command(['mrconvert', images[0], path.to_scratch('label_%d.mif' % i, False)])



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

  for tissue, indices in { 'cGM': DHCP_CGM + ([] if app.ARGS.sgm_amyg_hipp else DHCP_AMYG_HIPP),
                             'sGM': DHCP_SGM + (DHCP_AMYG_HIPP if app.ARGS.sgm_amyg_hipp else []),
                             'WM' : DHCP_WM,
                             'CSF': DHCP_CSF }.items():
    run.command(['mrmath', ['label_%d.mif' % i for i in indices], 'sum', 'Pmap-' + tissue + '.mif'])

  # Uncompress just once, since we need to read multiple times
  run.command('mrconvert input_parcellation_Labels.nii.gz input_parcellation_Labels.nii')

  #extract sGM from M-CRIB
  run.command('mrcalc ' + ' '.join('input_parcellation_Labels.nii ' + str(i) + ' -eq' for i in MCRIB_SGM) + ' ' + ' '.join(['-add'] * (len(MCRIB_SGM) - 1)) + ' sGM_mcrib.mif')

  #refine the WM
  run.command('mrcalc sGM_mcrib.mif 0.5 -lt Pmap-WM.mif -mult Pmap-WM-corr.mif')

  #normalize 0-1 and combine
  run.command('mrcalc Pmap-cGM.mif 0.01 -mult cGM.mif')
  run.command('mrcalc Pmap-sGM.mif 0.01 -mult sGM_mcrib.mif -add sGM.mif')
  run.command('mrcalc Pmap-WM-corr.mif 0.01 -mult WM.mif')
  run.command('mrcalc Pmap-CSF.mif 0.01 -mult CSF.mif')

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
