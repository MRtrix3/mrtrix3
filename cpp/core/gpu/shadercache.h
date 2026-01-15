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

#pragma once

#include <string>
#include <unordered_map>

namespace MR::GPU {
class ShaderCache {
public:
  using CacheKey = std::string;
  using CacheValue = std::string;
  using CacheMap = std::unordered_map<CacheKey, CacheValue>;

  explicit ShaderCache() = default;

  bool contains(const CacheKey &key) const;

  void insert(const CacheKey &key, const CacheValue &value);

  const CacheValue &get(const CacheKey &key) const;

  void clear();

private:
  CacheMap m_cache;
};

} // namespace MR::GPU
