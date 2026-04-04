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

#include "connectome/validate.h"

#include <cstdint>
#include <limits>
#include <map>
#include <numeric>
#include <set>
#include <vector>

#include "algo/loop.h"
#include "app.h"
#include "connectome/connectome.h"
#include "exception.h"
#include "header.h"
#include "image.h"
#include "mrtrix.h"

namespace MR::Connectome {

// The 13 "forward" half-neighbours for 26-NN connectivity.
// Processing only these avoids visiting each unordered pair twice.
// Composed of:
//   dz=0: (1,0,0), (-1,1,0), (0,1,0), (1,1,0)           — 4 neighbours
//   dz=1: all (dx,dy,1) for dx,dy in {-1,0,+1}           — 9 neighbours
static constexpr int32_t half_offsets[13][3] = {{1, 0, 0},
                                                {-1, 1, 0},
                                                {0, 1, 0},
                                                {1, 1, 0},
                                                {-1, -1, 1},
                                                {0, -1, 1},
                                                {1, -1, 1},
                                                {-1, 0, 1},
                                                {0, 0, 1},
                                                {1, 0, 1},
                                                {-1, 1, 1},
                                                {0, 1, 1},
                                                {1, 1, 1}};

LabelValidation validate_label_image(const Header &H) {
  // Basic format validation: 3D (or 4D with singleton), non-negative integers only.
  // Reuse the existing Connectome::check(), which throws on any violation.
  Connectome::check(H);

  const uint32_t X = static_cast<uint32_t>(H.size(0));
  const uint32_t Y = static_cast<uint32_t>(H.size(1));
  const uint32_t Z = static_cast<uint32_t>(H.size(2));

  // Guard against images too large for the uint32_t index space used below.
  if (static_cast<uint64_t>(X) * Y * Z > static_cast<uint64_t>(std::numeric_limits<uint32_t>::max()))
    throw Exception("Image \"" + std::string(H.name()) +
                    "\" has too many voxels (> 2^32)"
                    " for label validation");

  const uint32_t total = X * Y * Z;

  // ---------------------------------------------------------------
  // Load label values into a flat array (x varies fastest)
  // ---------------------------------------------------------------
  std::vector<node_t> lflat(total, 0);
  {
    auto img = Image<node_t>::open(H.name());
    for (auto l = Loop(img, 0, 3)(img); l; ++l)
      lflat[static_cast<uint32_t>(img.index(0)) + X * static_cast<uint32_t>(img.index(1)) +
            X * Y * static_cast<uint32_t>(img.index(2))] = img.value();
  }

  LabelValidation result;

  // ---------------------------------------------------------------
  // Collect unique non-background labels
  // ---------------------------------------------------------------
  for (const node_t v : lflat) {
    if (v)
      result.labels.insert(v);
  }

  // ---------------------------------------------------------------
  // Check index contiguity: are labels {1, 2, ..., max_label}?
  // ---------------------------------------------------------------
  result.indices_contiguous = true;
  if (!result.labels.empty()) {
    const node_t max_label = *result.labels.rbegin();
    for (node_t i = 1; i <= max_label; ++i) {
      if (!result.labels.count(i)) {
        result.indices_contiguous = false;
        result.missing_indices.push_back(i);
      }
    }
  }

  // ---------------------------------------------------------------
  // Spatial contiguity via union-find with 26-NN connectivity
  // ---------------------------------------------------------------
  // parent[i] is the representative of the connected component containing voxel i.
  std::vector<uint32_t> parent(total);
  std::iota(parent.begin(), parent.end(), uint32_t(0));

  // Path-halving find: amortised near-O(1) per operation.
  auto find = [&parent](uint32_t x) -> uint32_t {
    while (parent[x] != x) {
      parent[x] = parent[parent[x]];
      x = parent[x];
    }
    return x;
  };

  for (uint32_t z = 0; z < Z; ++z) {
    for (uint32_t y = 0; y < Y; ++y) {
      for (uint32_t x = 0; x < X; ++x) {
        const uint32_t idx = x + X * y + X * Y * z;
        const node_t label = lflat[idx];
        if (!label)
          continue;

        for (const auto &o : half_offsets) {
          const int32_t nx = static_cast<int32_t>(x) + o[0];
          const int32_t ny = static_cast<int32_t>(y) + o[1];
          const int32_t nz = static_cast<int32_t>(z) + o[2];
          if (nx < 0 || nx >= static_cast<int32_t>(X))
            continue;
          if (ny < 0 || ny >= static_cast<int32_t>(Y))
            continue;
          if (nz < 0 || nz >= static_cast<int32_t>(Z))
            continue;
          const uint32_t nidx =
              static_cast<uint32_t>(nx) + X * static_cast<uint32_t>(ny) + X * Y * static_cast<uint32_t>(nz);
          if (lflat[nidx] != label)
            continue;
          const uint32_t ra = find(idx), rb = find(nidx);
          if (ra != rb)
            parent[ra] = rb;
        }
      }
    }
  }

  // Count the number of distinct component roots per label.
  std::map<node_t, std::set<uint32_t>> roots_per_label;
  for (uint32_t i = 0; i < total; ++i) {
    if (lflat[i])
      roots_per_label[lflat[i]].insert(find(i));
  }
  for (const auto &[label, roots] : roots_per_label)
    result.component_counts[label] = static_cast<uint32_t>(roots.size());

  return result;
}

void debug_validate_label_image(const Header &H) {
  if (App::log_level < 3)
    return;
  try {
    const LabelValidation v = validate_label_image(H);
    if (!v.indices_contiguous) {
      DEBUG("Label image \"" + std::string(H.name()) +
            "\": indices are non-contiguous"
            " (" +
            str(v.missing_indices.size()) + " gap(s) in range [1, " + str(*v.labels.rbegin()) + "])");
    }
    for (const auto &[label, count] : v.component_counts) {
      if (count > 1)
        DEBUG("Label image \"" + std::string(H.name()) + "\": label " + str(label) + " is spatially disconnected (" +
              str(count) + " components)");
    }
  } catch (const Exception &e) {
    DEBUG("Label image \"" + std::string(H.name()) + "\": validation failed: " + e[0]);
  }
}

} // namespace MR::Connectome
