/*
 * Copyright (c) 2008-2018 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/
 *
 * MRtrix3 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/
 */


template <class ImageType> Matrix (const MR::Helper::ConstRow<ImageType>& row) : Base () { operator= (row); }
template <class ImageType> Matrix (const MR::Helper::Row<ImageType>& row) : Base () { operator= (row); }

#define MRTRIX_OP(ARG) \
template <class ImageType> inline Matrix& operator ARG (const MR::Helper::ConstRow<ImageType>& row) { \
  this->resize (row.image.size(row.axis),1); \
  for (row.image.index(row.axis) = 0; row.image.index (row.axis) < row.image.size (row.axis); ++row.image.index (row.axis)) \
    this->operator() (row.image.index (row.axis), 0) ARG row.image.value(); \
  return *this; \
}

MRTRIX_OP(=)

#undef MRTRIX_OP
