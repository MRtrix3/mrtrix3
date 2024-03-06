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

from mrtrix3 import algorithm, app, _version

def usage(cmdline): #pylint: disable=unused-variable

  cmdline.set_author('Robert E. Smith (robert.smith@florey.edu.au) and Warda Syeda (wtsyeda@unimelb.edu.au)')
  cmdline.set_synopsis('Generate a binary mask from DWI data')
  cmdline.add_description('This script serves as an interface for many different algorithms that generate a binary mask from DWI data in different ways. '
                          'Each algorithm available has its own help page, including necessary references; e.g. to see the help page of the \'fslbet\' algorithm, type \'dwi2mask fslbet\'.')
  cmdline.add_description('More information on mask derivation from DWI data can be found at the following link: \n'
                          'https://mrtrix.readthedocs.io/en/' + _version.__tag__ + '/dwi_preprocessing/masking.html')

  # General options
  #common_options = cmdline.add_argument_group('General dwi2mask options')
  app.add_dwgrad_import_options(cmdline)

  # Import the command-line settings for all algorithms found in the relevant directory
  algorithm.usage(cmdline)
