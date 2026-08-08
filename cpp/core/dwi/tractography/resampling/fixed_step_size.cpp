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

#include "dwi/tractography/resampling/fixed_step_size.h"

#include "dwi/tractography/spline.h"
#include "math/hermite.h"

namespace MR::DWI::Tractography::Resampling {

bool FixedStepSize::operator()(const Streamline<> &in, Streamline<> &out) const {
  out.clear();
  if (!valid())
    return false;
  out.set_index(in.get_index());
  out.weight = in.weight;
  if (in.size() < 2)
    return true;
  Math::Hermite<value_type> interp(hermite_tension);
  // The reflected ghost vertices at index -1 and size() provide the control points
  //   required for Hermite interpolation in the first and last streamline segments.
  // Indices below are expressed over a virtual padded streamline: index 0 is the front
  //   ghost, indices [1, in.size()] are the real vertices, and index in.size()+1 is the
  //   back ghost; access maps to the view via view[index - 1].
  const SplineView<value_type> view(in);
  const size_t s = view.size();
  const ssize_t padded_size = static_cast<ssize_t>(s) + 2;
  const ssize_t midpoint = padded_size / 2;
  out.push_back(view[midpoint - 1]);
  // Generate from the midpoint to the start, reverse, then generate from midpoint to the end
  for (ssize_t step = -1; step <= 1; step += 2) {

    ssize_t index = midpoint;
    value_type mu_lower = value_type(0);

    // Loop to generate points
    do {

      // If we don't have to step along the input track, can keep the mu from the previous
      //   interpolation point as the lower bound
      while (index > 1 && index < padded_size - 2 && (out.back() - view[index + step - 1]).norm() < step_size) {
        index += step;
        mu_lower = value_type(0);
      }
      // Always preserve the termination points, regardless of resampling
      if (index == 1) {
        out.push_back(view[0]);
        std::reverse(out.begin(), out.end());
      } else if (index == padded_size - 2) {
        out.push_back(view[static_cast<ssize_t>(s) - 1]);
      } else {

        // Perform binary search
        point_type p_lower = view[index - 1], p, p_upper = view[index + step - 1];
        value_type mu_upper = value_type(1);
        value_type mu = std::numeric_limits<value_type>::quiet_NaN();
        do {
          mu = value_type(0.5) * (mu_lower + mu_upper);
          interp.set(mu);
          p = interp.value(view[index - step - 1], view[index - 1], view[index + step - 1], view[index + 2 * step - 1]);
          if ((p - out.back()).norm() < step_size) {
            mu_lower = mu;
            p_lower = p;
          } else {
            mu_upper = mu;
            p_upper = p;
          }
        } while ((p_upper - p_lower).norm() > value_type(0.001) * step_size);
        out.push_back(p);
      }

      // Loop until an endpoint has been added
    } while (index > 1 && index < padded_size - 2);
  }

  return true;
}

} // namespace MR::DWI::Tractography::Resampling
