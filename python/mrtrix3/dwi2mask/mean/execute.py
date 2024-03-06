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

  run.command(('dwiextract input.mif - -shells ' + app.ARGS.shells + ' | mrmath -' \
                if app.ARGS.shells \
                else 'mrmath input.mif')
              + ' mean - -axis 3 |'
              + ' mrthreshold - - |'
              + ' maskfilter - connect -largest - |'
              + ' maskfilter - fill - |'
              + ' maskfilter - clean -scale ' + str(app.ARGS.clean_scale) + ' mask.mif')

  return 'mask.mif'
