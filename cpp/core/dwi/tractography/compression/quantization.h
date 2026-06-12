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

#include <cmath>

namespace MR::DWI::Tractography::Compression {

//! \brief The uniform-quantization precision p for a worst-case error (paper §5.1).
/*! p = -1 (snap to the 0.1 mm grid) when the user's maximum error is below
 * 0.2 mm, else p = 0 (snap to the 1 mm grid). */
inline int select_precision(const double max_error_mm) { return (max_error_mm < 0.2) ? -1 : 0; }

//! \brief The worst-case Euclidean quantization error budget α = √3·10ᵖ (Table 7).
/*! Reserved from the user's maximum error so that the linearization tolerance
 * plus the quantization error respects the worst case. Per the paper's Table 7
 * tabulation (p=0 → 1.73295, p=-1 → 0.17321). */
inline double quantization_error(const int precision) { return std::sqrt(3.0) * std::pow(10.0, precision); }

//! \brief Uniformly quantize one coordinate to the 10ᵖ grid (Eq. 5).
/*! Rounds to nearest (the paper's round operator), narrows to float (the on-disk
 * symbol dtype), and normalises a negative zero so logically equal values share
 * a single histogram bin. */
inline float uniform_quantize(const double coordinate, const int precision) {
  const double beta = std::pow(10.0, precision);
  const double value = std::round(coordinate / beta) * beta;
  float narrowed = static_cast<float>(value);
  if (narrowed == 0.0F)
    narrowed = 0.0F;
  return narrowed;
}

} // namespace MR::DWI::Tractography::Compression
