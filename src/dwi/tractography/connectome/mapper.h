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



#ifndef __dwi_tractography_connectome_mapper_h__
#define __dwi_tractography_connectome_mapper_h__

#include "dwi/tractography/streamline.h"
#include "dwi/tractography/connectome/mapped_track.h"
#include "dwi/tractography/connectome/metric.h"
#include "dwi/tractography/connectome/tck2nodes.h"


namespace MR {
namespace DWI {
namespace Tractography {
namespace Connectome {




class Mapper
{ MEMALIGN(Mapper)

  public:
    Mapper (const Tck2nodes_base& a, const Metric& b) :
      tck2nodes (a),
      metric (b) { }

    Mapper (const Mapper& that) :
      tck2nodes (that.tck2nodes),
      metric (that.metric) { }


    bool operator() (const Tractography::Streamline<float>& in, Mapped_track_nodepair& out)
    {
      assert (tck2nodes.provides_pair());
      out.set_track_index (in.index);
      out.set_nodes (tck2nodes (in));
      out.set_factor (metric (in, out.get_nodes()));
      out.set_weight (in.weight);
      return true;
    }

    bool operator() (const Tractography::Streamline<float>& in, Mapped_track_nodelist& out)
    {
      assert (!tck2nodes.provides_pair());
      out.set_track_index (in.index);
      std::vector<node_t> nodes;
      tck2nodes (in, nodes);
      out.set_nodes (std::move (nodes));
      out.set_factor (metric (in, out.get_nodes()));
      out.set_weight (in.weight);
      return true;
    }


  private:
    const Tck2nodes_base& tck2nodes;
    const Metric& metric;

};




}
}
}
}


#endif

