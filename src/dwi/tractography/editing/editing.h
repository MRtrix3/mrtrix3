/* Copyright (c) 2008-2025 the MRtrix3 contributors.
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

#ifndef __dwi_tractography_editing_editing_h__
#define __dwi_tractography_editing_editing_h__

#include "app.h"

#include "dwi/tractography/properties.h"

namespace MR {
namespace DWI {
namespace Tractography {
namespace Editing {



extern const App::OptionGroup LengthOption;
extern const App::OptionGroup TruncateOption;
extern const App::OptionGroup WeightsOption;


void load_properties (Tractography::Properties&);



}
}
}
}

#endif
