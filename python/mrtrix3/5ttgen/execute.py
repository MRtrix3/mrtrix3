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

from mrtrix3 import algorithm, app, run

def execute(): #pylint: disable=unused-variable

  # Find out which algorithm the user has requested
  alg = algorithm.get(app.ARGS.algorithm)

  alg.check_output_paths()

  app.make_scratch_dir()
  alg.get_inputs()
  app.goto_scratch_dir()

  alg.execute()

  stderr = run.command('5ttcheck result.mif').stderr
  if '[WARNING]' in stderr:
    app.warn('Generated image does not perfectly conform to 5TT format:')
    for line in stderr.splitlines():
      app.warn(line)
