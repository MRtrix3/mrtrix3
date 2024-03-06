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

NEEDS_MEAN_BZERO = True

SOFTWARES = ['antsfull', 'antsquick', 'fsl']
DEFAULT_SOFTWARE = 'antsquick'

ANTS_REGISTERFULL_CMD = 'antsRegistration'
ANTS_REGISTERQUICK_CMD = 'antsRegistrationSyNQuick.sh'
ANTS_TRANSFORM_CMD = 'antsApplyTransforms'


# From Tustison and Avants, Frontiers in Neuroinformatics 2013:
ANTS_REGISTERFULL_OPTIONS = \
  ' --use-histogram-matching 1' \
  ' --initial-moving-transform [template_image.nii,bzero.nii,1]' \
  ' --transform Rigid[0.1]' \
  ' --metric MI[template_image.nii,bzero.nii,1,32,Regular,0.25]' \
  ' --convergence 1000x500x250x100' \
  ' --smoothing-sigmas 3x2x1x0' \
  ' --shrink-factors 8x4x2x1' \
  ' --transform Affine[0.1]' \
  ' --metric MI[template_image.nii,bzero.nii,1,32,Regular,0.25]' \
  ' --convergence 1000x500x250x100' \
  ' --smoothing-sigmas 3x2x1x0' \
  ' --shrink-factors 8x4x2x1' \
  ' --transform BSplineSyN[0.1,26,0,3]' \
  ' --metric CC[template_image.nii,bzero.nii,1,4]' \
  ' --convergence 100x70x50x20' \
  ' --smoothing-sigmas 3x2x1x0' \
  ' --shrink-factors 6x4x2x1'

ANTS_REGISTERQUICK_OPTIONS = '-j 1'
