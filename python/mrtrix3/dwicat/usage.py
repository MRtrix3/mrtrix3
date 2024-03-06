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

def usage(cmdline): #pylint: disable=unused-variable
  cmdline.set_author('Lena Dorfschmidt (ld548@cam.ac.uk) and Jakub Vohryzek (jakub.vohryzek@queens.ox.ac.uk) and Robert E. Smith (robert.smith@florey.edu.au)')
  cmdline.set_synopsis('Concatenating multiple DWI series accounting for differential intensity scaling')
  cmdline.add_description('This script concatenates two or more 4D DWI series, accounting for the '
                          'fact that there may be differences in intensity scaling between those series. '
                          'This intensity scaling is corrected by determining scaling factors that will '
                          'make the overall image intensities in the b=0 volumes of each series approximately '
                          'equivalent.')
  cmdline.add_argument('inputs', nargs='+', help='Multiple input diffusion MRI series')
  cmdline.add_argument('output', help='The output image series (all DWIs concatenated)')
  cmdline.add_argument('-mask', metavar='image', help='Provide a binary mask within which image intensities will be matched')
