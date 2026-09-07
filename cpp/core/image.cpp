/* Copyright (c) 2008-2026 the MRtrix3 contributors.
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
template Image<bool>::Buffer::~Buffer();
template Image<int8_t>::Buffer::~Buffer();
template Image<uint8_t>::Buffer::~Buffer();
template Image<int16_t>::Buffer::~Buffer();
template Image<uint16_t>::Buffer::~Buffer();
template Image<int32_t>::Buffer::~Buffer();
template Image<uint32_t>::Buffer::~Buffer();
template Image<int64_t>::Buffer::~Buffer();
template Image<uint64_t>::Buffer::~Buffer();
template Image<Eigen::half>::Buffer::~Buffer();
template Image<float>::Buffer::~Buffer();
template Image<double>::Buffer::~Buffer();
template Image<cfloat>::Buffer::~Buffer();
template Image<cdouble>::Buffer::~Buffer();
} // namespace MR
