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



#ifndef __dwi_tractography_connectome_streamline_h__
#define __dwi_tractography_connectome_streamline_h__

#include "dwi/tractography/streamline.h"
#include "dwi/tractography/connectome/connectome.h"


namespace MR {
namespace DWI {
namespace Tractography {
namespace Connectome {






class Streamline : public Tractography::Streamline<float>
{
  public:
    Streamline() : Tractography::Streamline<float>(), nodes (std::make_pair (0, 0)) { }
    Streamline (const size_t i) : Tractography::Streamline<float> (i), nodes (std::make_pair (0, 0)) { }

    void set_nodes (const NodePair& i) { nodes = i; }
    const NodePair& get_nodes() const { return nodes; }

  private:
    NodePair nodes;
};





}
}
}
}


#endif

