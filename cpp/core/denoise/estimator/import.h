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

#include "denoise/denoise.h"
#include "denoise/estimator/base.h"
#include "denoise/estimator/result.h"
#include "image.h"
#include "interp/cubic.h"

namespace MR::Denoise::Estimator {

class Import : public Base {
public:
  Import(const std::string &path, Image<float> &vst_image) //
      : noise_image(Image<float>::open(path)),             //
        vst_image(vst_image) {}                            //
  void update_vst_image(Image<float> &new_vst_image) override { vst_image = new_vst_image; }
  Result operator()(const Eigen::VectorBlock<eigenvalues_type> s, //
                    const ssize_t m,                              //
                    const ssize_t n,                              //
                    const ssize_t rp,                             //
                    const Eigen::Vector3d &pos) const final {     //
    assert(s.size() == std::min(m, n));
    const ssize_t qnz = dimlong_nonzero(m, n, rp);
    const ssize_t rz = rank_zero(m, n, rp);
    const ssize_t rnz = rank_nonzero(m, n, rp);
    Result result;
    {
      // Construct on each call to preserve const-ness & thread-safety
      Interp::Cubic<Image<float>> interp(noise_image);
      // TODO This will cause issues at the edge of the image FoV
      // Addressing this may require integration of the mrfilter changes
      //   that provide wrappers for various handling of FoV edges
      // For now, just expect that denoising won't do anything
      //   where the patch centre is too close to the image edge for cubic interpolation
      if (!interp.scanner(pos))
        return result;
      // If the data have been preconditioned at input based on a pre-estimated noise level,
      //   then we need to rescale the threshold that we load from this image
      //   based on knowledge of that rescaling
      if (vst_image.valid()) {
        Interp::Cubic<Image<float>> vst_interp(vst_image);
        if (!vst_interp.scanner(pos))
          return result;
        result.sigma2 = Math::pow2(interp.value() / vst_interp.value());
      } else {
        result.sigma2 = Math::pow2(interp.value());
      }
    }
    // From this noise level,
    //   get the upper bound of the MP distribution and rank of signal
    //   given the ordered list of eigenvalues
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
  Image<float> noise_image;
  Image<float> vst_image;
};

} // namespace MR::Denoise::Estimator
