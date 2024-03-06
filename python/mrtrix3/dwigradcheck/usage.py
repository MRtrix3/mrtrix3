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

from mrtrix3 import app, _version #pylint: disable=no-name-in-module

def usage(cmdline): #pylint: disable=unused-variable
  cmdline.set_author('Robert E. Smith (robert.smith@florey.edu.au)')
  cmdline.set_synopsis('Check the orientation of the diffusion gradient table')
  cmdline.add_description('Note that the corrected gradient table can be output using the -export_grad_{mrtrix,fsl} option.')
  cmdline.add_description('Note that if the -mask command-line option is not specified, the MRtrix3 command dwi2mask will automatically be called to '
                          'derive a binary mask image to be used for streamline seeding and to constrain streamline propagation. '
                          'More information on mask derivation from DWI data can be found at the following link: \n'
                          'https://mrtrix.readthedocs.io/en/' + _version.__tag__ + '/dwi_preprocessing/masking.html')
  cmdline.add_citation('Jeurissen, B.; Leemans, A.; Sijbers, J. Automated correction of improperly rotated diffusion gradient orientations in diffusion weighted MRI. Medical Image Analysis, 2014, 18(7), 953-962')
  cmdline.add_argument('input', help='The input DWI series to be checked')
  cmdline.add_argument('-mask', metavar='image', help='Provide a mask image within which to seed & constrain tracking')
  cmdline.add_argument('-number', type=int, default=10000, help='Set the number of tracks to generate for each test')

  app.add_dwgrad_export_options(cmdline)
  app.add_dwgrad_import_options(cmdline)
