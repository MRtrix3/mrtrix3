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

import os, shutil
from mrtrix3 import CONFIG, MRtrixError
from mrtrix3 import app, path, run


ANTS_BRAIN_EXTRACTION_CMD = 'antsBrainExtraction.sh'


def usage(base_parser, subparsers): #pylint: disable=unused-variable
  parser = subparsers.add_parser('ants', parents=[base_parser])
  parser.set_author('Robert E. Smith (robert.smith@florey.edu.au)')
  parser.set_synopsis('Use ANTs Brain Extraction to derive a DWI brain mask')
  parser.add_citation('B. Avants, N.J. Tustison, G. Song, P.A. Cook, A. Klein, J.C. Jee. A reproducible evaluation of ANTs similarity metric performance in brain image registration. NeuroImage, 2011, 54, 2033-2044', is_external=True)
  parser.add_argument('input',  help='The input DWI series')
  parser.add_argument('output', help='The output mask image')
  options = parser.add_argument_group('Options specific to the "ants" algorithm')
  options.add_argument('-template', metavar='TemplateImage MaskImage', nargs=2, help='Provide the template image and corresponding mask for antsBrainExtraction.sh to use; the template image should be T2-weighted.')



def get_inputs(): #pylint: disable=unused-variable
  if app.ARGS.template:
    run.command('mrconvert ' + app.ARGS.template[0] + ' ' + path.to_scratch('template_image.nii')
                + ' -strides +1,+2,+3')
    run.command('mrconvert ' + app.ARGS.template[1] + ' ' + path.to_scratch('template_mask.nii')
                + ' -strides +1,+2,+3 -datatype uint8')
  elif all(item in CONFIG for item in ['Dwi2maskTemplateImage', 'Dwi2maskTemplateMask']):
    run.command('mrconvert ' + CONFIG['Dwi2maskTemplateImage'] + ' ' + path.to_scratch('template_image.nii')
                + ' -strides +1,+2,+3')
    run.command('mrconvert ' + CONFIG['Dwi2maskTemplateMask'] + ' ' + path.to_scratch('template_mask.nii')
                + ' -strides +1,+2,+3 -datatype uint8')
  else:
    raise MRtrixError('No template image information available from '
                      'either command-line or MRtrix configuration file(s)')



def needs_mean_bzero(): #pylint: disable=unused-variable
  return True



def execute(): #pylint: disable=unused-variable
  ants_path = os.environ.get('ANTSPATH', '')
  if not ants_path:
    raise MRtrixError('Environment variable ANTSPATH is not set; '
                      'please appropriately confirure ANTs software')
  if not shutil.which(ANTS_BRAIN_EXTRACTION_CMD):
    raise MRtrixError('Unable to find command "'
                      + ANTS_BRAIN_EXTRACTION_CMD
                      + '"; please check ANTs installation')

  run.command(ANTS_BRAIN_EXTRACTION_CMD
              + ' -d 3'
              + ' -c 3x3x2x1'
              + ' -a bzero.nii'
              + ' -e template_image.nii'
              + ' -m template_mask.nii'
              + ' -o out'
              + ('' if app.DO_CLEANUP else ' -k 1')
              + (' -z' if app.VERBOSITY >= 3 else ''))

  return 'outBrainExtractionMask.nii.gz'
