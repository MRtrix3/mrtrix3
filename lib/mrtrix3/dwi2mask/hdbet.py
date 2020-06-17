# Copyright (c) 2008-2019 the MRtrix3 contributors.
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

from distutils.spawn import find_executable
from mrtrix3 import MRtrixError
from mrtrix3 import app, image, path, run



def usage(base_parser, subparsers): #pylint: disable=unused-variable
  parser = subparsers.add_parser('hdbet', parents=[base_parser])
  parser.set_author('Robert E. Smith (robert.smith@florey.edu.au)')
  parser.set_synopsis('Use HD-BET to derive a brain mask from the DWI mean b=0 image')
  parser.add_citation('Isensee F, Schell M, Tursunova I, Brugnara G, Bonekamp D, Neuberger U, Wick A, Schlemmer HP, Heiland S, Wick W, Bendszus M, Maier-Hein KH, Kickingereder P. Automated brain extraction of multi-sequence MRI using artificial neural networks. Hum Brain Mapp. 2019; 1–13. https://doi.org/10.1002/hbm.24750', is_external=True)
  parser.add_argument('input',  help='The input DWI series')
  parser.add_argument('output', help='The output mask image')



def execute(): #pylint: disable=unused-variable
  hdbet_cmd = find_executable('hd-bet')
  if not hdbet_cmd:
    raise MRtrixError('Unable to locate "hd-bet" executable; check installation')

  # Produce mean b=0 image
  run.command('dwiextract input.mif -bzero - | '
              'mrmath - mean - -axis 3 | '
              'mrconvert - bzero.nii -strides +1,+2,+3')

  # GPU version is not guaranteed to work;
  #   attempt CPU version if that is the case
  try:
    run.command('hd-bet -i bzero.nii')
  except run.MRtrixCmdError as e_gpu:
    app.warn('HD-BET failed when running on GPU; attempting on CPU')
    try:
      run.command('hd-bet -i bzero.nii -device cpu -mode fast -tta 0')
    except run.MRtrixCmdError as e_cpu:
      gpu_header = ('===\nGPU\n===\n')
      cpu_header = ('===\nCPU\n===\n')
      exception_stdout = gpu_header + e_gpu.stdout + '\n\n' + cpu_header + e_cpu.stdout + '\n\n'
      exception_stderr = gpu_header + e_gpu.stderr + '\n\n' + cpu_header + e_cpu.stderr + '\n\n'
      raise run.MRtrixCmdError('hd-bet', 1, exception_stdout, exception_stderr)

  strides = image.Header('input.mif').strides()[0:3]
  strides = [value + 1 - min(abs(v) for v in strides) for value in strides]

  run.command('mrconvert bzero_bet_mask.nii.gz '
              + path.from_user(app.ARGS.output)
              + ' -strides ' + ','.join(str(value) for value in strides)
              + ' -datatype bit',
              mrconvert_keyval=path.from_user(app.ARGS.input, False),
              force=app.FORCE_OVERWRITE)
