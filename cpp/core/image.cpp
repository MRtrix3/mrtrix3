/* Copyright (c) 2008-2025 the MRtrix3 contributors.
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
#include "image.h"

namespace MR {
template Image<bool>::~Image();
template Image<int8_t>::~Image();
template Image<uint8_t>::~Image();
template Image<int16_t>::~Image();
template Image<uint16_t>::~Image();
template Image<int32_t>::~Image();
template Image<uint32_t>::~Image();
template Image<int64_t>::~Image();
template Image<uint64_t>::~Image();
template Image<Eigen::half>::~Image();
template Image<float>::~Image();
template Image<double>::~Image();
template Image<cfloat>::~Image();
template Image<cdouble>::~Image();
} // namespace MR
