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
from mrtrix3 import app, run, image, path


LMAXES_MULTI = '4,0,0'
LMAXES_SINGLE = '4,0'

def usage(base_parser, subparsers): #pylint: disable=unused-variable
  parser = subparsers.add_parser('mtnorm', parents=[base_parser])
  parser.set_author('Robert E. Smith (robert.smith@florey.edu.au) and Arshiya Sangchooli (asangchooli@student.unimelb.edu.au)')
  parser.set_synopsis('Performs DWI bias field correction')
  parser.add_description('This script first performs response function estimation and multi-tissue CSD within a provided or derived '
                         'brain mask (with a lower lmax than the dwi2fod default, for higher speed) before calling mtnormalise and '
                         'utilizing the final estimated spatially varying intensity level used for normalisation to correct bias fields')
  parser.add_argument('input',  help='The input image series to be corrected')
  parser.add_argument('output', help='The output corrected image series')
  options = parser.add_argument_group('Options specific to the "mtnorm" algorithm')
  options.add_argument('-lmax',
                       type=str,
                       help='the maximum spherical harmonic order for the output FODs. The value is passed to '
                            'the dwi2fod command and should be provided in the format which it expects '
                            '(Default is "' + str(LMAXES_MULTI) + '" for multi-shell and "' + str(LMAXES_SINGLE) + '" for single-shell data)')


def check_output_paths(): #pylint: disable=unused-variable
  pass

def get_inputs(): #pylint: disable=unused-variable
  pass


def execute(): #pylint: disable=unused-variable
  # Determine whether we are working with single-shell or multi-shell data
  bvalues = [
      int(round(float(value)))
      for value in image.mrinfo('in.mif', 'shell_bvalues') \
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
  
  app.debug('Estimating response function using brain mask...')
  run.command('dwi2response dhollander '
              + 'in.mif '
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
              + 'in.mif'
              + ' -lmax ' + lmaxes
              + ' '
              + ' '.join(tissue.tissue_rf + ' ' + tissue.fod
                          for tissue in tissues))
  
  app.debug('correcting bias field...')
  run.command('maskfilter'
              + ' mask.mif'
              + ' erode - |'
              + ' mtnormalise -mask - -balanced'
              + ' -check_norm field.mif '
              + ' '.join(tissue.fod + ' ' + tissue.fod_norm
                          for tissue in tissues))
  app.cleanup([tissue.fod for tissue in tissues])
  app.cleanup([tissue.fod_norm for tissue in tissues])
  app.cleanup([tissue.tissue_rf for tissue in tissues])

  run.command('mrcalc '
              + 'in.mif '
              + 'field.mif '
              + '-div '
              + path.from_user(app.ARGS.output),
              force=app.FORCE_OVERWRITE)

  if app.ARGS.bias:
    run.command('mrconvert field.mif '
                + path.from_user(app.ARGS.bias),
                mrconvert_keyval=path.from_user(app.ARGS.input, False), force=app.FORCE_OVERWRITE)
