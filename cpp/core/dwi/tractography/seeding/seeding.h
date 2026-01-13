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

#include <unordered_map>

#include "mrtrix.h"

namespace MR::App {
class OptionGroup;
}

namespace MR::DWI::Tractography {
class Properties;
}

namespace MR::DWI::Tractography::Seeding {

// These constants set how many times a tracking algorithm should attempt to propagate
//   from a given seed point, based on the mechanism used to provide the seed point
enum class seed_attempt_t { RANDOM, DYNAMIC, GMWMI, FIXED };

extern const std::unordered_map<seed_attempt_t, ssize_t> attempts_per_seed;

extern const App::OptionGroup SeedMechanismOption;
extern const App::OptionGroup SeedParameterOption;
void load_seed_mechanisms(Properties &);
void load_seed_parameters(Properties &);

} // namespace MR::DWI::Tractography::Seeding
