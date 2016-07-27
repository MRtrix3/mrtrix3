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


#include "connectome/graph_theory.h"
#include "file/ofstream.h"


namespace MR {
namespace Connectome {


GraphTheory::GraphTheory( const matrix_type& cm )
            : _cm( cm )
{
  _num_nodes = _cm.cols();
}


GraphTheory::~GraphTheory()
{
}


void GraphTheory::exclude( std::set< node_t > nodes )
{
  for ( auto n = nodes.crbegin(); n != nodes.crend() ; n++ )
  {
    // remove one row
    _cm.block( (*n)  , 0, _num_nodes-(*n), _num_nodes ) =
    _cm.block( (*n)+1, 0, _num_nodes-(*n), _num_nodes );
    _cm.conservativeResize( _num_nodes-1, _num_nodes );

    // remove one column
    _cm.block( 0, (*n)  , _num_nodes, _num_nodes-(*n) ) =
    _cm.block( 0, (*n)+1, _num_nodes, _num_nodes-(*n) );
    _cm.conservativeResize( _num_nodes, _num_nodes-1 );

    // update node count
    -- _num_nodes;
  }
}


void GraphTheory::zero_diagonal()
{
  _cm.diagonal() = Eigen::MatrixXd::Zero( _num_nodes, 1 );
}


void GraphTheory::symmetrise()
{
  Eigen::MatrixXd upper = _cm.triangularView< Eigen::Upper >();
  _cm = upper + upper.transpose() -
        upper.cwiseProduct( Eigen::MatrixXd::Identity(_num_nodes,_num_nodes) );
}


void GraphTheory::weight_conversion()
{
  _cm_length.resize( _num_nodes, _num_nodes );
  _cm_max_scaled.resize( _num_nodes, _num_nodes );
  float maxValue = _cm.maxCoeff();
  for ( node_t r = 0; r < _num_nodes; r++ )
  {
    for ( node_t c = 0; c < _num_nodes; c++ )
    {
      float value = _cm( r, c );
      if ( std::abs( value ) < std::numeric_limits< float >::epsilon() )
      {
        _cm_length( r, c ) = _cm_max_scaled( r, c ) = 0.0;
      }
      else
      {
        _cm_length( r, c ) = 1.0 / value;
        _cm_max_scaled( r, c ) = value / maxValue;
      }
    }
  }
}


metric_type GraphTheory::strength() const
{
  return _cm.rowwise().sum();
}


void GraphTheory::write_matrix( const std::string& path ) const
{
  std::ofstream file( path );
  for ( node_t r = 0; r != _num_nodes; r++ )
  {
    for ( node_t c = 0; c != _num_nodes; c++ )
    {
      file << _cm( r, c ) << " " << std::flush;
    }
    file << '\n';
  }
}


void GraphTheory::print_global() const
{
  Eigen::MatrixXf global_metrics = Eigen::MatrixXf::Zero( 1, 1 );
  global_metrics( 0, 0 ) = strength().mean();
  std::cout << global_metrics << std::endl;
}


}
}

