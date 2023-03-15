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
from mrtrix3 import CONFIG, MRtrixError
from mrtrix3 import app, run, image, path, matrix,  _version

DEFAULT_TARGET_INTENSITY=1000

LMAXES_MULTI = '4,0,0'
LMAXES_SINGLE = '4,0'


def usage(base_parser, subparsers): #pylint: disable=unused-variable
  parser = subparsers.add_parser('mtnorm', parents=[base_parser])
  parser.set_author('Robert E. Smith (robert.smith@florey.edu.au) and Arshiya Sangchooli (asangchooli@student.unimelb.edu.au)')
  parser.set_synopsis('Performs a global DWI intensity normalisation, normalising mean b=0 CSF signal intensity')
  parser.add_description('This script first performs response function estimation and multi-tissue CSD within a provided brain mask'
                         '(with a lower lmax than the dwi2fod default, for higher speed) before using the global scaling factor derived '
                         'by mtnormalise to scale the intensity of the DWI series such that the mean b=0 CSF signal intensity is '
                         'approximately close to some reference value (by default ' + str(DEFAULT_TARGET_INTENSITY) + ').'
                         'Note that if the -mask command-line option is not specified, the MRtrix3 command dwi2mask will automatically be '
                         'called to derive a mask that will be passed to the relevant bias field estimation command. '
                         'More information on mask derivation from DWI data can be found at the following link: \n'
                         'https://mrtrix.readthedocs.io/en/' + _version.__tag__ + '/dwi_preprocessing/masking.html')
  parser.add_argument('input_dwi',  help='The input DWI series')
  parser.add_argument('output', help='The normalised DWI series')
  options = parser.add_argument_group('Options specific to the "mtnorm" algorithm')
  options.add_argument('-target', type=float, default=DEFAULT_TARGET_INTENSITY,
                       help='the threshold on the balanced tissue sum image used to derive the brain mask. the default is 0.5')
  options.add_argument('-lmax', type=str,
                       help='the maximum spherical harmonic order for the output FODs. The value is passed to '
                            'the dwi2fod command and should be provided in the format which it expects '
                            '(Default is "' + str(LMAXES_MULTI) + '" for multi-shell and "' + str(LMAXES_SINGLE) + '" for single-shell data)')
  options.add_argument('-mask', metavar='image', help='Manually provide a mask image for normalisation')
  app.add_dwgrad_import_options(parser)

def check_output_paths(): #pylint: disable=unused-variable
  app.check_output_path(app.ARGS.output)


def execute(): #pylint: disable=unused-variable
  # Get input data into the scratch directory
  grad_option = ''
  if app.ARGS.grad:
    grad_option = ' -grad ' + path.from_user(app.ARGS.grad)
  elif app.ARGS.fslgrad:
    grad_option = ' -fslgrad ' + path.from_user(app.ARGS.fslgrad[0]) + ' ' + path.from_user(app.ARGS.fslgrad[1])

  app.make_scratch_dir()
  app.goto_scratch_dir()

  run.command('mrconvert '
              + path.from_user(app.ARGS.input_dwi)
              + ' '
              + path.to_scratch('input.mif')
              + grad_option)

  # Get mask into scratch directory
  if app.ARGS.mask:
    run.command('mrconvert ' + path.from_user(app.ARGS.mask) + ' ' + path.to_scratch('mask.mif'))
    if not image.match('input.mif', 'mask.mif', up_to_dim=3):
      raise MRtrixError('Provided mask image does not match input DWI')
  else:
    app.debug('Deriving brain mask...')
    run.command('dwi2mask ' + CONFIG.get('Dwi2maskAlgorithm', 'legacy') + ' input.mif mask.mif')

  # Determine whether we are working with single-shell or multi-shell data
  bvalues = [
      int(round(float(value)))
      for value in image.mrinfo('input.mif', 'shell_bvalues') \
                               .strip().split()]
  multishell = (len(bvalues) > 2)

  # RF estimation and multi-tissue CSD
  class Tissue(object): #pylint: disable=useless-object-inheritance
    def __init__(self, name):
      self.name = name
      self.tissue_rf = 'response_' + name + '.txt'
      self.fod = 'FOD_' + name + '.mif'
      self.fod_norm = 'FODnorm_' + name + '.mif'

  tissues = [Tissue('WM'), Tissue('GM'), Tissue('CSF')]

  app.debug('Estimating response function using initial brain mask...')
  run.command('dwi2response dhollander '
              + 'input.mif '
              + '-mask mask.mif '
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
              + 'input.mif'
              + ' -lmax ' + lmaxes
              + ' '
              + ' '.join(tissue.tissue_rf + ' ' + tissue.fod
                          for tissue in tissues))

  # Normalisation in brain mask
  run.command('maskfilter'
              + ' mask.mif'
              + ' erode - |'
              + ' mtnormalise -mask - -balanced'
              + ' -check_factors factors.txt '
              + ' '.join(tissue.fod + ' ' + tissue.fod_norm
                          for tissue in tissues))
  app.cleanup([tissue.fod for tissue in tissues])
  app.cleanup([tissue.fod_norm for tissue in tissues])

  app.debug('applying appropiate scaling factor to DWI series...')
  csf_rf = matrix.load_matrix(tissues[-1].tissue_rf)
  csf_rf_bzero_lzero = csf_rf[0][0]
  app.cleanup([tissue.tissue_rf for tissue in tissues])
  balance_factors = matrix.load_vector('factors.txt')
  csf_balance_factor = balance_factors[-1]
  app.cleanup('factors.txt')
  scale_multiplier = (1000.0 * math.sqrt(4.0*math.pi)) / (csf_rf_bzero_lzero / csf_balance_factor)
  run.command('mrcalc '
              + 'input.mif '
              + str(scale_multiplier)
              + ' -mult '
              + path.from_user(app.ARGS.output),
              force=app.FORCE_OVERWRITE)
