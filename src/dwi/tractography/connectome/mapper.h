/*
    Copyright 2013 Brain Research Institute, Melbourne, Australia

    Written by Robert Smith, 2013.

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



#ifndef __dwi_tractography_connectome_mapper_h__
#define __dwi_tractography_connectome_mapper_h__

#include "dwi/tractography/streamline.h"
#include "dwi/tractography/connectome/edge_metrics.h"
#include "dwi/tractography/connectome/mapped_track.h"
#include "dwi/tractography/connectome/tck2nodes.h"


namespace MR {
namespace DWI {
namespace Tractography {
namespace Connectome {




class Mapper
{

  public:
    Mapper (Tck2nodes_base& a, const Metric_base& b) :
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
    Tck2nodes_base& tck2nodes;
    const Metric_base& metric;

};




}
}
}
}


#endif

