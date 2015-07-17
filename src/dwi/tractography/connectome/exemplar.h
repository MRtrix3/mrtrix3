/*
    Copyright 2013 Brain Research Institute, Melbourne, Australia

    Written by Robert Smith, 2015.

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



#ifndef __dwi_tractography_connectome_exemplar_h__
#define __dwi_tractography_connectome_exemplar_h__

#include <mutex>

#include "point.h"

#include "dwi/tractography/streamline.h"
#include "dwi/tractography/connectome/connectome.h"
#include "dwi/tractography/connectome/streamline.h"


namespace MR {
namespace DWI {
namespace Tractography {
namespace Connectome {




class Exemplar : private Tractography::Streamline<float>
{
  public:
    Exemplar (const size_t length, const NodePair& nodes, const std::pair< Point<float>, Point<float> >& COMs) :
        Tractography::Streamline<float> (length, Point<float> (0.0f, 0.0f, 0.0f)),
        nodes (nodes),
        node_COMs (COMs),
        is_finalized (false)
    {
      weight = 0.0f;
    }

    Exemplar (Exemplar&& that) :
        Tractography::Streamline<float> (std::move (that)),
        mutex (),
        nodes (that.nodes),
        node_COMs (that.node_COMs),
        is_finalized (that.is_finalized) { }

    Exemplar (const Exemplar& that) :
        Tractography::Streamline<float> (that),
        mutex (),
        nodes (that.nodes),
        node_COMs (that.node_COMs),
        is_finalized (that.is_finalized) { }

    Exemplar& operator= (const Exemplar& that);

    void add (const Connectome::Streamline& in);
    void finalize (const float);

    const Tractography::Streamline<float>& get() const { assert (is_finalized); return *this; }

    bool is_diagonal() const { return nodes.first == nodes.second; }

    float get_weight() const { return weight; }

  private:
    std::mutex mutex;
    NodePair nodes;
    std::pair< Point<float>, Point<float> > node_COMs;
    bool is_finalized;
};




}
}
}
}


#endif

