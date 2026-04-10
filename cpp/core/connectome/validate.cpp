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

void validate_label_header(const Header &H) {
  // Basic format validation: 3D (or 4D with singleton), non-negative integers only.
  // Reproduces the previous Connectome::check().
  // Throws on any violation.
  if (!(H.ndim() == 3 || (H.ndim() == 4 && H.size(3) == 1)))
    throw Exception("Image \"" + std::string(H.name()) + "\" is not 3D," + //
                    " and hence is not a volume of node parcel indices");  //
  if (H.datatype().is_floating_point()) {
    CONSOLE("Image \"" + std::string(H.name()) + "\" stored with floating-point type;" + //
            " need to check for non-integer or negative values");                        //
    // Need to open the image WITHOUT using the IO handler stored in H;
    //   creating an image from this "claims" the handler from the header, and
    //   therefore once this check has completed the image can no longer be opened
    auto test = Image<float>::open(H.name());
    for (auto l = Loop("Verifying parcellation image", test)(test); l; ++l) {
      if (std::round(static_cast<float>(test.value())) != test.value())
        throw Exception("Floating-point number detected in image \"" + std::string(H.name()) + "\";" + //
                        " label images should contain integers only");                                 //
      if (static_cast<float>(test.value()) < 0.0F)
        throw Exception("Negative value detected in image \"" + std::string(H.name()) + "\";" + //
                        " label images must be strictly non-negative");                         //
    }
    // WARN ("Image \"" + H.name() + "\" stored as floating-point;"
    //       " it is preferable to store label images using an unsigned integer type");
  } else if (H.datatype().is_signed()) {
    CONSOLE("Image \"" + std::string(H.name()) + "\" stored with signed integer type;" + //
            " need to check for negative values");                                       //
    auto test = Image<int64_t>::open(H.name());
    for (auto l = Loop("Verifying parcellation image", test)(test); l; ++l) {
      if (static_cast<int64_t>(test.value()) < int64_t(0))
        throw Exception("Negative value detected in image \"" + std::string(H.name()) + "\";" + //
                        " label images must be strictly non-negative");                         //
    }
    // WARN ("Image \"" + H.name() + "\" stored as signed integer; it is preferable to store label images using an
    // unsigned integer type");
  }
}

const LabelValidation validate_label_image(Image<node_t> image) {
  validate_label_header(image);

  // TODO Could consider using existing connected-component analysis code
  // Possible approach:
  //   - Determine mapping from input index to contiguous indices
  //   - Generate 4D binary image with one volume per unique index
  //   - Run connected-component analysis w. 26-neighbour connectivity
  //     but no adjacency across volumes
  //   - Each cluster can be ascribed to a volume index
  //     (and hence back to input parcellation index)
  //     based on the volume index of any voxel in the cluster

  const uint32_t X = static_cast<uint32_t>(image.size(0));
  const uint32_t Y = static_cast<uint32_t>(image.size(1));
  const uint32_t Z = static_cast<uint32_t>(image.size(2));

  // Guard against images too large for the uint32_t index space used below.
  if (static_cast<uint64_t>(X) * static_cast<uint64_t>(Y) * static_cast<uint64_t>(Z) >
      static_cast<uint64_t>(std::numeric_limits<uint32_t>::max()))
    throw Exception("Image \"" + std::string(image.name()) + "\"" +        //
                    " has too many voxels (> 2^32) for label validation"); //

  const uint32_t total = X * Y * Z;

  // ---------------------------------------------------------------
  // Load label values into a flat array (x varies fastest)
  // ---------------------------------------------------------------
  std::vector<node_t> lflat(total, 0);
  for (auto l = Loop(image, 0, 3)(image); l; ++l)
    lflat[static_cast<uint32_t>(image.index(0)) +                         //
          X * static_cast<uint32_t>(image.index(1)) +                     //
          X * Y * static_cast<uint32_t>(image.index(2))] = image.value(); //

  LabelValidation result;

  // ---------------------------------------------------------------
  // Collect unique non-background labels
  // ---------------------------------------------------------------
  std::set<node_t> label_set;
  for (const node_t v : lflat) {
    if (v)
      label_set.insert(v);
  }
  result.labels = MR::container_cast<std::vector<node_t>>(label_set);

  // ---------------------------------------------------------------
  // Check index contiguity: are labels {1, 2, ..., max_label}?
  // ---------------------------------------------------------------
  result.indices_contiguous = true;
  if (!label_set.empty()) {
    const node_t max_label = result.labels.back();
    for (node_t i = 1; i <= max_label; ++i) {
      if (!label_set.count(i)) {
        result.indices_contiguous = false;
        result.missing_indices.push_back(i);
      }
    }
  }

  // ---------------------------------------------------------------
  // Spatial contiguity via union-find with 26-NN connectivity
  // ---------------------------------------------------------------
  // parent[i] is the representative of the connected component containing voxel i.
  std::vector<uint32_t> parent(total, uint32_t(0));

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
          const uint32_t nidx = static_cast<uint32_t>(nx) +     //
                                X * static_cast<uint32_t>(ny) + //
                                X * Y * static_cast<uint32_t>(nz);
          if (lflat[nidx] != label)
            continue;
          const uint32_t ra = find(idx);
          const uint32_t rb = find(nidx);
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
  result.disconnected_components = 0;
  for (const auto &[label, roots] : roots_per_label) {
    result.component_counts[label] = static_cast<uint32_t>(roots.size());
    if (roots.size() > 1)
      ++result.disconnected_components;
  }

  return result;
}

void debug_validate_label_image(Image<node_t> image) {
  validate_label_header(image);
  if (App::log_level < 3)
    return;
  LabelValidation result;
  try {
    result = validate_label_image(image);
  } catch (const Exception &e) {
    throw Exception(e, "Label image \"" + std::string(image.name()) + "\" validation failed");
  }
  if (!result.indices_contiguous) {
    DEBUG("Label image \"" + std::string(image.name()) + "\":" +   //
          " indices are non-contiguous" +                          //
          " (" + str(result.missing_indices.size()) + " gap(s)" +  //
          " in range [1, " + str(*result.labels.rbegin()) + "])"); //
  }
  if (result.disconnected_components > 0)
    DEBUG("Label image \"" + std::string(image.name()) + "\":" +                               //
          str(result.disconnected_components) + " label" +                                     //
          (result.disconnected_components > 0 ? "s are" : " is") + " spatially disconnected"); //
}

} // namespace MR::Connectome
