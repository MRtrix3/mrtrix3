/* Copyright (c) 2008-2017 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
 */

#pragma once

#include <string_view>

#include "types.h"

#include "fixel/correspondence/correspondence.h"

namespace MR::Fixel::Correspondence {

class Mapping {
public:
  /// @brief A single fixel-to-fixel association: destination index and fractional contribution weight.
  struct Entry {
    index_type index;
    float weight;
  };

  Mapping(const index_type source_fixels, const index_type target_fixels);
  Mapping(std::string_view path);

  void load(std::string_view path, const bool import_inverse = false);

  // Save to CSR format as an uncompressed .npz archive containing six .npy entries:
  // - indptr_forward.npy: (Nt+1) npz_index_type vector, CSR index pointer array for forward mapping
  // - indices_forward.npy: C npz_index_type vector, source fixel indices to pull into target fixels
  // - data_forward.npy: C npz_value_type vector, fractional contribution of each source fixel to its target fixel
  // - indptr_inverse.npy: (Ns+1) npz_index_type vector, CSR index pointer array for inverse mapping
  // - indices_inverse.npy: C npz_index_type vector, target fixel indices to pull into source fixels
  // - data_inverse.npy: C npz_value_type vector, forward weights per (source, target) pair normalised to unity sum per
  // target
  void save(std::string_view path) const;

  const std::vector<Entry> &operator[](const size_t index) const { return M[index]; }

  class Value {
  public:
    Value(std::vector<std::vector<Entry>> &M, const size_t index) : M(M), index(index) { assert(index < M.size()); }
    const std::vector<Entry> &operator()() const { return M[index]; }
    const std::vector<Entry> &operator=(const std::vector<Entry> &entries) {
      M[index] = entries;
      return M[index];
    }
    const Entry &operator[](const size_t i) const {
      assert(i < M[index].size());
      return M[index][i];
    }

  private:
    std::vector<std::vector<Entry>> &M;
    const size_t index;
  };
  Value operator[](const size_t index) { return Value(M, index); }

  size_t size() const { return M.size(); }

  Mapping inverse() const;

private:
  index_type source_fixels, target_fixels;
  std::vector<std::vector<Entry>> M;
};

} // namespace MR::Fixel::Correspondence
