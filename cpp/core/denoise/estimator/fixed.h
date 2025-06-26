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

#include "denoise/denoise.h"
#include "denoise/estimator/base.h"
#include "denoise/estimator/result.h"

#include "image.h"
#include "interp/cubic.h"

namespace MR::Denoise::Estimator {

class Fixed : public Base {
public:
  Fixed(const default_type value, Image<float> &vst_noise_in) //
      : sigma2(Math::pow2(value)),                            //
        vst_image(vst_noise_in) {}                            //
  void update_vst_image(Image<float> &new_vst_image) override { vst_image = new_vst_image; }
  Result operator()(const Eigen::VectorBlock<eigenvalues_type> s, //
                    const ssize_t m,                              //
                    const ssize_t n,                              //
                    const ssize_t rp,                             //
                    const Eigen::Vector3d &pos) const final {     //
    assert(s.size() == r);
    const ssize_t qnz = dimlong_nonzero(m, n, rp);
    const ssize_t rz = rank_zero(m, n, rp);
    const ssize_t rnz = rank_nonzero(m, n, rp);
    Result result;
    // If the data have been preconditioned at input based on a pre-estimated noise level,
    //   then we need to rescale the threshold that we load from this image
    //   based on knowledge of that rescaling
    if (vst_image.valid()) {
      Interp::Cubic<Image<float>> vst_interp(vst_image);
      if (!vst_interp.scanner(pos))
        return result;
      result.sigma2 = sigma2 / Math::pow2(vst_interp.value());
    } else {
      result.sigma2 = sigma2;
    }
    // From this noise level,
    //   get the upper bound of the MP distribution and rank of signal
    //   given the ordered list of eigenvalues
    // Bear in mind: "cutoff_p" is the *number* of sub-threshold eigenvalues
    result.lamplus = Math::pow2(1.0 + std::sqrt(double(rnz) / double(qnz))) * result.sigma2;
    result.cutoff_p = rz;
    for (ssize_t p = rz; p != s.size(); ++p) {
      if (s[p] / qnz > result.lamplus)
        break;
      result.cutoff_p = p + 1;
    }
    return result;
  }

private:
  const default_type sigma2;
  Image<float> vst_image;
};

} // namespace MR::Denoise::Estimator
