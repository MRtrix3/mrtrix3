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
import os
import warnings
from mrtrix3 import MRtrixError
from mrtrix3 import app, run



SYNTHSTRIP_CMD='mri_synthstrip'


def usage(base_parser, subparsers): #pylint: disable=unused-variable
  parser = subparsers.add_parser('synthstrip', parents=[base_parser])
  parser.set_author('Ruobing Chen (chrc@student.unimelb.edu.au)')
  parser.set_synopsis('Use the Synthstrip heuristic (based on skull-stripping method)')
  parser.add_argument('input',  help='The input DWI series')
  parser.add_argument('output', help='The output mask image')
  options=parser.add_argument_group('Options specific to the \' Synthstrip \'algorithm')
  options.add_argument('-s', action='store_true', default=False, help='The output stripped image')
  options.add_argument('-h', action='store_true',default=False, help='The help messeage of Synthstrip algorithm')
  options.add_argument('-g', action='store_true',default=False, help='Use the GPU')
  options.add_argument('-mo',action='store_true',default=False,help='Alternative model weights')
  options.add_argument('-nocsf',action='store_true', default=False, help='Compute the immediate boundary of brain matter excluding surrounding CSF')
  options.add_argument('-b', type=int,help='Control the boundary distance from the brain')



def get_inputs(): #pylint: disable=unused-variable
  pass



def needs_mean_bzero(): #pylint: disable=unused-variable
  return True



def execute(): #pylint: disable=unused-variable



  synthstrip_cmd = shutil.which(SYNTHSTRIP_CMD)
  if not synthstrip_cmd:
    raise MRtrixError('Unable to locate "Synthstrip" executable; please check installation')

  cmd_string =SYNTHSTRIP_CMD+ ' -i bzero.nii -m synthstrip_mask.nii'
  output_file='synthstrip_mask.mif'
  #current_path=os.path.abspath('input.mif')
  current_path=os.path.abspath(__file__)
  father_path=os.path.abspath(os.path.dirname(current_path))


  if app.ARGS.h:
    cmd_string=SYNTHSTRIP_CMD +' -h'
    output_file='input.mif'
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

  run.command(cmd_string)
  run.command('mrconvert synthstrip_mask.nii synthstrip_mask.mif -datatype bit')
  app.cleanup('synthstrip_mask.nii')
  return output_file

  #if app.ARGS.h:
  #  run.command(cmd_string)
  #  return 'input.mif'
  #else:
  #  run.command(cmd_string)
  #  run.command('mrconvert synthstrip_mask.nii synthstrip_mask.mif -datatype bit')


#    app.cleanup('synthstrip_mask.nii')

#    return output_file
