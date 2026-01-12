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

#include "fixel/filter/cfe.h"

namespace MR::Fixel::Filter {

using namespace MR::App;

// clang-format off
const OptionGroup cfe_options = OptionGroup ("Parameters for the Connectivity-based Fixel Enhancement (CFE) algorithm")

  + Option ("cfe_dh", "the height increment used in the cfe integration (default: " + str(cfe_default_dh, 2) + ")")
    + Argument ("value").type_float (0.001, 1.0)

  + Option ("cfe_e", "cfe extent exponent (default: " + str(cfe_default_e, 2) + ")")
    + Argument ("value").type_float (0.0, 100.0)

  + Option ("cfe_h", "cfe height exponent (default: " + str(cfe_default_h, 2) + ")")
    + Argument ("value").type_float (0.0, 100.0)

  + Option ("cfe_c", "cfe connectivity exponent (default: " + str(cfe_default_c, 2) + ")")
    + Argument ("value").type_float (0.0, 100.0)

  + Option ("cfe_legacy", "use the legacy (non-normalised) form of the cfe equation");
// clang-format on

CFE::CFE(const Fixel::Matrix::Reader &connectivity_matrix,
         const value_type dh,
         const value_type E,
         const value_type H,
         const value_type C,
         const bool norm)
    : matrix(connectivity_matrix), dh(dh), E(E), H(H), C(C), normalise(norm) {}

// Define functions necessary for the main CFE function to be capable of operating on both image and matrix data
namespace {
template <class ImageType>
typename std::enable_if<is_pure_image<typename std::remove_reference<ImageType>::type>::value, void>::type
zero(ImageType &image) {
  for (auto l = Loop(0)(image); l; ++l)
    image.value() = 0;
}
template <class DataType>
typename std::enable_if<!is_pure_image<typename std::remove_reference<DataType>::type>::value, void>::type
zero(DataType &data) {
  data.setZero();
}
template <class ImageType>
typename std::enable_if<is_pure_image<typename std::remove_reference<ImageType>::type>::value, default_type>::type
get(ImageType &image, const size_t index) {
  image.index(0) = index;
  return image.value();
}
template <class DataType>
typename std::enable_if<!is_pure_image<typename std::remove_reference<DataType>::type>::value, default_type>::type
get(DataType &data, const size_t index) {
  return data[index];
}
template <class ImageType>
typename std::enable_if<is_pure_image<typename std::remove_reference<ImageType>::type>::value, void>::type
set(ImageType &image, const size_t index, const default_type value) {
  assert(image.index(0) == index);
  image.value() = value;
}
template <class DataType>
typename std::enable_if<!is_pure_image<typename std::remove_reference<DataType>::type>::value, void>::type
set(DataType &data, const size_t index, const default_type value) {
  data[index] = value;
}
template <class ImageType>
typename std::enable_if<is_pure_image<typename std::remove_reference<ImageType>::type>::value, void>::type
set_index(ImageType &image, const size_t index) {
  image.index(0) = index;
}
template <class DataType>
typename std::enable_if<!is_pure_image<typename std::remove_reference<DataType>::type>::value, void>::type
set_index(DataType &data, const size_t index) {}

template <class ImageType>
typename std::enable_if<is_pure_image<typename std::remove_reference<ImageType>::type>::value, void>::type
increment(ImageType &image, const size_t index, const default_type value) {
  // Note: Assumes set_index() has already been called
  //   (avoiding unnecessary repeated re-calculation of offsets when data for the same fixel is being repeatedly
  //   incremented)
  assert(image.index(0) == index);
  image.value() += value;
}
template <class DataType>
typename std::enable_if<!is_pure_image<typename std::remove_reference<DataType>::type>::value, void>::type
increment(DataType &data, const size_t index, const default_type value) {
  data[index] += value;
}
template <class ImageType>
typename std::enable_if<is_pure_image<typename std::remove_reference<ImageType>::type>::value, void>::type
normalise_cfe(ImageType &image, const size_t index, const default_type multiplier) {
  image.value() *= multiplier;
}
template <class DataType>
typename std::enable_if<!is_pure_image<typename std::remove_reference<DataType>::type>::value, void>::type
normalise_cfe(DataType &data, const size_t index, const default_type multiplier) {
  data[index] *= multiplier;
}
} // End anonymous namespace

template <class InputType, class OutputType> void CFE::run(InputType stats, OutputType enhanced_stats) const {
  zero(enhanced_stats);
  std::vector<default_type> connected_stats;
  for (size_t fixel = 0; fixel < matrix.size(); ++fixel) {
    set_index(enhanced_stats, fixel);
    const default_type stat = get(stats, fixel);
    if (!std::isfinite(stat)) {
      set(enhanced_stats, fixel, stat);
      continue;
    }
    if (stat < dh) {
      set(enhanced_stats, fixel, 0.0);
      continue;
    }
    auto connections = matrix[fixel];
    // Need to re-normalise based on the value of the power C
    if (C != 1.0) {
      default_type sum = 0.0;
      for (auto &c : connections) {
        c.exponentiate(C);
        sum += c.value();
      }
      connections.normalise(static_cast<Fixel::Matrix::connectivity_value_type>(sum));
    }
    // Rather than allocating data for the stats and then looping over dh,
    //   divide statistic by dh to determine the number of cluster sizes that should
    //   be incremented, and dynamically increment all cluster sizes for that
    //   particular connected fixel
    std::vector<Fixel::Matrix::connectivity_value_type> extents(static_cast<size_t>(std::floor(stat / dh)),
                                                                Fixel::Matrix::connectivity_value_type(0));
    for (const auto &connection : connections) {
      const default_type connection_stat = get(stats, connection.index());
      if (connection_stat > dh) {
        const size_t cluster_count = std::min(extents.size(), static_cast<size_t>(std::floor(connection_stat / dh)));
        for (size_t cluster_index = 0; cluster_index != cluster_count; ++cluster_index)
          extents[cluster_index] += connection.value();
      }
    }
    // Pre-calculate h^H
    if (h_pow_H.size() < extents.size()) {
      const size_t old_size = h_pow_H.size();
      h_pow_H.resize(extents.size());
      for (size_t ih = old_size; ih != h_pow_H.size(); ++ih)
        h_pow_H[ih] = std::pow(dh * (ih + 1), H);
    }
    for (size_t cluster_index = 0; cluster_index != extents.size(); ++cluster_index)
      increment(enhanced_stats, fixel, std::pow(extents[cluster_index], E) * h_pow_H[cluster_index]);
    if (normalise)
      normalise_cfe(enhanced_stats, fixel, connections.norm_multiplier);
  }
}
// Explicitly instantiate two required versions of templated function
template void CFE::run(Image<float>, Image<float>) const;
template void CFE::run(in_column_type, out_column_type) const;

void CFE::operator()(Image<float> &input, Image<float> &output) const { run(input, output); }

void CFE::operator()(in_column_type stats, out_column_type enhanced_stats) const { run(stats, enhanced_stats); }

} // namespace MR::Fixel::Filter
