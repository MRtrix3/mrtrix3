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

def execute(): #pylint: disable=unused-variable
  # check inputs and outputs
  dwi_header = image.Header(path.from_user(app.ARGS.input, False))
  if len(dwi_header.size()) != 4:
    raise MRtrixError('Input image must be a 4D image')
  gradimport = app.read_dwgrad_import_options()
  if not gradimport and 'dw_scheme' not in dwi_header.keyval():
    raise MRtrixError('No diffusion gradient table provided, and none present in image header')
  app.check_output_path(app.ARGS.output)
  # import data and gradient table
  app.make_scratch_dir()
  run.command('mrconvert ' + path.from_user(app.ARGS.input) + ' ' + path.to_scratch('in.mif') + gradimport + ' -strides 0,0,0,1')
  app.goto_scratch_dir()
  # run per-shell operations
  files = []
  for index, bvalue in enumerate(image.mrinfo('in.mif', 'shell_bvalues').split()):
    filename = 'shell-{:02d}.mif'.format(index)
    run.command('dwiextract -shells ' + bvalue + ' in.mif - | mrmath -axis 3 - ' + app.ARGS.operation + ' ' + filename)
    files.append(filename)
  if len(files) > 1:
    # concatenate to output file
    run.command('mrcat -axis 3 ' + ' '.join(files) + ' out.mif')
    run.command('mrconvert out.mif ' + path.from_user(app.ARGS.output), mrconvert_keyval=path.from_user(app.ARGS.input, False), force=app.FORCE_OVERWRITE)
  else:
    # make a 4D image with one volume
    app.warn('Only one unique b-value present in DWI data; command mrmath with -axis 3 option may be preferable')
    run.command('mrconvert ' + files[0] + ' ' + path.from_user(app.ARGS.output) + ' -axes 0,1,2,-1', mrconvert_keyval=path.from_user(app.ARGS.input, False), force=app.FORCE_OVERWRITE)
