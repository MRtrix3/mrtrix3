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

    GraphTheory();
    virtual ~GraphTheory();

    void exclude( matrix_type&, const node_t& node ) const;
    void exclude( matrix_type&, const std::set< node_t >& nodes ) const;
    void zero_diagonal( matrix_type& ) const;
    void symmetrise( matrix_type& ) const;

    matrix_type weight_to_max_scaled( const matrix_type& ) const;
    matrix_type weight_to_length( const matrix_type& ) const;
    matrix_type weight_to_distance( const matrix_type& ) const;

    metric_type strength( const matrix_type& ) const;
    metric_type betweenness( const matrix_type& ) const;
    metric_type clustering_coefficient( const matrix_type& ) const;
    metric_type characteristic_path_length( const matrix_type& ) const;
    metric_type local_efficiency( const matrix_type& ) const;

    double global_efficiency( const matrix_type& ) const;
    double vulnerability( const matrix_type& ) const;
    double small_worldness( const matrix_type& ) const;

    void write_matrix( const matrix_type&, const std::string& path ) const;

    void print_global( const matrix_type& ) const;

  private:

    matrix_type length_to_distance( const matrix_type& cm_length ) const;

    bool nonzero( const double& value ) const;
    std::vector< uint32_t > nonzero_indices( const Eigen::RowVectorXd& vec ) const;
    std::vector< uint32_t > equal_indices( const Eigen::RowVectorXd& vec, const double& value ) const;

};


}

}


#endif

