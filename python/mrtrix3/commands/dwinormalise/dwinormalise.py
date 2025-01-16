# Copyright (c) 2008-2025 the MRtrix3 contributors.
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

import importlib



def usage(cmdline): #pylint: disable=unused-variable
  cmdline.set_author('Robert E. Smith (robert.smith@florey.edu.au)')
  cmdline.set_synopsis('Perform various forms of intensity normalisation of DWIs')
  cmdline.add_description('This script provides access to different techniques'
                          ' for globally scaling the intensity of diffusion-weighted images.'
                          ' The different algorithms have different purposes,'
                          ' and different requirements with respect to the data'
                          ' with which they must be provided & will produce as output.'
                          ' Further information on the individual algorithms available'
                          ' can be accessed via their individual help pages;'
                          ' eg. "dwinormalise group -help".')

  # Import the command-line settings for all algorithms found in the relevant directory
  cmdline.add_subparsers()



def execute(): #pylint: disable=unused-variable
  from mrtrix3 import app #pylint: disable=no-name-in-module, import-outside-toplevel

  # Load module for the user-requested algorithm
  alg = importlib.import_module(f'.{app.ARGS.algorithm}', 'mrtrix3.commands.dwinormalise')

  # From here, the script splits depending on what algorithm is being used
  alg.execute()
