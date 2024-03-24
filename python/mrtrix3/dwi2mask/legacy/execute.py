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

from mrtrix3 import app, run

def execute(): #pylint: disable=unused-variable

  run.command('mrcalc input.mif 0 -max input_nonneg.mif')
  run.command('dwishellmath input_nonneg.mif mean trace.mif')
  app.cleanup('input_nonneg.mif')
  run.command('mrthreshold trace.mif - -comparison gt | '
              'mrmath - max -axis 3 - | '
              'maskfilter - median - | '
              'maskfilter - connect -largest - | '
              'maskfilter - fill - | '
              'maskfilter - clean -scale ' + str(app.ARGS.clean_scale) + ' mask.mif')

  return 'mask.mif'
