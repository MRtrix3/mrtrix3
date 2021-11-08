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

import math, os
from distutils.spawn import find_executable
from mrtrix3 import MRtrixError
from mrtrix3 import app, image, path, run, utils



def usage(base_parser, subparsers): #pylint: disable=unused-variable
  parser = subparsers.add_parser('neonatal', parents=[base_parser])
  parser.set_author('Manuel Blesa (manuel.blesa@ed.ac.uk), Paola Galdi (paola.galdi@ed.ac.uk) and Robert E. Smith (robert.smith@florey.edu.au)')
  parser.set_synopsis('Use ANTs commands and the M-CRIB atlas to generate the 5TT image of a neonatal subject based on a T1-weighted or T2-weighted image')
  parser.add_citation('Blesa, M.; Galdi, P.; Cox, S.R.; Sullivan, G.; Stoye, D.Q.; Lamb, G.L.; Quigley, A.J.; Thrippleton, M.J.; Escudero, J.; Bastin, M.E.; Smith, K.M. & Boardman. J.P. Hierarchical complexity of the macro-scale neonatal brain. Cerebral Cortex, 2021, 4, 2071-2084', is_external=True)
  parser.add_citation('Avants, B.; Epstein, C.; Grossman, M. & Gee, J. Symmetric diffeomorphic image registration with cross-correlation: evaluating automated labeling of elderly and neurodegenerative brain. 2008, Medical Image Analysis, 41, 12-26', is_external=True)
  parser.add_citation('Wang, H.; Suh, J.W.; Das, S.R.; Pluta, J.B.; Craige, C. & Yushkevich, P.A. Multi-atlas segmentation with joint label fusion. IEEE Trans Pattern Anal Mach Intell., 2013, 35, 611–623.', is_external=True)
  parser.add_citation('Alexander, B.; Murray, A.L.; Loh, W.Y.; Matthews, L.G.; Adamson, C.; Beare, R.; Chen, J.; Kelly, C.E.; Rees, S.; Warfield, S.K.; Anderson, P.J.; Doyle, L.W.; Spittle, A.J.; Cheong, J.L.Y; Seal, M.L. & Thompson, D.K. A new neonatal cortical and subcortical brain atlas: the Melbourne Children’s Regional Infant Brain (m-crib) atlas. NeuroImage, 2017, 852, 147-841.', is_external=True)
  parser.add_argument('input',  help='The input structural image')
  parser.add_argument('modality',  choices = ["t1", "t2"], help='Specify the modality of the input image, either "t1" or "t2"')
  parser.add_argument('output', help='The output 5TT image')
  options = parser.add_argument_group('Options specific to the \'neonatal\' algorithm')
  options.add_argument('-mask', type=str, help='Manually provide a brain mask, MANDATORY', required=True)
  options.add_argument('-parcellation', type=str, help='Name of the hard segmentation')
  options.add_argument('-mcrib_path', type=str, help='Provide the path of the M-CRIB atlas. This can be also specified in the config file as MCRIBpath')
  options.add_argument('-quick', action="store_true", help='Specify the use of quick registration parameters')
  options.add_argument('-ants_parallel', default=0, type=int, help='Control for parallel computation for antsJointLabelFusion (default 0) -- 0 == run serially,  1 == SGE qsub, 2 == use PEXEC (localhost), 3 == Apple XGrid, 4 == PBS qsub, 5 == SLURM.')



def check_output_paths(): #pylint: disable=unused-variable
  app.check_output_path(app.ARGS.output)



def get_inputs(): #pylint: disable=unused-variable
  from mrtrix3 import CONFIG, MRtrixError #pylint: disable=no-name-in-module, import-outside-toplevel
  if app.ARGS.mcrib_path:
    mpath = app.ARGS.mcrib_path
  elif 'MCRIBpath' in CONFIG:
    mpath = CONFIG.get('MCRIBpath')
  else:
    raise MRtrixError('The MCRIB atlas path needs to be specified either in the config file or with the option -mcrib_path')
  image.check_3d_nonunity(path.from_user(app.ARGS.input, False))
  run.command('mrconvert ' + path.from_user(app.ARGS.input) + ' ' + path.to_scratch('input_raw.mif'))
  run.command('mrconvert ' + path.from_user(app.ARGS.mask) + ' ' + path.to_scratch('mask.mif') + ' -datatype bit -strides -1,+2,+3')
  atlaslist = ["01", "02", "03", "04", "05", "06", "07", "08", "09", "10"]
  if app.ARGS.modality=="t1":
    for x in atlaslist:
      run.command('mrconvert ' + path.from_user(mpath) + '/M-CRIB_P' + x + '_T1_registered_to_T2.nii.gz ' + path.to_scratch('template_' + x + '.mif'))
      run.command('mrconvert ' + path.from_user(mpath) + '/M-CRIB_orig_P' + x + '_parc.nii.gz ' + path.to_scratch('template_labels_' + x + '.mif'))
  else:
    for x in atlaslist:
      run.command('mrconvert ' + path.from_user(mpath) + '/M-CRIB_P' + x + '_T2.nii.gz ' + path.to_scratch('template_' + x + '.mif'))
      run.command('mrconvert ' + path.from_user(mpath) + '/M-CRIB_orig_P' + x + '_parc.nii.gz ' + path.to_scratch('template_labels_' + x + '.mif'))



