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

from mrtrix3 import app, _version

def usage(cmdline): #pylint: disable=unused-variable

  cmdline.set_author('Robert E. Smith (robert.smith@florey.edu.au) and Thijs Dhollander (thijs.dhollander@gmail.com)')
  cmdline.set_synopsis('Estimate response function(s) for spherical deconvolution')
  cmdline.add_description('dwi2response offers different algorithms for performing various types of response function estimation. The name of the algorithm must appear as the first argument on the command-line after \'dwi2response\'. The subsequent arguments and options depend on the particular algorithm being invoked.')
  cmdline.add_description('Each algorithm available has its own help page, including necessary references; e.g. to see the help page of the \'fa\' algorithm, type \'dwi2response fa\'.')
  cmdline.add_description('More information on response function estimation for spherical deconvolution can be found at the following link: \n'
                          'https://mrtrix.readthedocs.io/en/' + _version.__tag__ + '/constrained_spherical_deconvolution/response_function_estimation.html')
  cmdline.add_description('Note that if the -mask command-line option is not specified, the MRtrix3 command dwi2mask will automatically be called to '
                          'derive an initial voxel exclusion mask. '
                          'More information on mask derivation from DWI data can be found at: '
                          'https://mrtrix.readthedocs.io/en/' + _version.__tag__ + '/dwi_preprocessing/masking.html')

  # General options
  common_options = cmdline.add_argument_group('General dwi2response options')
  common_options.add_argument('-mask', help='Provide an initial mask for response voxel selection')
  common_options.add_argument('-voxels', help='Output an image showing the final voxel selection(s)')
  common_options.add_argument('-shells', help='The b-value(s) to use in response function estimation (comma-separated list in case of multiple b-values, b=0 must be included explicitly)')
  common_options.add_argument('-lmax', help='The maximum harmonic degree(s) for response function estimation (comma-separated list in case of multiple b-values)')
  app.add_dwgrad_import_options(cmdline)

  cmdline.add_subparsers()
