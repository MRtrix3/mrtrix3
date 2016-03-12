/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 * 
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * For more details, see www.mrtrix.org
 * 
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



#define TCK2NODES_RADIAL_DEFAULT_DIST 2.0
#define TCK2NODES_REVSEARCH_DEFAULT_DIST 0.0 // Default = no distance limit, reverse search all the way to the streamline midpoint
#define TCK2NODES_FORWARDSEARCH_DEFAULT_DIST 3.0


typedef MR::Connectome::node_t node_t;
typedef std::pair<node_t, node_t> NodePair;

class Tck2nodes_base;
class Metric_base;



extern const char* metrics[];
extern const char* modes[];



extern const App::OptionGroup AssignmentOption;
Tck2nodes_base* load_assignment_mode (Image<node_t>&);

extern const App::OptionGroup MetricOption;
Metric_base* load_metric (Image<node_t>&);



}
}
}
}


#endif

