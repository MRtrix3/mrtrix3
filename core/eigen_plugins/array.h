/* Copyright (c) 2008-2023 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Covered Software is provided under this License on an "as is"
 * basis, without warranty of any kind, either expressed, implied, or
 * statutory, including, without limitation, warranties that the
 * Covered Software is free of defects, merchantable, fit for a
 * particular purpose or non-infringing.
 * See the Mozilla Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
 */

#define MRTRIX_OP(ARG) \
template <class ImageType> inline Array& operator ARG (const MR::Helper::ConstRow<ImageType>& row) {\
  this->resize (row.image.size(row.axis),1); \
  for (row.image.index(row.axis) = 0; row.image.index (row.axis) < row.image.size (row.axis); ++row.image.index (row.axis)) \
    this->operator() (row.image.index (row.axis), 0) ARG row.image.value(); \
  return *this; \
}

MRTRIX_OP(=)

#undef MRTRIX_OP
