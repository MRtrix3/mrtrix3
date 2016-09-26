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



#ifndef __dwi_tractography_connectome_matrix_h__
#define __dwi_tractography_connectome_matrix_h__

#include <set>
#include <vector>

#include "connectome/connectome.h"
#include "math/math.h"

#include "dwi/tractography/connectome/connectome.h"
#include "dwi/tractography/connectome/mapped_track.h"


namespace MR {
namespace DWI {
namespace Tractography {
namespace Connectome {



enum stat_edge { SUM, MEAN, MIN, MAX };
extern const char* statistics[];
extern const App::Option EdgeStatisticOption;




class Matrix
{

  public:
    Matrix (const node_t max_node_index, const stat_edge stat, const bool vector_output = false) :
        data   (MR::Connectome::matrix_type::Zero (vector_output ? 1 : (max_node_index + 1), max_node_index + 1)),
        counts (MR::Connectome::matrix_type::Zero (vector_output ? 1 : (max_node_index + 1), max_node_index + 1)),
        statistic (stat)
    {
      if (statistic == stat_edge::MIN)
        data = MR::Connectome::matrix_type::Constant (vector_output ? 1 : (max_node_index + 1), max_node_index + 1, std::numeric_limits<default_type>::infinity());
      else if (statistic == stat_edge::MAX)
        data = MR::Connectome::matrix_type::Constant (vector_output ? 1 : (max_node_index + 1), max_node_index + 1, -std::numeric_limits<default_type>::infinity());
    }

    bool operator() (const Mapped_track_nodepair&);
    bool operator() (const Mapped_track_nodelist&);

    void finalize();
    void remove_unassigned();

    void error_check (const std::set<node_t>&);

    void write_assignments (const std::string&) const;

    bool is_vector() const { return (data.rows() == 1); }

    const MR::Connectome::matrix_type get() const { return data; }


  private:
    MR::Connectome::matrix_type data, counts;
    const stat_edge statistic;
    std::vector<NodePair> assignments_pairs;
    std::vector< std::vector<node_t> > assignments_lists;

    void apply (double&, const double, const double);

};






}
}
}
}


#endif