def execute(): #pylint: disable=unused-variable
  if not find_executable('antsJointLabelFusion.sh'):
    raise MRtrixError('Could not find ANTS script antsJointLabelFusion.sh; please check installation')

  run.command('mrcalc input_raw.mif mask.mif -mul input.mif')
  run.command('mrconvert input.mif input.nii')
  run.command('mrconvert mask.mif mask.nii -datatype bit')
  atlaslist = ["01", "02", "03", "04", "05", "06", "07", "08", "09", "10"]
  for x in atlaslist:
    run.command('mrthreshold template_' + x + '.mif mask_template_' + x + '.mif -abs 0.5')
    run.command('mrhistmatch -mask_input mask_template_' + x + '.mif -mask_target mask.mif scale template_' + x + '.mif input.mif template_hist_' + x + '.mif ')
    run.command('mrconvert template_hist_' + x + '.mif template_hist_' + x + '.nii')
    run.command('mrconvert mask_template_' + x + '.mif mask_template_' + x + '.nii')
    run.command('mrconvert template_labels_' + x + '.mif template_labels_' + x + '.nii')

  ants_options = '-q {} -c {} -k 1'.format(1 if app.ARGS.quick else 0, app.ARGS.ants_parallel)
  print(ants_options)
  if app.ARGS.ants_parallel == 2:
    ants_options += ' -j {}'.format(2 if app.ARGS.nthreads is None else app.ARGS.nthreads)

  

  run.command('antsJointLabelFusion.sh -d 3 -t input.nii -p posterior%04d.nii.gz -g template_hist_01.nii -l template_labels_01.nii -g template_hist_02.nii -l template_labels_02.nii -g template_hist_03.nii -l template_labels_03.nii -g template_hist_04.nii -l template_labels_04.nii -g template_hist_05.nii -l template_labels_05.nii -g template_hist_06.nii -l template_labels_06.nii -g template_hist_07.nii -l template_labels_07.nii -g template_hist_08.nii -l template_labels_08.nii -g template_hist_09.nii -l template_labels_09.nii -g template_hist_10.nii -l template_labels_10.nii -o input_parcellation_ ' + ants_options) 

  run.command('antsJointFusion -d 3 -t input.nii --verbose 1  -g input_parcellation_template_hist_01_0_Warped.nii.gz -l input_parcellation_template_hist_01_0_WarpedLabels.nii.gz -g input_parcellation_template_hist_02_1_Warped.nii.gz -l input_parcellation_template_hist_02_1_WarpedLabels.nii.gz -g input_parcellation_template_hist_03_2_Warped.nii.gz -l input_parcellation_template_hist_03_2_WarpedLabels.nii.gz -g input_parcellation_template_hist_04_3_Warped.nii.gz -l input_parcellation_template_hist_04_3_WarpedLabels.nii.gz -g input_parcellation_template_hist_05_4_Warped.nii.gz -l input_parcellation_template_hist_05_4_WarpedLabels.nii.gz -g input_parcellation_template_hist_06_5_Warped.nii.gz -l input_parcellation_template_hist_06_5_WarpedLabels.nii.gz -g input_parcellation_template_hist_07_6_Warped.nii.gz -l input_parcellation_template_hist_07_6_WarpedLabels.nii.gz -g input_parcellation_template_hist_08_7_Warped.nii.gz -l input_parcellation_template_hist_08_7_WarpedLabels.nii.gz -g input_parcellation_template_hist_09_8_Warped.nii.gz -l input_parcellation_template_hist_09_8_WarpedLabels.nii.gz -g input_parcellation_template_hist_10_9_Warped.nii.gz -l input_parcellation_template_hist_10_9_WarpedLabels.nii.gz -o [input_parcellation_Labels.nii.gz,input_parcellation_Intensity.nii.gz,posterior%04d.nii.gz]')

    # Combine the tissue images into the 5TT format within the script itself

  if app.ARGS.sgm_amyg_hipp:
    #WM
    run.command('mrmath posterior0002.nii.gz  posterior0041.nii.gz  posterior0192.nii.gz  posterior0170.nii.gz  posterior0075.nii.gz  posterior0076.nii.gz  posterior0090.nii.gz sum WM.nii.gz')
    #subcortical GM
    run.command('mrmath posterior0091.nii.gz  posterior0093.nii.gz  posterior0017.nii.gz posterior0018.nii.gz posterior0053.nii.gz posterior0054.nii.gz posterior0009.nii.gz  posterior0011.nii.gz  posterior0012.nii.gz  posterior0013.nii.gz  posterior0026.nii.gz  posterior0048.nii.gz  posterior0050.nii.gz  posterior0051.nii.gz  posterior0052.nii.gz  posterior0058.nii.gz sum sGM.nii.gz')
    #CSF
    run.command('mrmath posterior0014.nii.gz  posterior0015.nii.gz  posterior0024.nii.gz  posterior0043.nii.gz  posterior0004.nii.gz sum CSF.nii.gz')
    #GM
    run.command('mrmath posterior1000.nii.gz  posterior1001.nii.gz  posterior1002.nii.gz   posterior1003.nii.gz  posterior1005.nii.gz  posterior1006.nii.gz   posterior1007.nii.gz  posterior1008.nii.gz  posterior1009.nii.gz  posterior1010.nii.gz  posterior1011.nii.gz  posterior1012.nii.gz   posterior1013.nii.gz  posterior1014.nii.gz  posterior1015.nii.gz   posterior1016.nii.gz  posterior1017.nii.gz  posterior1018.nii.gz   posterior1019.nii.gz  posterior1020.nii.gz  posterior1021.nii.gz   posterior1022.nii.gz  posterior1023.nii.gz  posterior1024.nii.gz   posterior1025.nii.gz  posterior1026.nii.gz  posterior1027.nii.gz  posterior1028.nii.gz  posterior1029.nii.gz  posterior1030.nii.gz  posterior1031.nii.gz  posterior1031.nii.gz  posterior1032.nii.gz  posterior1033.nii.gz  posterior1034.nii.gz  posterior1035.nii.gz  posterior2000.nii.gz  posterior2001.nii.gz  posterior2002.nii.gz   posterior2003.nii.gz  posterior2005.nii.gz  posterior2006.nii.gz   posterior2007.nii.gz  posterior2008.nii.gz  posterior2009.nii.gz  posterior2010.nii.gz  posterior2011.nii.gz  posterior2012.nii.gz   posterior2013.nii.gz  posterior2014.nii.gz  posterior2015.nii.gz   posterior2016.nii.gz  posterior2017.nii.gz  posterior2018.nii.gz   posterior2019.nii.gz  posterior2020.nii.gz  posterior2021.nii.gz   posterior2022.nii.gz  posterior2023.nii.gz  posterior2024.nii.gz   posterior2025.nii.gz  posterior2026.nii.gz  posterior2027.nii.gz  posterior2028.nii.gz  posterior2029.nii.gz  posterior2030.nii.gz  posterior2031.nii.gz  posterior2031.nii.gz  posterior2032.nii.gz  posterior2033.nii.gz  posterior2034.nii.gz  posterior2035.nii.gz sum GM.nii.gz')  
  else:
    #WM
    run.command('mrmath posterior0002.nii.gz  posterior0041.nii.gz  posterior0192.nii.gz  posterior0170.nii.gz  posterior0075.nii.gz  posterior0076.nii.gz  posterior0090.nii.gz sum WM.nii.gz')
    #subcortical GM
    run.command('mrmath posterior0091.nii.gz posterior0093.nii.gz posterior0009.nii.gz  posterior0011.nii.gz  posterior0012.nii.gz  posterior0013.nii.gz  posterior0026.nii.gz  posterior0048.nii.gz  posterior0050.nii.gz  posterior0051.nii.gz  posterior0052.nii.gz  posterior0058.nii.gz sum sGM.nii.gz')
    #CSF
    run.command('mrmath posterior0014.nii.gz  posterior0015.nii.gz  posterior0024.nii.gz  posterior0043.nii.gz  posterior0004.nii.gz sum CSF.nii.gz')
    #GM
    run.command('mrmath posterior0017.nii.gz posterior0018.nii.gz posterior0053.nii.gz posterior0054.nii.gz posterior1000.nii.gz  posterior1001.nii.gz  posterior1002.nii.gz   posterior1003.nii.gz  posterior1005.nii.gz  posterior1006.nii.gz   posterior1007.nii.gz  posterior1008.nii.gz  posterior1009.nii.gz  posterior1010.nii.gz  posterior1011.nii.gz  posterior1012.nii.gz   posterior1013.nii.gz  posterior1014.nii.gz  posterior1015.nii.gz   posterior1016.nii.gz  posterior1017.nii.gz  posterior1018.nii.gz   posterior1019.nii.gz  posterior1020.nii.gz  posterior1021.nii.gz   posterior1022.nii.gz  posterior1023.nii.gz  posterior1024.nii.gz   posterior1025.nii.gz  posterior1026.nii.gz  posterior1027.nii.gz  posterior1028.nii.gz  posterior1029.nii.gz  posterior1030.nii.gz  posterior1031.nii.gz  posterior1031.nii.gz  posterior1032.nii.gz  posterior1033.nii.gz  posterior1034.nii.gz  posterior1035.nii.gz  posterior2000.nii.gz  posterior2001.nii.gz  posterior2002.nii.gz   posterior2003.nii.gz  posterior2005.nii.gz  posterior2006.nii.gz   posterior2007.nii.gz  posterior2008.nii.gz  posterior2009.nii.gz  posterior2010.nii.gz  posterior2011.nii.gz  posterior2012.nii.gz   posterior2013.nii.gz  posterior2014.nii.gz  posterior2015.nii.gz   posterior2016.nii.gz  posterior2017.nii.gz  posterior2018.nii.gz   posterior2019.nii.gz  posterior2020.nii.gz  posterior2021.nii.gz   posterior2022.nii.gz  posterior2023.nii.gz  posterior2024.nii.gz   posterior2025.nii.gz  posterior2026.nii.gz  posterior2027.nii.gz  posterior2028.nii.gz  posterior2029.nii.gz  posterior2030.nii.gz  posterior2031.nii.gz  posterior2031.nii.gz  posterior2032.nii.gz  posterior2033.nii.gz  posterior2034.nii.gz  posterior2035.nii.gz sum GM.nii.gz')


  #Force normalization  
  run.command('mrcalc mask.nii WM.nii.gz WM.nii.gz GM.nii.gz sGM.nii.gz CSF.nii.gz -add -add -add -divide 0 -if wm.mif')
  run.command('mrcalc mask.nii GM.nii.gz WM.nii.gz GM.nii.gz sGM.nii.gz CSF.nii.gz -add -add -add -divide 0 -if cgm.mif')
  run.command('mrcalc mask.nii sGM.nii.gz WM.nii.gz GM.nii.gz sGM.nii.gz CSF.nii.gz -add -add -add -divide 0 -if sgm.mif')
  run.command('mrcalc mask.nii CSF.nii.gz WM.nii.gz GM.nii.gz sGM.nii.gz CSF.nii.gz -add -add -add -divide 0 -if csf.mif')

  run.command('mrcalc csf.mif 0 -mul path.mif')

  run.command('mrcat cgm.mif sgm.mif wm.mif csf.mif path.mif - -axis 3 | mrconvert - combined_precrop.mif -strides +2,+3,+4,+1')

  # Crop to reduce file size (improves caching of image data during tracking)
  if app.ARGS.nocrop:
    run.function(os.rename, 'combined_precrop.mif', 'result.mif')
  else:
    run.command('mrmath combined_precrop.mif sum - -axis 3 | mrthreshold - - -abs 0.5 | mrgrid combined_precrop.mif crop result.mif -mask -')

  run.command('mrconvert result.mif ' + path.from_user(app.ARGS.output), mrconvert_keyval=path.from_user(app.ARGS.input, False), force=app.FORCE_OVERWRITE)
  
  if app.ARGS.parcellation:
    run.command('mrconvert input_parcellation_Labels.nii.gz ' + path.from_user(app.ARGS.parcellation), mrconvert_keyval=path.from_user(app.ARGS.input, False), force=app.FORCE_OVERWRITE)
