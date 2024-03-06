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

DEFAULT_RIGID_SCALES  = [0.3,0.4,0.6,0.8,1.0,1.0]
DEFAULT_RIGID_LMAX    = [2,2,2,4,4,4]
DEFAULT_AFFINE_SCALES = [0.3,0.4,0.6,0.8,1.0,1.0]
DEFAULT_AFFINE_LMAX   = [2,2,2,4,4,4]

DEFAULT_NL_SCALES = [0.3,0.4,0.5,0.6,0.7,0.8,0.9,1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0]
DEFAULT_NL_NITER  = [  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5]
DEFAULT_NL_LMAX   = [  2,  2,  2,  2,  2,  2,  2,  2,  4,  4,  4,  4,  4,  4,  4,  4]

REGISTRATION_MODES = ['rigid', 'affine', 'nonlinear', 'rigid_affine', 'rigid_nonlinear', 'affine_nonlinear', 'rigid_affine_nonlinear']

AGGREGATION_MODES = ['mean', 'median']

IMAGEEXT = ['mif', 'nii', 'mih', 'mgh', 'mgz', 'img', 'hdr']
