# Copyright (c) 2008-2022 the MRtrix3 contributors.
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

import math
from mrtrix3 import app, run, image

LMAXES_MULTI = '4,0,0'
LMAXES_SINGLE = '4,0'

def usage(base_parser, subparsers): #pylint: disable=unused-variable
  parser = subparsers.add_parser('mtnorm', parents=[base_parser])
  parser.set_author('Robert E. Smith (robert.smith@florey.edu.au) and Arshiya Sangchooli (asangchooli@student.unimelb.edu.au)')
  parser.set_synopsis('Derives a DWI brain mask by calculating and then thresholding a tissue density image')
  parser.add_description('This script first derives an initial brain mask using the legacy MRtrix3 dwi2mask heuristic '
                         '(based on thresholded trace images), and then performs response function estimation and multi-tissue CSD '
                         '(with a lower lmax than the dwi2fod default, for higher speed), before summing the derived tissue ODFs and '
                         'thresholding the resulting tissue density image to derive a DWI brain mask.')
  parser.add_argument('input',  help='The input DWI series')
  parser.add_argument('output', help='The output mask image')
  options = parser.add_argument_group('Options specific to the "mtnorm" algorithm')
  options.add_argument('-threshold',
                       type=float,
                       default=0.5,
                       help='the threshold on the total tissue density sum image used to derive the brain mask. the default is 0.5')
  options.add_argument('-lmax',
                       type=str,
                       help='the maximum spherical harmonic order for the output FODs. The value is passed to '
                            'the dwi2fod command and should be provided in the format which it expects '
                            '(Default is "' + str(LMAXES_MULTI) + '" for multi-shell and "' + str(LMAXES_SINGLE) + '" for single-shell data)')

def get_inputs(): #pylint: disable=unused-variable
  pass


def needs_mean_bzero(): #pylint: disable=unused-variable
  return False


def execute(): #pylint: disable=unused-variable
  # Determine whether we are working with single-shell or multi-shell data
  bvalues = [
      int(round(float(value)))
      for value in image.mrinfo('input.mif', 'shell_bvalues') \
                               .strip().split()]
  multishell = (len(bvalues) > 2)

  # Step 1: Initial DWI brain mask
  dwi_mask_image = 'dwi_mask_init.mif'
  app.debug('Performing intial DWI brain masking with the legacy MRtrix3 algorithm')
  run.command('dwi2mask legacy input.mif ' + dwi_mask_image)

  # Step 2: RF estimation / CSD / mtnormalise / mask revision
  class Tissue(object): #pylint: disable=useless-object-inheritance
    def __init__(self, name):
      self.name = name
      self.tissue_rf = 'response_' + name + '.txt'
      self.fod = 'FOD_' + name + '.mif'

  dwi_image = 'input.mif'
  tissues = [Tissue('WM'), Tissue('GM'), Tissue('CSF')]
  
  app.debug('Estimating response function using initial brain mask...')
  run.command('dwi2response dhollander '
              + dwi_image
              + ' -mask '
              + dwi_mask_image
              + ' '
              + ' '.join(tissue.tissue_rf for tissue in tissues))

  # Remove GM if we can't deal with it
  if app.ARGS.lmax:
    lmaxes = app.ARGS.lmax
  else:
    lmaxes = LMAXES_MULTI
    if not multishell:
      app.cleanup(tissues[1].tissue_rf)
      tissues = tissues[::2]
      lmaxes = LMAXES_SINGLE

  app.debug('FOD estimation with lmaxes ' + lmaxes + '...')
  run.command('dwi2fod msmt_csd '
              + dwi_image
              + ' -lmax ' + lmaxes
              + ' '
              + ' '.join(tissue.tissue_rf + ' ' + tissue.fod
                          for tissue in tissues))

  app.debug('Deriving final brain mask by thresholding tissue sum image...')
  new_dwi_mask_image = 'dwi_mask' + '.mif'

  TISSUESUM_THRESHOLD = app.ARGS.threshold / math.sqrt(4.0 * math.pi)

  run.command('mrconvert '
              + tissues[0].fod
              + ' -coord 3 0 - |'
              + ' mrmath - '
              + ' '.join(tissue.fod for tissue in tissues[1:])
              + ' sum - |'
              + ' mrthreshold - -abs '
              + str(TISSUESUM_THRESHOLD)
              + ' - |'
              + ' maskfilter - connect -largest - |'
              + ' mrcalc 1 - -sub - -datatype bit |'
              + ' maskfilter - connect -largest - |'
              + ' mrcalc 1 - -sub - -datatype bit |'
              + ' maskfilter - clean - |'
              + ' mrcalc - input_pos_mask.mif -mult '
              + new_dwi_mask_image
              + ' -datatype bit')
  app.cleanup([tissue.fod for tissue in tissues])

  return dwi_mask_image

