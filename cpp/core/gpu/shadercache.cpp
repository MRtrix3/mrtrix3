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

#include "shadercache.h"

namespace MR::GPU {
bool ShaderCache::contains(const CacheKey &key) const { return m_cache.find(key) != m_cache.end(); }

void ShaderCache::insert(const CacheKey &key, const CacheValue &value) { m_cache.insert_or_assign(key, value); }

const ShaderCache::CacheValue &ShaderCache::get(const CacheKey &key) const { return m_cache.at(key); }

void ShaderCache::clear() { m_cache.clear(); }
} // namespace MR::GPU
