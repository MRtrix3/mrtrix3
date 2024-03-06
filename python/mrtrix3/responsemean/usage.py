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

import os, sys

def usage(cmdline): #pylint: disable=unused-variable
  cmdline.set_author('Robert E. Smith (robert.smith@florey.edu.au) and David Raffelt (david.raffelt@florey.edu.au)')
  cmdline.set_synopsis('Calculate the mean response function from a set of text files')
  cmdline.add_description('Example usage: ' + os.path.basename(sys.argv[0]) + ' input_response1.txt input_response2.txt input_response3.txt ... output_average_response.txt')
  cmdline.add_description('All response function files provided must contain the same number of unique b-values (lines), as well as the same number of coefficients per line.')
  cmdline.add_description('As long as the number of unique b-values is identical across all input files, the coefficients will be averaged. This is performed on the assumption that the actual acquired b-values are identical. This is however impossible for the ' + os.path.basename(sys.argv[0]) + ' command to determine based on the data provided; it is therefore up to the user to ensure that this requirement is satisfied.')
  cmdline.add_argument('inputs', help='The input response functions', nargs='+')
  cmdline.add_argument('output', help='The output mean response function')
  cmdline.add_argument('-legacy', action='store_true', help='Use the legacy behaviour of former command \'average_response\': average response function coefficients directly, without compensating for global magnitude differences between input files')
