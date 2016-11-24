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



#ifndef __dwi_tractography_connectome_exemplar_h__
#define __dwi_tractography_connectome_exemplar_h__

#include <mutex>

#include "dwi/tractography/streamline.h"
#include "dwi/tractography/connectome/connectome.h"
#include "dwi/tractography/connectome/streamline.h"


namespace MR {
namespace DWI {
namespace Tractography {
namespace Connectome {




class Exemplar : private Tractography::Streamline<float>
{ MEMALIGN(Exemplar)
  public:
    using Tractography::Streamline<float>::point_type;
    Exemplar (const size_t length, const NodePair& nodes, const std::pair<point_type, point_type>& COMs) :
        Tractography::Streamline<float> (length, { 0.0f, 0.0f, 0.0f }),
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

    void add (const Connectome::Streamline_nodepair&);
    void add (const Connectome::Streamline_nodelist&);
    void finalize (const float);

    const Tractography::Streamline<float>& get() const { assert (is_finalized); return *this; }

    bool is_diagonal() const { return nodes.first == nodes.second; }

    float get_weight() const { return weight; }

  private:
    std::mutex mutex;
    NodePair nodes;
    std::pair<point_type, point_type> node_COMs;
    bool is_finalized;

    void add (const Tractography::Streamline<float>&, const bool is_reversed);
};




}
}
}
}


#endif

