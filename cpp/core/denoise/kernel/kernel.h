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
#include <memory>
#include <string>
#include <vector>

#include "app.h"
#include "header.h"
#include "types.h"

namespace MR::Denoise::Kernel {

class Base;

extern const char *const shape_description;
extern const char *const default_size_description;
extern const char *const cuboid_size_description;

const std::vector<std::string> shapes = {"cuboid", "sphere"};
enum class shape_type { CUBOID, SPHERE };
extern const App::OptionGroup options;

std::shared_ptr<Base> make_kernel(const Header &H,                                 //
                                  const std::array<ssize_t, 3> &subsample_factors, //
                                  const default_type size_multiplier);             //

} // namespace MR::Denoise::Kernel
