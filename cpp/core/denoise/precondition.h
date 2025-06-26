/* Copyright (c) 2008-2024 the MRtrix3 contributors.
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
#include <vector>

#include "app.h"
#include "denoise/kernel/voxel.h"
#include "filter/demodulate.h"
#include "header.h"
#include "image.h"
#include "types.h"

namespace MR::Denoise {

extern const char *const demodulation_description;

const std::vector<std::string> demodulation_choices({"none", "linear", "nonlinear"});
enum class demodulation_t { NONE, LINEAR, NONLINEAR };

const std::vector<std::string> demean_choices = {"none", "volume_groups", "shells", "all"};
enum class demean_type { NONE, VOLUME_GROUPS, SHELLS, ALL };

App::OptionGroup precondition_options(const bool include_output);

class Demodulation {
public:
  Demodulation(demodulation_t mode) : mode(mode) {}
  Demodulation() : mode(demodulation_t::NONE) {}
  explicit operator bool() const { return mode != demodulation_t::NONE; }
  bool operator!() const { return mode == demodulation_t::NONE; }
  demodulation_t mode;
  std::vector<size_t> axes;
};
Demodulation select_demodulation(const Header &);

demean_type select_demean(const Header &);

// Need to SFINAE define the demodulator type,
//   so that it does not attempt to compile the demodulation filter for non-complex types
class DummyDemodulator {
public:
  template <class ImageType> DummyDemodulator(ImageType &, const std::vector<size_t> &, const bool) {}
  template <class InputImageType, class OutputImageType>
  void operator()(InputImageType &, OutputImageType &, const bool) {
    assert(false);
  }
  Image<cfloat> operator()() { return Image<cfloat>(); }
};
template <typename T> struct DemodulatorSelector {
  using type = DummyDemodulator;
};
template <typename T> struct DemodulatorSelector<std::complex<T>> {
  using type = Filter::Demodulate;
};

template <typename T> class Precondition {
public:
  Precondition(Image<T> &image, const Demodulation &demodulation, const demean_type demean, Image<float> &vst);
  Precondition(Precondition &) = default;
  void update_vst_image(Image<float> new_vst_image) { vst_image = new_vst_image; }
  void operator()(Image<T> input, Image<T> output, const bool inverse = false) const;
  const Header &header() const { return H_out; }

  ssize_t null_rank() const {
    if (!mean_image.valid())
      return 0;
    if (mean_image.ndim() == 3)
      return 1;
    return mean_image.size(3);
  }

  bool noop() const {
    return (num_volume_groups == 1 && !phase_image.valid() && !mean_image.valid() && !vst_image.valid());
  }

private:
  const Header H_in;
  Header H_out;
  // For serialisation of >4D images
  ssize_t num_volume_groups;
  Image<ssize_t> serialise_image;
  // First step: Phase demodulation
  Image<cfloat> phase_image;
  // Second step: Demeaning
  std::vector<ssize_t> index2shell;
  std::vector<ssize_t> index2group;
  Image<T> mean_image;
  // Third step: Variance-stabilising transform
  Image<float> vst_image;
};

// TODO New function that will pad a VST image by one voxel in every direction,
//   making it more likely to succeed in obtaining interpolated values for every input voxel
// (eventually this should be subsumed by an edge handling adapter)

} // namespace MR::Denoise
