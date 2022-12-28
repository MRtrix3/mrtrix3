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
from mrtrix3 import app, run, image, matrix

DWIBIASCORRECT_MAX_ITERS = 2

# We use a threshold on the balanced tissue sum image as a replacement of dwi2mask
# within the iterative bias field correction / brain masking loop
TISSUESUM_THRESHOLD = 0.5 / math.sqrt(4.0 * math.pi)

LMAXES_MULTI = '4,0,0'
LMAXES_SINGLE = '4,0'

def usage(base_parser, subparsers): #pylint: disable=unused-variable
  parser = subparsers.add_parser('iterative', parents=[base_parser])
  parser.set_author('Robert E. Smith (robert.smith@florey.edu.au) and Arshiya Sangchooli (asangchooli@student.unimelb.edu.au)')
  parser.set_synopsis('Iteratively perform masking, RF estimation, CSD, bias field removal, and mask revision to derive a DWI brain mask')
  parser.add_description('DWI brain masking may be inaccurate due to residual bias field. This script first derives an initial '
                         'brain mask using the legacy MRtrix3 dwi2mask heuristic (based on thresholded trace images), and then '
                         'performs response function estimation, multi-tissue CSD (with a lower lmax than the dwi2fod default, for '
                         'higher speed), and mtnormalise to remove any bias field, before the DWI brain mask is recalculated. '
                         'These steps are performed iteratively, since each step may influence the others.')
  parser.add_argument('input',  help='The input DWI series')
  parser.add_argument('output', help='The output mask image')
  options = parser.add_argument_group('Options specific to the "iterative" algorithm')
  options.add_argument('-max_iters',
                       type=int,
                       default=DWIBIASCORRECT_MAX_ITERS,
                       help='the maximum number of iterations. The default is ' + str(DWIBIASCORRECT_MAX_ITERS) +
                            'iterations since for some problematic data more iterations may lead to the masks '
                            'diverging, but more iterations can lead to a better mask.')
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
  app.console('Performing intial DWI brain masking with the legacy MRtrix3 algorithm')
  run.command('dwi2mask legacy input.mif ' + dwi_mask_image)

  # Step 2: Combined RF estimation / CSD / mtnormalise / mask revision
  class Tissue(object): #pylint: disable=useless-object-inheritance
    def __init__(self, name, index):
      self.name = name
      iter_string = '_iter' + str(index)
      self.tissue_rf = 'response_' + name + iter_string + '.txt'
      self.fod_init = 'FODinit_' + name + iter_string + '.mif'
      self.fod_norm = 'FODnorm_' + name + iter_string + '.mif'

  app.console('Commencing iterative DWI bias field correction and brain masking '
              'with a maximum of ' + str(app.ARGS.max_iters) + ' iterations')

  dwi_image = 'input.mif'

  for iteration in range(0, app.ARGS.max_iters):
    iter_string = '_iter' + str(iteration+1)
    tissues = [Tissue('WM', iteration),
               Tissue('GM', iteration),
               Tissue('CSF', iteration)]
    app.console('Iteration ' + str(iteration) + ', '
                + 'estimating response function using brain mask...')
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

    app.console('Iteration ' + str(iteration) + ', '
                + 'FOD estimation with lmaxes ' + lmaxes + '...')
    run.command('dwi2fod msmt_csd '
                + dwi_image
                + ' -lmax ' + lmaxes
                + ' '
                + ' '.join(tissue.tissue_rf + ' ' + tissue.fod_init
                           for tissue in tissues))

    app.console('Iteration ' + str(iteration) + ', '
                + 'running mtnormalise...')
    field_path = 'field' + iter_string + '.mif'
    factors_path = 'factors' + iter_string + '.txt'
    run.command('maskfilter ' + dwi_mask_image + ' erode - |'
                + ' mtnormalise -mask - -balanced'
                + ' -check_norm ' + field_path
                + ' -check_factors ' + factors_path
                + ' '
                + ' '.join(tissue.fod_init + ' ' + tissue.fod_norm
                           for tissue in tissues))
    app.cleanup([tissue.fod_init for tissue in tissues])

    app.console('Iteration ' + str(iteration) + ', '
                + 'applying estimated bias field and appropiate scaling factor...')
    csf_rf = matrix.load_matrix(tissues[-1].tissue_rf)
    csf_rf_bzero_lzero = csf_rf[0][0]
    app.cleanup([tissue.tissue_rf for tissue in tissues])
    balance_factors = matrix.load_vector(factors_path)
    csf_balance_factor = balance_factors[-1]
    app.cleanup(factors_path)
    scale_multiplier = (1000.0 * math.sqrt(4.0*math.pi)) / \
                       (csf_rf_bzero_lzero / csf_balance_factor)
    new_dwi_image = 'dwi' + iter_string + '.mif'
    run.command('mrcalc ' + dwi_image + ' '
                + field_path + ' -div '
                + str(scale_multiplier) + ' -mult '
                + new_dwi_image)
    app.cleanup(field_path)
    app.cleanup(dwi_image)
    dwi_image = new_dwi_image

    app.console('Iteration ' + str(iteration) + ', '
                + 'revising brain mask...')
    new_dwi_mask_image = 'dwi_mask' + iter_string + '.mif'
    run.command('mrconvert '
                + tissues[0].fod_norm
                + ' -coord 3 0 - |'
                + ' mrmath - '
                + ' '.join(tissue.fod_norm for tissue in tissues[1:])
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
    app.cleanup([tissue.fod_norm for tissue in tissues])

    app.console('Iteration ' + str(iteration) + ', '
                + 'comparing the new mask with the previous iteration...')
    dwi_old_mask_count = image.statistics(dwi_mask_image,
                                          mask=dwi_mask_image).count
    dwi_new_mask_count = image.statistics(new_dwi_mask_image,
                                          mask=new_dwi_mask_image).count
    app.debug('Old mask: ' + str(dwi_old_mask_count))
    app.debug('New mask: ' + str(dwi_new_mask_count))
    dwi_mask_overlap_image = 'dwi_mask_overlap' + iter_string + '.mif'
    run.command('mrcalc '
                + dwi_mask_image
                + ' '
                + new_dwi_mask_image
                + ' -mult '
                + dwi_mask_overlap_image)
    app.cleanup(dwi_mask_image)
    dwi_mask_image = new_dwi_mask_image
    mask_overlap_count = image.statistics(dwi_mask_overlap_image,
                                          mask=dwi_mask_overlap_image).count
    app.debug('Mask overlap: ' + str(mask_overlap_count))
    dice_coefficient = 2.0 * mask_overlap_count / \
                       (dwi_old_mask_count + dwi_new_mask_count)
    app.console('Iteration ' + str(iteration)
                + ' dice coefficient: ' + str(dice_coefficient) + '...')
    if dice_coefficient > (1.0 - 1e-3):
      app.console('Exiting iterative loop due to mask convergence')
      break

  # Processing completed; export
  app.console('Processing completed after '
              + str(iteration + 1)
              + ' iterations, writing results to output directory')

  return dwi_mask_image
