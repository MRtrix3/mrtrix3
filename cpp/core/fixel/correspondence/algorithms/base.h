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

#pragma once

#include <filesystem>

#include "image.h"
#include "types.h"

#include "fixel/correspondence/correspondence.h"
#include "fixel/correspondence/fixel.h"
#include "fixel/correspondence/mapping.h"

namespace MR::Fixel::Correspondence::Algorithms {

class Base {
public:
  Base() = default;
  virtual ~Base() = default;

  void export_cost_image(const std::filesystem::path &path) {
    if (!cost_image.valid())
      return;
    Image<float> output(Image<float>::create(path, cost_image));
    copy(cost_image, output);
  }

  virtual std::vector<std::vector<Mapping::Entry>> operator()(const voxel_t &v,
                                                              const std::vector<Correspondence::Fixel> &s,
                                                              const std::vector<Correspondence::Fixel> &t) const = 0;

  /// @brief Whether this algorithm consumes per-fixel dixel masks.
  /// When true, the Matcher opens the dixel-mask images and calls the mask-carrying operator() overload.
  virtual bool requires_masks() const { return false; }

  /// @brief Mask-carrying entry point.
  /// The default ignores the masks and forwards to the 3-argument form, so mask-agnostic
  ///   algorithms (e.g. All2All, Legacy) need no modification.
  virtual std::vector<std::vector<Mapping::Entry>> operator()(const voxel_t &v,
                                                              const std::vector<Correspondence::Fixel> &s,
                                                              const std::vector<Correspondence::Fixel> &t,
                                                              const std::vector<dixel_mask_t> &s_masks,
                                                              const std::vector<dixel_mask_t> &t_masks) const {
    (void)s_masks;
    (void)t_masks;
    return (*this)(v, s, t);
  }

protected:
  Image<float> cost_image;
};

} // namespace MR::Fixel::Correspondence::Algorithms
