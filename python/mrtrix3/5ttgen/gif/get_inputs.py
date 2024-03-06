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

def check_gif_input(image_path):
  dim = image.Header(image_path).size()
  if len(dim) < 4:
    raise MRtrixError('Image \'' + image_path + '\' does not look like GIF segmentation (less than 4 spatial dimensions)')
  if min(dim[:4]) == 1:
    raise MRtrixError('Image \'' + image_path + '\' does not look like GIF segmentation (axis with size 1)')


def get_inputs(): #pylint: disable=unused-variable
  check_gif_input(path.from_user(app.ARGS.input, False))
  run.command('mrconvert ' + path.from_user(app.ARGS.input) + ' ' + path.to_scratch('input.mif'))
