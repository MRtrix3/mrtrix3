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



#ifndef __dwi_tractography_connectome_matrix_h__
#define __dwi_tractography_connectome_matrix_h__

#include <set>
#include <vector>

#include "math/matrix.h"

#include "dwi/tractography/connectome/connectome.h"
#include "dwi/tractography/connectome/mapped_track.h"


namespace MR {
namespace DWI {
namespace Tractography {
namespace Connectome {




class Matrix
{

  public:
    Matrix (const node_t max_node_index) :
      data   (max_node_index + 1, max_node_index + 1),
      counts (max_node_index + 1, max_node_index + 1)
    {
      data = 0.0;
      counts = 0.0;
    }


    bool operator() (const Mapped_track&);

    void scale_by_streamline_count();
    void remove_unassigned();
    void zero_diagonal();

    void error_check (const std::set<node_t>&);

    void write (const std::string& path) { data.save (path); }
    void write_assignments (const std::string&);

    node_t num_nodes() const { return (data.rows() - 1); }


  private:
    Math::Matrix<double> data, counts;
    std::vector<NodePair> assignments;

};






}
}
}
}


#endif

