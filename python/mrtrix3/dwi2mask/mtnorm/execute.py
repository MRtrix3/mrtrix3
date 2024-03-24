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
