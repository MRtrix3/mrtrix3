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

#include "dwi/fmls.h"

#include "dwi/tractography/SIFT/model_base.h"

namespace MR::DWI::Tractography::SIFT {

class Fixel : public FixelBase {

public:
  Fixel() : FixelBase() {}

  Fixel(const FMLS::FOD_lobe &lobe) : FixelBase(lobe) {}

  Fixel(const Fixel &that) = default;

  Fixel &operator-=(const double length) {
    TD = std::max(TD - length, 0.0);
    return *this;
  }

  [[nodiscard]] double get_d_cost_d_mu(const double mu) const { return get_d_cost_d_mu_unweighted(mu) * weight; }
  [[nodiscard]] double get_cost_wo_track(const double mu, const double length) const {
    return get_cost_wo_track_unweighted(mu, length) * weight;
  }
  [[nodiscard]] double get_cost_manual_TD(const double mu, const double manual_TD) const {
    return get_cost_manual_TD_unweighted(mu, manual_TD) * weight;
  }
  [[nodiscard]] double calc_quantisation(const double mu, const double length) const {
    return get_cost_manual_TD(mu, (FOD / mu) + length);
  }

private:
  [[nodiscard]] double get_d_cost_d_mu_unweighted(const double mu) const { return (2.0 * TD * get_diff(mu)); }
  [[nodiscard]] double get_cost_wo_track_unweighted(const double mu, const double length) const {
    return (Math::pow2((std::max(TD - length, 0.0) * mu) - FOD));
  }
  [[nodiscard]] double get_cost_manual_TD_unweighted(const double mu, const double manual_TD) const {
    return Math::pow2((manual_TD * mu) - FOD);
  }
};

} // namespace MR::DWI::Tractography::SIFT
