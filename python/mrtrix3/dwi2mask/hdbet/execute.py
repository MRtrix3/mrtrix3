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

import shutil
from mrtrix3 import MRtrixError
from mrtrix3 import app, run

def execute(): #pylint: disable=unused-variable
  hdbet_cmd = shutil.which('hd-bet')
  if not hdbet_cmd:
    raise MRtrixError('Unable to locate "hd-bet" executable; check installation')

  output_image_path = 'bzero_bet_mask.nii.gz'

  # GPU version is not guaranteed to work;
  #   attempt CPU version if that is the case
  e_gpu = None #pylint: disable=unused-variable
  if not app.ARGS.nogpu:
    try:
      run.command('hd-bet -i bzero.nii')
      return output_image_path
    except run.MRtrixCmdError as e_gpu: #pylint: disable=unused-variable
      app.warn('HD-BET failed when running on GPU; attempting on CPU')
  try:
    run.command('hd-bet -i bzero.nii -device cpu -mode fast -tta 0')
    return output_image_path
  except run.MRtrixCmdError as e_cpu:
    if app.ARGS.nogpu:
      raise
    gpu_header = ('===\nGPU\n===\n')
    cpu_header = ('===\nCPU\n===\n')
    exception_stdout = gpu_header + e_gpu.stdout + '\n\n' + cpu_header + e_cpu.stdout + '\n\n'
    exception_stderr = gpu_header + e_gpu.stderr + '\n\n' + cpu_header + e_cpu.stderr + '\n\n'
    raise run.MRtrixCmdError('hd-bet', 1, exception_stdout, exception_stderr)
