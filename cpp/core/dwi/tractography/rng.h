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

#include "math/rng.h"

namespace MR::DWI::Tractography {

//! Thread-local, but globally accessible RNG to vastly simplify multi-threading.
/*! Implemented as a Meyers-style accessor returning a reference to a
 *  function-local \c thread_local instance, lazily constructed on first use.
 *  This avoids a namespace-scope \c thread_local definition,
 *  which triggers compilation failures under GCC 16 on Windows
 *  due to changes in thread-local storage (TLS) handling. */
Math::RNG &rng();

} // namespace MR::DWI::Tractography
