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

#include "match_variant.h"
#include "gpu/registration/ncccalculator.h"
#include "gpu/registration/nmicalculator.h"
#include "gpu/registration/registrationtypes.h"
#include "gpu/registration/ssdcalculator.h"

#include <cassert>
#include <variant>

namespace MR::GPU {

class Calculator final : public std::variant<NMICalculator, SSDCalculator, NCCCalculator> {
public:
  // Expose std::variant constructors publicly
  using std::variant<NMICalculator, SSDCalculator, NCCCalculator>::variant;

  struct Config {
    Texture fixed_texture;
    Texture moving_texture;
    MR::Transform fixed_transform;
    MR::Transform moving_transform;
    float downscale_factor;
    Metric metric;
  };

  void update(const GlobalTransform &transformation) {
    MR::match_v(*this, [&](auto &&arg) { arg.update(transformation); });
  }

  IterationResult get_result() const {
    return MR::match_v(*this, [&](auto &&arg) { return arg.get_result(); });
  }
};

} // namespace MR::GPU
