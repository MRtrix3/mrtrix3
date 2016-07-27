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



#ifndef __connectome_graph_theory_h__
#define __connectome_graph_theory_h__


#include "connectome/connectome.h"
#include <set>


namespace MR
{

namespace Connectome
{


typedef Eigen::Matrix<default_type, Eigen::Dynamic, 1> metric_type;


class GraphTheory
{

  public:

    GraphTheory( const matrix_type& cm );
    virtual ~GraphTheory();

    void exclude( std::set< node_t > nodes );
    void zero_diagonal();
    void symmetrise();

    void weight_conversion();

    metric_type strength() const;
    metric_type betweenness() const;
    metric_type clustering_coefficient() const;
    metric_type shortest_path_lenth() const;
    metric_type local_efficiency() const;

    float global_efficiency() const;
    float small_worldness() const;

    void write_matrix( const std::string& path ) const;

    void print_global() const;

  private:

    node_t _num_nodes;
    matrix_type _cm;
    matrix_type _cm_length;
    matrix_type _cm_max_scaled;

};


}

}


#endif

