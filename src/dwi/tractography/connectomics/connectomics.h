/*
    Copyright 2012 Brain Research Institute, Melbourne, Australia

    Written by Robert Smith, 2012.

    This file is part of MRtrix.

    MRtrix is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    MRtrix is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with MRtrix.  If not, see <http://www.gnu.org/licenses/>.

 */



#ifndef __dwi_tractography_connectomics_connectomics_h__
#define __dwi_tractography_connectomics_connectomics_h__


#include "app.h"
#include "args.h"

#include "image/buffer.h"



namespace MR {
namespace DWI {
namespace Tractography {
namespace Connectomics {



#define TCK2NODES_RADIAL_DEFAULT_DIST 2.0
#define TCK2NODES_REVSEARCH_DEFAULT_DIST 0.0 // Default = no distance limit, reverse search all the way to the streamline midpoint



typedef uint32_t node_t;

class Tck2nodes_base;
class Metric_base;



extern const char* metrics[];
extern const char* modes[];



extern const App::OptionGroup AssignmentOption;
Tck2nodes_base* load_assignment_mode (Image::Buffer<node_t>&);

extern const App::OptionGroup MetricOption;
Metric_base* load_metric (Image::Buffer<node_t>&);



}
}
}
}


#endif

