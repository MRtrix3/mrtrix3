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

#include <array>
#include <vector>

#include "algo/threaded_loop.h"
#include "denoise/estimate.h"
#include "denoise/estimator/estimator.h"
#include "denoise/exports.h"
#include "denoise/kernel/kernel.h"
#include "denoise/precondition.h"
#include "denoise/subsample.h"
#include "filter/smooth.h"
#include "image.h"
#include "interp/linear.h"
#include "types.h"

namespace MR::Denoise::Iterative {

struct Iteration {
  std::array<ssize_t, 3> subsample_ratios;
  default_type kernel_size_multiplier;
  bool smooth_noiseout;
};

// Internal function covering as much as possible for iterative implementation
template <typename T>
void estimate(Image<T> &input,
              Image<T> &input_preconditioned,
              Image<bool> &mask,
              Image<float> &vst_image,
              const Iteration &config,
              const ssize_t iter,
              std::shared_ptr<Subsample> subsample,
              std::shared_ptr<Estimator::Base> estimator,
              const Precondition<T> &preconditioner,
              Exports &exports) {
  auto kernel = Kernel::make_kernel(input, subsample->get_factors(), config.kernel_size_multiplier);
  kernel->set_mask(mask);
  if (preconditioner.noop())
    input_preconditioned = input;
  else
    preconditioner(input, input_preconditioned, false);
  Estimate<T> func(input_preconditioned, subsample, kernel, estimator, exports, preconditioner.null_rank(), false);
  ThreadedLoop("MPPCA noise level estimation", input_preconditioned, 0, 3).run(func, input_preconditioned);
  // If a VST was applied to the input data for this iteration,
  //   need to remove its effect from the estimated noise map
  if (vst_image.valid()) {
    Interp::Linear<Image<float>> vst_interp(vst_image);
    const Transform transform(subsample->header());
    for (auto l = Loop(exports.noise_out)(exports.noise_out); l; ++l) {
      vst_interp.scanner(transform.voxel2scanner * Eigen::Vector3d({double(exports.noise_out.index(0)),
                                                                    double(exports.noise_out.index(1)),
                                                                    double(exports.noise_out.index(2))}));
      exports.noise_out.value() *= vst_interp.value();
    }
  }
  // TODO Ideally include more complex processing on this image;
  //   eg. outlier detection & infilling
  if (config.smooth_noiseout) {
    assert(exports.noise_out.valid());
    Filter::Smooth smooth_filter(exports.noise_out);
    smooth_filter(exports.noise_out);
  }
}

} // namespace MR::Denoise::Iterative
