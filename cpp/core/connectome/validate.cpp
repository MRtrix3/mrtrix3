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
#include "datatype.h"
#include "exception.h"
#include "filter/connected_components.h"
#include "header.h"
#include "image.h"
#include "misc/voxel2vector.h"
#include "mrtrix.h"
#include "thread_queue.h"

namespace MR::Connectome {

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
    if (v > 0U)
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
      if (label_set.count(i) == size_t(0)) {
        result.indices_contiguous = false;
        result.missing_indices.push_back(i);
      }
    }
  }

  // ---------------------------------------------------------------
  // Spatial contiguity via connected-component analysis
  // ---------------------------------------------------------------

  class Seeder {
  public:
    Seeder(const std::vector<node_t> &labels) : data(labels), index(0) {}
    bool operator()(node_t &out) {
      if (index == data.size()) {
        out = std::numeric_limits<node_t>::max();
        return false;
      }
      out = data[index++];
      return true;
    }

  private:
    std::vector<node_t> data;
    size_t index;
  };
  class Worker {
  public:
    Worker(const Image<node_t> &image) : image(image), H_3D(std::make_shared<Header>(image)) { H_3D->ndim() = 3; }
    Worker(const Worker &) = default;
    bool operator()(const node_t &label, std::pair<node_t, uint32_t> &count) {
      Image<bool> mask = Image<bool>::scratch(*H_3D, "Scratch boolean mask per unique label");
      for (auto l = Loop(image)(image, mask); l; ++l) {
        if (image.value() == label)
          mask.value() = true;
      }
      const Voxel2Vector v2v(mask, *H_3D);
      Filter::Connector connector;
      connector.adjacency.set_axes(Filter::Base::axis_mask_type::Ones(3));
      connector.adjacency.set_26_adjacency(true);
      connector.adjacency.initialise(mask, v2v);
      std::vector<Filter::Connector::Cluster> clusters;
      std::vector<uint32_t> labels;
      connector.run(clusters, labels);
      count = {label, clusters.size()};
      return true;
    }

  private:
    Image<node_t> image;
    std::shared_ptr<Header> H_3D;
  };
  class Receiver {
  public:
    Receiver(const node_t num_labels) : progress("Assessing spatial contiguity of labels", num_labels) {}
    bool operator()(const std::pair<node_t, uint32_t> &in) {
      data.insert(in);
      ++progress;
      return true;
    }
    const std::map<node_t, uint32_t> &count_per_label() const { return data; }
    node_t disconnected_count() const {
      node_t result = 0;
      for (const auto &item : data) {
        if (item.second > 1)
          ++result;
      }
      return result;
    }

  private:
    ProgressBar progress;
    std::map<node_t, uint32_t> data;
  };

  Receiver receiver(result.labels.size());
  Thread::run_queue(Seeder(result.labels), node_t(0), Worker(image), std::pair<node_t, uint32_t>({0, 0}), receiver);
  result.component_counts = receiver.count_per_label();
  result.disconnected_components = receiver.disconnected_count();

  return result;
}

void debug_validate_label_image(const Image<node_t> &image) {
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
