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

#include "denoise/estimator/estimator.h"

#include "denoise/estimator/base.h"
#include "denoise/estimator/exp.h"
#include "denoise/estimator/fixed.h"
#include "denoise/estimator/import.h"
#include "denoise/estimator/med.h"
#include "denoise/estimator/mrm2023.h"
#include "denoise/estimator/rank.h"

namespace MR::Denoise::Estimator {

using namespace App;

// clang-format off
const Option estimator_option =
    Option("estimator",
           "Select the noise level estimator"
           " (default = Exp2),"
           " either: \n"
           "* Exp1: the original estimator used in Veraart et al. (2016); \n"
           "* Exp2: the improved estimator introduced in Cordero-Grande et al. (2019); \n"
           "* Med: estimate based on the median eigenvalue as in Gavish and Donohue (2014); \n"
           "* MRM2023: the alternative estimator introduced in Olesen et al. (2023). \n"
           "Operation will be bypassed if -noise_in or -fixed_rank are specified")
      + Argument("algorithm").type_choice(estimators);

const OptionGroup estimator_denoise_options =
    OptionGroup("Options relating to signal / noise level estimation for denoising")

    + estimator_option

    + Option("noise_in",
             "manually specify the noise level rather than estimating from the data, "
             "whether as a scalar value or as a pre-estimated spatial map")
      + Argument("value/image").type_various()

    + Option("fixed_rank",
             "set a fixed input signal rank rather than estimating the noise level from the data")
      + Argument("value").type_integer(1);

std::shared_ptr<Base> make_estimator(Image<float> &vst_noise_in, const bool permit_bypass) {
  auto opt = get_options("estimator");
  if (permit_bypass) {
    auto noise_in = get_options("noise_in");
    auto fixed_rank = get_options("fixed_rank");
    if (!noise_in.empty()) {
      if (!opt.empty())
        throw Exception("Cannot both provide an input noise level image and specify a noise level estimator");
      if (!fixed_rank.empty())
        throw Exception("Cannot both provide an input noise level image and request a fixed signal rank");
      try {
        return std::make_shared<Fixed>(default_type(noise_in[0][0]), vst_noise_in);
      } catch (Exception &) {
        return std::make_shared<Import>(std::string(noise_in[0][0]), vst_noise_in);
      }
    }
    if (!fixed_rank.empty()) {
      if (!opt.empty())
        throw Exception("Cannot both provide an input signal rank and specify a noise level estimator");
      return std::make_shared<Rank>(fixed_rank[0][0]);
    }
  }
  const estimator_type est = opt.empty() ? estimator_type::EXP2 : estimator_type((int)(opt[0][0]));
  switch (est) {
  case estimator_type::EXP1:
    return std::make_shared<Exp<1>>();
  case estimator_type::EXP2:
    return std::make_shared<Exp<2>>();
  case estimator_type::MED:
    return std::make_shared<Med>();
  case estimator_type::MRM2023:
    return std::make_shared<MRM2023>();
  default:
    assert(false);
  }
  return nullptr;
}

} // namespace MR::Denoise::Estimator
