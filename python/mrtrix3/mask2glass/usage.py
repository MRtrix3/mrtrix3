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
  cmdline.set_author('Remika Mito (remika.mito@florey.edu.au) and Robert E. Smith (robert.smith@florey.edu.au)')
  cmdline.set_synopsis('Create a glass brain from mask input')
  cmdline.add_description('The output of this command is a glass brain image, which can be viewed '
                          'using the volume render option in mrview, and used for visualisation purposes to view results in 3D.')
  cmdline.add_description('While the name of this script indicates that a binary mask image is required as input, it can '
                          'also operate on a floating-point image. One way in which this can be exploited is to compute the mean '
                          'of all subject masks within template space, in which case this script will produce a smoother result '
                          'than if a binary template mask were to be used as input.')
  cmdline.add_argument('input', help='The input mask image')
  cmdline.add_argument('output', help='The output glass brain image')
  cmdline.add_argument('-dilate', type=int, default=2, help='Provide number of passes for dilation step; default = 2')
  cmdline.add_argument('-scale', type=float, default=2.0, help='Provide resolution upscaling value; default = 2.0')
  cmdline.add_argument('-smooth', type=float, default=1.0, help='Provide standard deviation of smoothing (in mm); default = 1.0')
