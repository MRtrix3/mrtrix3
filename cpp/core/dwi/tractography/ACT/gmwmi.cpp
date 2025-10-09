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

#include "dwi/tractography/ACT/gmwmi.h"

namespace MR::DWI::Tractography::ACT {

const default_type GMWMI_finder::perturbation_mm = 0.001;
const ssize_t GMWMI_finder::max_iters = 10;
const default_type GMWMI_finder::hermite_tension = 0.1;

bool GMWMI_finder::find_interface(Eigen::Vector3f &p) const {
  Interp interp(interp_template);
  return find_interface(p, interp);
}

Eigen::Vector3f GMWMI_finder::normal(const Eigen::Vector3f &p) const {
  Interp interp(interp_template);
  return get_normal(p, interp);
}

Eigen::Vector3f GMWMI_finder::find_interface(const std::vector<Eigen::Vector3f> &tck, const bool end) const {
  Interp interp(interp_template);
  return find_interface(tck, end, interp);
}

void GMWMI_finder::crop_track(std::vector<Eigen::Vector3f> &tck) const {
  if (tck.size() < 3)
    return;
  Interp interp(interp_template);
  const Eigen::Vector3f new_first_point = find_interface(tck, false, interp);
  tck[0] = new_first_point;
  const Eigen::Vector3f new_last_point = find_interface(tck, true, interp);
  tck[tck.size() - 1] = new_last_point;
}

bool GMWMI_finder::find_interface(Eigen::Vector3f &p, Interp &interp) const {

  Tissues tissues;

  Eigen::Vector3f step(Eigen::Vector3f::Zero());
  size_t gradient_iters = 0;

  tissues = get_tissues(p, interp);

  do {
    step = get_cf_min_step(p, interp);
    p += step;
    tissues = get_tissues(p, interp);
  } while (tissues.valid() && step.squaredNorm() > 0.0F &&
           (std::fabs(tissues.get_gm() - tissues.get_wm()) > gmwmi_accuracy) && ++gradient_iters < max_iters);

  // Make sure an appropriate cost function minimum has been found, and that
  //   this would be an acceptable termination point if it were processed by the tracking algorithm
  if (!tissues.valid() || tissues.is_csf() || tissues.is_path() || !tissues.get_wm() ||
      (std::fabs(tissues.get_gm() - tissues.get_wm()) > gmwmi_accuracy)) {

    p.fill(NaNF);
    return false;
  }

  if (tissues.get_gm() >= tissues.get_wm())
    return true;

  step = get_cf_min_step(p, interp);
  if (!step.allFinite())
    return true;
  if (!step.squaredNorm()) {
    p.fill(NaNF);
    return false;
  }

  do {
    step *= 1.5;
    p += step;
    tissues = get_tissues(p, interp);

    if (tissues.valid() && (tissues.get_gm() >= tissues.get_wm()) &&
        (tissues.get_gm() - tissues.get_wm() < gmwmi_accuracy))
      return true;

  } while (step.norm() < 0.5 * min_vox);

  p.fill(NaNF);
  return false;
}

Eigen::Vector3f GMWMI_finder::get_normal(const Eigen::Vector3f &p, Interp &interp) const {

  Eigen::Vector3f normal(Eigen::Vector3f::Zero());

  for (size_t axis = 0; axis != 3; ++axis) {

    Eigen::Vector3f p_minus(p);
    p_minus[axis] -= 0.5 * perturbation_mm;
    const Tissues v_minus = get_tissues(p_minus, interp);

    Eigen::Vector3f p_plus(p);
    p_plus[axis] += 0.5 * perturbation_mm;
    const Tissues v_plus = get_tissues(p_plus, interp);

    normal[axis] = (v_plus.get_wm() - v_plus.get_gm()) - (v_minus.get_wm() - v_minus.get_gm());
  }

  return normal.normalized();
}

Eigen::Vector3f GMWMI_finder::get_cf_min_step(const Eigen::Vector3f &p, Interp &interp) const {

  Eigen::Vector3f grad(Eigen::Vector3f::Zero());

  for (size_t axis = 0; axis != 3; ++axis) {

    Eigen::Vector3f p_minus(p);
    p_minus[axis] -= 0.5 * perturbation_mm;
    const Tissues v_minus = get_tissues(p_minus, interp);

    Eigen::Vector3f p_plus(p);
    p_plus[axis] += 0.5 * perturbation_mm;
    const Tissues v_plus = get_tissues(p_plus, interp);

    if (!v_minus.valid() || !v_plus.valid())
      return Eigen::Vector3f::Zero();

    grad[axis] = (v_plus.get_gm() - v_plus.get_wm()) - (v_minus.get_gm() - v_minus.get_wm());
  }

  grad *= (1.0 / perturbation_mm);

  if (!grad.squaredNorm())
    return Eigen::Vector3f::Zero();

  const Tissues local_tissue = get_tissues(p, interp);
  const float diff = local_tissue.get_gm() - local_tissue.get_wm();

  Eigen::Vector3f step = -grad * (diff / grad.squaredNorm());

  const float norm = step.norm();
  if (norm > 0.5 * min_vox)
    step *= 0.5 * min_vox / norm;
  return step;
}

Eigen::Vector3f
GMWMI_finder::find_interface(const std::vector<Eigen::Vector3f> &tck, const bool end, Interp &interp) const {

  if (tck.empty())
    return Eigen::Vector3f::Constant(NaNF);

  if (tck.size() == 1)
    return tck.front();

  if (tck.size() == 2)
    return (end ? tck.back() : tck.front());

  // Track is long enough; can do the proper search
  // Need to generate an additional point beyond the end point
  size_t last = tck.size() - 1;

  const Eigen::Vector3f p_end(end ? tck.back() : tck.front());
  const Eigen::Vector3f p_prev(end ? tck[last - 1] : tck[1]);

  // Before proceeding, make sure that the interface lies somewhere in between these two points
  if (!interp.scanner(p_end))
    return p_end;
  const Tissues t_end(interp);
  if (!interp.scanner(p_prev))
    return p_end;
  const Tissues t_prev(interp);
  if (!(((t_end.get_gm() > t_end.get_wm()) && (t_prev.get_gm() < t_prev.get_wm())) ||
        ((t_end.get_gm() < t_end.get_wm()) && (t_prev.get_gm() > t_prev.get_wm())))) {
    return p_end;
  }

  // Also make sure that the existing endpoint doesn't already obey the criterion
  if (t_end.get_gm() - t_end.get_wm() < gmwmi_accuracy)
    return p_end;

  const Eigen::Vector3f curvature(end ? ((tck[last] - tck[last - 1]) - (tck[last - 1] - tck[last - 2]))
                                      : ((tck[0] - tck[1]) - (tck[1] - tck[2])));
  const Eigen::Vector3f extrap((end ? (tck[last] - tck[last - 1]) : (tck[0] - tck[1])) + curvature);
  const Eigen::Vector3f p_extrap(p_end + extrap);

  Eigen::Vector3f domain[4];
  domain[0] = (end ? tck[last - 2] : tck[2]);
  domain[1] = p_prev;
  domain[2] = p_end;
  domain[3] = p_extrap;

  Math::Hermite<float> hermite(hermite_tension);

  float min_mu = 0.0, max_mu = 1.0;
  Eigen::Vector3f p_best = p_end;
  size_t iters = 0;
  do {
    const float mu = 0.5 * (min_mu + max_mu);
    hermite.set(mu);
    const Eigen::Vector3f p(hermite.value(domain));
    interp.scanner(p);
    const Tissues t(interp);
    if (t.get_wm() > t.get_gm()) {
      min_mu = mu;
    } else {
      max_mu = mu;
      p_best = p;
      if (t.get_gm() - t.get_wm() < gmwmi_accuracy)
        return p_best;
    }
  } while (++iters != max_iters);

  return p_best;
}

} // namespace MR::DWI::Tractography::ACT
