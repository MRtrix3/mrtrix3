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

#include <memory>
#include <string>
#include <vector>

#include "app.h"
#include "image.h"

namespace MR::Denoise::Estimator {

class Base;

extern const App::Option estimator_option;
extern const App::OptionGroup estimator_denoise_options;
const std::vector<std::string> estimators = {"exp1", "exp2", "med", "mrm2023"};
enum class estimator_type { EXP1, EXP2, MED, MRM2023 };
std::shared_ptr<Base> make_estimator(Image<float> &vst_noise_in, const bool permit_bypass);

} // namespace MR::Denoise::Estimator
