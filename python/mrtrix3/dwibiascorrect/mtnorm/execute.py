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

  # Determine whether we are working with single-shell or multi-shell data
  bvalues = [
      int(round(float(value)))
      for value in image.mrinfo('in.mif', 'shell_bvalues') \
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

  run.command('dwi2response dhollander in.mif'
              + (' -mask mask.mif' if app.ARGS.mask else '')
              + ' '
              + ' '.join(tissue.tissue_rf for tissue in tissues))

  # Immediately remove GM if we can't deal with it
  if not multishell:
    app.cleanup(tissues[1].tissue_rf)
    tissues = tissues[::2]

  run.command('dwi2fod msmt_csd in.mif'
              + ' -lmax ' + ','.join(str(item) for item in lmax)
              + ' '
              + ' '.join(tissue.tissue_rf + ' ' + tissue.fod
                         for tissue in tissues))

  run.command('maskfilter mask.mif erode - | '
              + 'mtnormalise -mask - -balanced'
              + ' -check_norm field.mif '
              + ' '.join(tissue.fod + ' ' + tissue.fod_norm
                          for tissue in tissues))
  app.cleanup([tissue.fod for tissue in tissues])
  app.cleanup([tissue.fod_norm for tissue in tissues])
  app.cleanup([tissue.tissue_rf for tissue in tissues])

  run.command('mrcalc in.mif field.mif -div - | '
              'mrconvert - '+ path.from_user(app.ARGS.output),
              mrconvert_keyval=path.from_user(app.ARGS.input, False),
              force=app.FORCE_OVERWRITE)

  if app.ARGS.bias:
    run.command('mrconvert field.mif ' + path.from_user(app.ARGS.bias),
                mrconvert_keyval=path.from_user(app.ARGS.input, False),
                force=app.FORCE_OVERWRITE)
