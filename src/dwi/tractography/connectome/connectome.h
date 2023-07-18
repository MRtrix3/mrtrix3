/* Copyright (c) 2008-2023 the MRtrix3 contributors.
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

#ifndef __dwi_tractography_connectome_connectome_h__
#define __dwi_tractography_connectome_connectome_h__


#include "app.h"
#include "image.h"

#include "connectome/connectome.h"



namespace MR {
namespace DWI {
namespace Tractography {
namespace Connectome {



#define TCK2NODES_RADIAL_DEFAULT_DIST 4.0
#define TCK2NODES_REVSEARCH_DEFAULT_DIST 0.0 // Default = no distance limit, reverse search all the way to the streamline midpoint
#define TCK2NODES_FORWARDSEARCH_DEFAULT_DIST 3.0


using node_t = MR::Connectome::node_t;
using NodePair = std::pair<node_t, node_t>;

class Tck2nodes_base;
class Metric;



extern const char* modes[];
extern const App::OptionGroup AssignmentOptions;
Tck2nodes_base* load_assignment_mode (Image<node_t>&);

extern const App::OptionGroup MetricOptions;
void setup_metric (Metric&, Image<node_t>&);



}
}
}
}


#endif

