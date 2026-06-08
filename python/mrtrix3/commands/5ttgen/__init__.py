# Copyright (c) 2008-2026 the MRtrix3 contributors.
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

# MRtrix3 commands that this command may invoke at execution.
# This list is parsed at build configure time to establish the set of
#   compilation targets required by this command (see the cmake Python command dependency helpers),
#   and is verified for completeness by the dependency linter run within continuous integration.
# pylint: disable=unused-variable
MRTRIX_DEPENDENCIES = ['5ttedit', '5ttvalidate', 'labelconvert', 'labelvalidate', 'maskdump', 'maskfilter',
                       'mesh2voxel', 'meshconvert', 'meshfilter', 'mrcalc', 'mrcat', 'mrconvert', 'mredit', 'mrgrid',
                       'mrinfo', 'mrmath', 'mrthreshold', 'mrtransform', 'voxel2mesh']

# pylint: disable=unused-variable
ALGORITHMS = ['deep_atropos', 'freesurfer', 'fsl', 'gif', 'hsvs']
