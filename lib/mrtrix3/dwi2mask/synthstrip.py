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

import shutil
import argparse
import os
import warnings
from mrtrix3 import MRtrixError
from mrtrix3 import app, run


DEFAULT_CLEAN_SCALE = 2
SYNTHSTRIP='mri_synthstrip'


def usage(base_parser, subparsers): #pylint: disable=unused-variable
  parser = subparsers.add_parser('synthstrip', parents=[base_parser])
  parser.set_author('Ruobing Chen (chrc@student.unimelb.edu.au)')
  parser.set_synopsis('Use the FreeSurfer Synthstrip method')
  parser.add_argument('input',  help='The input DWI series')
  parser.add_argument('output', help='The output mask image')
  options=parser.add_argument_group('Options specific to the \' Synthstrip \'algorithm')
  options.add_argument('-s', action='store_true', default=False, help='The output stripped image')
  options.add_argument('-h', action='store_true',default=False, help='The help messeage of Synthstrip algorithm')
  options.add_argument('-g', action='store_true',default=False, help='Use the GPU')
  options.add_argument('-mo',action='store_true',default=False,help='Alternative model weights')
  options.add_argument('-nocsf',action='store_true', default=False, help='Compute the immediate boundary of brain matter excluding surrounding CSF')
  options.add_argument('-b', type=int,help='Control the boundary distance from the brain')
  parser.add_argument('-clean_scale',
                      type=int,
                      default=DEFAULT_CLEAN_SCALE,
                      help='the maximum scale used to cut bridges. A certain maximum scale cuts '
                           'bridges up to a width (in voxels) of 2x the provided scale. Setting '
                           'this to 0 disables the mask cleaning step. (Default: ' + str(DEFAULT_CLEAN_SCALE) + ')')
  


def get_inputs(): #pylint: disable=unused-variable
  pass



def needs_mean_bzero(): #pylint: disable=unused-variable
  return True



def execute(): #pylint: disable=unused-variable
  
  run.command('dwiextract input.mif - -bzero | mrmath - mean mean_bzero.mif -axis 3')
  
  run.command('mrconvert mean_bzero.mif 3dbzero.mif -axes 0,1,2')
  app.cleanup('mean_bzero.mif')
  run.command('mrconvert 3dbzero.mif 3dbzero.nii')
  app.cleanup('3dbzero.mif')
  
  
  synthstrip_cmd = shutil.which("mri_synthstrip")
  if not synthstrip_cmd:
    raise MRtrixError('Unable to locate "Synthstrip" executable; please check installation')
  
  cmd_string =SYNTHSTRIP + ' -i' +' 3dbzero.nii' + ' -m' +' synthstrip_mask.nii'
  output_file='synthstrip_mask.mif'
  #current_path=os.path.abspath('input.mif')
  current_path=os.path.abspath(__file__)
  father_path=os.path.abspath(os.path.dirname(current_path))


  if app.ARGS.h:
    cmd_string=SYNTHSTRIP +' -h'
    warnings.warn('Displaying help message will not produce any desired files,the output of this command only produce the original input file with desired output file name, if need files, please rerun the command without -h syntax')
  if app.ARGS.s:
    cmd_string+=' -o '+ father_path+'/stripped.nii'
    warnings.warn('The stripped file will be saved in mrtrix3/lib/mrtrix3/dwi2mask')
  if app.ARGS.g:
    cmd_string+=' -g'
    
  if app.ARGS.nocsf:
    cmd_string+=' --no-csf'

  if app.ARGS.b is not None:
    cmd_string += ' -b' + ' '+str(app.ARGS.b)

  if app.ARGS.mo:
    cmd_string+= ' --model' + father_path + ' -model.nii'

  #try:
  #  #run.command( 'mri_synthstip -i 3dbzero.nii -o' + str(app.ARGS.o)+ '-m' + str(app.ARGS.m)+'-no-csf'+str(app.ARGS.no-csf)+'-b'+str(app.ARGS.b))
  #  run.command( 'mri_synthstrip -i 3dbzero.nii -o stripped.nii -m synthstrip_mask.nii')
  #except run.MRtrixError:
  #  app.warn('The input image may have more than 1 frames.')
  if app.ARGS.h:
    run.command(cmd_string)
    return 'input.mif'
  else:
    run.command(cmd_string)
    run.command('mrconvert synthstrip_mask.nii synthstrip_mask.mif -datatype bit')
    app.cleanup('3dbzero.nii')
  
    app.cleanup('synthstrip_mask.nii')
  
    return output_file
