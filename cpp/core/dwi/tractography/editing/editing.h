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

#include "app.h"

#include "dwi/tractography/editing/field_filter.h"
#include "dwi/tractography/field_registry.h"
#include "dwi/tractography/properties.h"

namespace MR::DWI::Tractography::Editing {

extern const App::OptionGroup LengthOption;
extern const App::OptionGroup TruncateOption;
extern const App::OptionGroup WeightsOption;
extern const App::OptionGroup FieldFilterOption;

void load_properties(Tractography::Properties &);

//! \brief resolve "-dps_min / -dps_max / -dpv_min / -dpv_max" against \a registry.
/*! Each option names a scalar sidecar field of the input tractogram and a
 * threshold; this looks the field name up in \a registry (the input dataset's
 * field registry, §2.5), validating that the field exists with the matching role
 * (dps for "-dps_*", dpv for "-dpv_*") and is single-column (scalar). The
 * resolved role-local ordinals let the worker index the per-item payload
 * directly. Throws a user-interpretable Exception naming the field if it is
 * absent or not scalar. */
FieldFilters load_field_filters(const FieldRegistry &registry);

} // namespace MR::DWI::Tractography::Editing
