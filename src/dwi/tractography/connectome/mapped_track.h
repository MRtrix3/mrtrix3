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



#ifndef __dwi_tractography_connectome_mapped_track_h__
#define __dwi_tractography_connectome_mapped_track_h__

#include "dwi/tractography/connectome/connectome.h"


namespace MR {
namespace DWI {
namespace Tractography {
namespace Connectome {



class Mapped_track
{

  public:
    Mapped_track() :
      nodes (std::make_pair (0, 0)),
      factor (0.0),
      weight (1.0) { }

    void set_track_index (const size_t i) { track_index = i; }
    void set_first_node  (const node_t i) { nodes.first = i;  }
    void set_second_node (const node_t i) { nodes.second = i; }
    void set_nodes       (const NodePair& i) { nodes = i; }
    void set_factor      (const float i)    { factor = i; }
    void set_weight      (const float i)    { weight = i; }

    size_t get_track_index() const { return track_index; }
    node_t get_first_node()  const { return nodes.first;  }
    node_t get_second_node() const { return nodes.second; }
    const NodePair& get_nodes() const { return nodes; }
    float get_factor() const { return factor; }
    float get_weight() const { return weight; }

  private:
    size_t track_index;
    NodePair nodes;
    float factor, weight;

};




}
}
}
}


#endif

