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

# A collection of functions for extracting information from images. Mostly these involve
#   calling the relevant MRtrix3 binaries in order to parse image headers / process image
#   data, rather than trying to duplicate support for all possible image formats natively
#   in Python.


import math

def n_for_l(lmax): #pylint: disable=unused-variable
  """ number of even-degree coefficients for the given lmax """
  return (lmax+1) * (lmax+2) // 2


def l_for_n(coeffs): #pylint: disable=unused-variable
  """ largest even-degree given number of coefficients """
  if coeffs == 0:
    return 0
  return 2 * int(math.floor(math.sqrt(1.0 + 8.0 * coeffs - 3.0) / 4.0))
