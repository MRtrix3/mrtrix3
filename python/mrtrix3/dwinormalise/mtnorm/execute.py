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
from mrtrix3 import CONFIG, MRtrixError
from mrtrix3 import app, image, matrix, path, run

from . import LMAXES_MULTI, LMAXES_SINGLE

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
  if app.ARGS.reference <= 0.0:
    raise MRtrixError('Reference intensity must be positive')

  grad_option = app.read_dwgrad_import_options()

  # Get input data into the scratch directory
  app.make_scratch_dir()
  run.command('mrconvert '
              + path.from_user(app.ARGS.input)
              + ' '
              + path.to_scratch('input.mif')
              + grad_option)
  if app.ARGS.mask:
    run.command('mrconvert ' + path.from_user(app.ARGS.mask) + ' ' + path.to_scratch('mask.mif') + ' -datatype bit')
  app.goto_scratch_dir()

  # Make sure we have a valid mask available
  if app.ARGS.mask:
    if not image.match('input.mif', 'mask.mif', up_to_dim=3):
      raise MRtrixError('Provided mask image does not match input DWI')
  else:
    run.command('dwi2mask ' + CONFIG['Dwi2maskAlgorithm'] + ' input.mif mask.mif')

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

  # RF estimation and multi-tissue CSD
  class Tissue(object): #pylint: disable=useless-object-inheritance
    def __init__(self, name):
      self.name = name
      self.tissue_rf = 'response_' + name + '.txt'
      self.fod = 'FOD_' + name + '.mif'
      self.fod_norm = 'FODnorm_' + name + '.mif'

  tissues = [Tissue('WM'), Tissue('GM'), Tissue('CSF')]

  run.command('dwi2response dhollander input.mif -mask mask.mif '
              + ' '.join(tissue.tissue_rf for tissue in tissues))

  # Immediately remove GM if we can't deal with it
  if not multishell:
    app.cleanup(tissues[1].tissue_rf)
    tissues = tissues[::2]

  run.command('dwi2fod msmt_csd input.mif'
              + ' -lmax ' + ','.join(str(item) for item in lmax)
              + ' '
              + ' '.join(tissue.tissue_rf + ' ' + tissue.fod
                         for tissue in tissues))

  # Normalisation in brain mask
  run.command('maskfilter mask.mif erode - |'
              + ' mtnormalise -mask - -balanced'
              + ' -check_factors factors.txt '
              + ' '.join(tissue.fod + ' ' + tissue.fod_norm
                         for tissue in tissues))
  app.cleanup([tissue.fod for tissue in tissues])
  app.cleanup([tissue.fod_norm for tissue in tissues])

  csf_rf = matrix.load_matrix(tissues[-1].tissue_rf)
  app.cleanup([tissue.tissue_rf for tissue in tissues])
  csf_rf_bzero_lzero = csf_rf[0][0]
  balance_factors = matrix.load_vector('factors.txt')
  app.cleanup('factors.txt')
  csf_balance_factor = balance_factors[-1]
  scale_multiplier = (app.ARGS.reference * math.sqrt(4.0*math.pi)) / (csf_rf_bzero_lzero / csf_balance_factor)

  run.command('mrcalc input.mif ' + str(scale_multiplier) + ' -mult - | '
              + 'mrconvert - ' + path.from_user(app.ARGS.output),
              mrconvert_keyval=path.from_user(app.ARGS.input, False),
              force=app.FORCE_OVERWRITE)

  if app.ARGS.scale:
    matrix.save_vector(path.from_user(app.ARGS.scale, False), [scale_multiplier])
