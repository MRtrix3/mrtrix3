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



#ifndef __dwi_tractography_connectome_streamline_h__
#define __dwi_tractography_connectome_streamline_h__

#include "dwi/tractography/streamline.h"
#include "dwi/tractography/connectome/connectome.h"


namespace MR {
namespace DWI {
namespace Tractography {
namespace Connectome {






class Streamline_nodepair : public Tractography::Streamline<>
{ MEMALIGN(Streamline_nodepair)
  public:
    Streamline_nodepair() : Tractography::Streamline<>(), nodes (std::make_pair (0, 0)) { }
    Streamline_nodepair (const size_t i) : Tractography::Streamline<> (i), nodes (std::make_pair (0, 0)) { }

    void set_nodes (const NodePair& i) { nodes = i; }
    const NodePair& get_nodes() const { return nodes; }

  private:
    NodePair nodes;
};



class Streamline_nodelist : public Tractography::Streamline<>
{ MEMALIGN(Streamline_nodelist)
  public:
    Streamline_nodelist() : Tractography::Streamline<>(), nodes () { }
    Streamline_nodelist (const size_t i) : Tractography::Streamline<> (i), nodes () { }

    void set_nodes (const vector<node_t>& i) { nodes = i; }
    const vector<node_t>& get_nodes() const { return nodes; }

  private:
    vector<node_t> nodes;
};





}
}
}
}


#endif

