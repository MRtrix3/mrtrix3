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

import math
from mrtrix3 import MRtrixError
from mrtrix3 import app, image, path, run


LMAXES_MULTI = [4, 0, 0]
LMAXES_SINGLE = [4, 0]
THRESHOLD_DEFAULT = 0.5



def usage(base_parser, subparsers): #pylint: disable=unused-variable
  parser = subparsers.add_parser('mtnorm', parents=[base_parser])
  parser.set_author('Robert E. Smith (robert.smith@florey.edu.au) and Arshiya Sangchooli (asangchooli@student.unimelb.edu.au)')
  parser.set_synopsis('Derives a DWI brain mask by calculating and then thresholding a sum-of-tissue-densities image')
  parser.add_description('This script attempts to identify brain voxels based on the total density of macroscopic '
                         'tissues as estimated through multi-tissue deconvolution. Following response function '
                         'estimation and multi-tissue deconvolution, the sum of tissue densities is thresholded at a '
                         'fixed value (default is ' + str(THRESHOLD_DEFAULT) + '), and subsequent mask image cleaning '
                         'operations are performed.')
  parser.add_description('The operation of this script is a subset of that performed by the script "dwibiasnormmask". '
                         'Many users may find that comprehensive solution preferable; this dwi2mask algorithm is nevertheless '
                         'provided to demonstrate specifically the mask estimation portion of that command.')
  parser.add_description('The ODFs estimated within this optimisation procedure are by default of lower maximal spherical harmonic '
                         'degree than what would be advised for analysis. This is done for computational efficiency. This '
                         'behaviour can be modified through the -lmax command-line option.')
  parser.add_citation('Jeurissen, B; Tournier, J-D; Dhollander, T; Connelly, A & Sijbers, J. '
                      'Multi-tissue constrained spherical deconvolution for improved analysis of multi-shell diffusion MRI data. '
                      'NeuroImage, 2014, 103, 411-426')
  parser.add_citation('Dhollander, T.; Raffelt, D. & Connelly, A. '
                      'Unsupervised 3-tissue response function estimation from single-shell or multi-shell diffusion MR data without a co-registered T1 image. '
                      'ISMRM Workshop on Breaking the Barriers of Diffusion MRI, 2016, 5')
  parser.add_argument('input', help='The input DWI series')
  parser.add_argument('output', help='The output mask image')
  options = parser.add_argument_group('Options specific to the "mtnorm" algorithm')
  options.add_argument('-init_mask',
                       metavar='image',
                       help='Provide an initial brain mask, which will constrain the response function estimation '
                            '(if omitted, the default dwi2mask algorithm will be used)')
  options.add_argument('-lmax',
                       metavar='values',
                       help='The maximum spherical harmonic degree for the estimated FODs (see Description); '
                            'defaults are "' + ','.join(str(item) for item in LMAXES_MULTI) + '" for multi-shell and "' + ','.join(str(item) for item in LMAXES_SINGLE) + '" for single-shell data)')
  options.add_argument('-threshold',
                       type=float,
                       metavar='value',
                       default=THRESHOLD_DEFAULT,
                       help='the threshold on the total tissue density sum image used to derive the brain mask; default is ' + str(THRESHOLD_DEFAULT))
  options.add_argument('-tissuesum', metavar='image', help='Export the tissue sum image that was used to generate the mask')



def get_inputs(): #pylint: disable=unused-variable
  if app.ARGS.init_mask:
    run.command(['mrconvert', path.from_user(app.ARGS.init_mask, False), path.to_scratch('init_mask.mif', False), '-datatype', 'bit'])


def needs_mean_bzero(): #pylint: disable=unused-variable
  return False


def execute(): #pylint: disable=unused-variable

  # Verify user inputs
  lmax = None
  if app.ARGS.lmax:
    try:
      lmax = [int(i) for i in app.ARGS.lmax.split(',')]
    except ValueError as exc:
      raise MRtrixError('Values provided to -lmax option must be a comma-separated list of integers') from exc
    if any(value < 0 or value % 2 for value in lmax):
      raise MRtrixError('lmax values must be non-negative even integers')
    if len(lmax) not in [2, 3]:
      raise MRtrixError('Length of lmax vector expected to be either 2 or 3')
  if app.ARGS.threshold <= 0.0 or app.ARGS.threshold >= 1.0:
    raise MRtrixError('Tissue density sum threshold must lie within the range (0.0, 1.0)')

  # Determine whether we are working with single-shell or multi-shell data
  bvalues = [
      int(round(float(value)))
      for value in image.mrinfo('input.mif', 'shell_bvalues') \
                               .strip().split()]
  multishell = (len(bvalues) > 2)
  if lmax is None:
    lmax = LMAXES_MULTI if multishell else LMAXES_SINGLE
  elif len(lmax) == 3 and not multishell:
    raise MRtrixError('User specified 3 lmax values for three-tissue decomposition, but input DWI is not multi-shell')

  class Tissue(object): #pylint: disable=useless-object-inheritance
    def __init__(self, name):
      self.name = name
      self.tissue_rf = 'response_' + name + '.txt'
      self.fod = 'FOD_' + name + '.mif'

  dwi_image = 'input.mif'
  tissues = [Tissue('WM'), Tissue('GM'), Tissue('CSF')]

  run.command('dwi2response dhollander '
              + dwi_image
              + (' -mask init_mask.mif' if app.ARGS.init_mask else '')
              + ' '
              + ' '.join(tissue.tissue_rf for tissue in tissues))

  # Immediately remove GM if we can't deal with it
  if not multishell:
    app.cleanup(tissues[1].tissue_rf)
    tissues = tissues[::2]

  run.command('dwi2fod msmt_csd '
              + dwi_image
              + ' -lmax ' + ','.join(str(item) for item in lmax)
              + ' '
              + ' '.join(tissue.tissue_rf + ' ' + tissue.fod
                         for tissue in tissues))

  tissue_sum_image = 'tissuesum.mif'
  run.command('mrconvert '
                + tissues[0].fod
                + ' -coord 3 0 - |'
                + ' mrmath - '
                + ' '.join(tissue.fod for tissue in tissues[1:])
                + ' sum - | '
                + 'mrcalc - ' + str(math.sqrt(4.0 * math.pi)) + ' -mult '
                + tissue_sum_image)

  mask_image = 'mask.mif'
  run.command('mrthreshold '
              + tissue_sum_image
              + ' -abs '
              + str(app.ARGS.threshold)
              + ' - |'
              + ' maskfilter - connect -largest - |'
              + ' mrcalc 1 - -sub - -datatype bit |'
              + ' maskfilter - connect -largest - |'
              + ' mrcalc 1 - -sub - -datatype bit |'
              + ' maskfilter - clean '
              + mask_image)
  app.cleanup([tissue.fod for tissue in tissues])

  if app.ARGS.tissuesum:
    run.command(['mrconvert', tissue_sum_image, path.from_user(app.ARGS.tissuesum, False)],
                mrconvert_keyval=path.from_user(app.ARGS.input, False),
                force=app.FORCE_OVERWRITE)

  return mask_image
