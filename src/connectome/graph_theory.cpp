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
#include "connectome/mat2vec.h"
#include "file/ofstream.h"
#include "exception.h"


namespace MR {
namespace Connectome {


#define EPSILON std::numeric_limits< double >::epsilon()


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
  /*
   * convert a connectivity matrix to a length matrix
   */
  _cm_length.resize( _num_nodes, _num_nodes );
  for ( node_t r = 0; r < _num_nodes; r++ )
  {
    for ( node_t c = 0; c < _num_nodes; c++ )
    {
      double value = _cm( r, c );
      _cm_length( r, c ) = nonzero( value ) ? ( 1.0 / value ) : 0.0;
    }
  }

  /*
   * convert a connectivity matrix to a max-normalised matrix
   */
  double maxValue = _cm.maxCoeff();
  if ( !nonzero( maxValue ) )
  {
    throw Exception( "Maximum matrix entry is zero." );
  }
  _cm_max_scaled = _cm / _cm.maxCoeff();

  /*
   * convert a length matrix to a distance matrix
   */
  // initialise a matrix with zero diagonal & Inf for pairwise connections
  _cm_distance = Eigen::MatrixXd::Ones( _num_nodes, _num_nodes ) * Inf;
  _cm_distance.diagonal() = Eigen::MatrixXd::Zero( _num_nodes, 1 );
  for ( node_t u = 0; u < _num_nodes; u++ )
  {
    std::vector< bool > S( _num_nodes, true );
    matrix_type L = _cm_length;
    node_t V = u;
    while ( 1 )
    {
      S[ V ] = false;
      L.col( V ).fill( 0 );
      Eigen::RowVectorXd vec = L.row( V );
      auto nonzeros = nonzero_indices( vec );
      for ( size_t index = 0; index < nonzeros.size(); index++ )
      {
        double d = std::min( _cm_distance( u, nonzeros[ index ] ),
                             _cm_distance( u, V ) + L( V, nonzeros[ index ] ) );
        _cm_distance( u, nonzeros[ index ] ) = d;
      }
      double min_distance = Inf;
      for ( node_t n = 0; n < _num_nodes; n++ )
      {
        if ( S[ n ] )
        {
          min_distance = std::min( min_distance, _cm_distance( u, n ) );
        }
      }
      if ( std::isinf( min_distance ) )
      {
        break;
      }
      auto equals = equal_indices( _cm_distance.row( u ), min_distance );
      V = equals[ 0 ];
    }
  }
}


metric_type GraphTheory::strength() const
{
  return _cm.rowwise().sum();
}


metric_type GraphTheory::clustering_coefficient() const
{
  // Method from Zhang et al., Stat Appl Genet Mol Biol 2005, 4(1), 17
  metric_type numerator = ( _cm_max_scaled *
                            _cm_max_scaled *
                            _cm_max_scaled ).diagonal();
  metric_type denominator = Eigen::MatrixXd::Zero( _num_nodes, 1 );
  for ( node_t n = 0; n != _num_nodes; n++ )
  {
    for ( node_t j = 0; j != _num_nodes; j++ )
    {
      for ( node_t k = 0; k != _num_nodes; k++ )
      {
        if ( j != n && k != n && j != k )
        {
          denominator( n ) += _cm_max_scaled( n, j ) * _cm_max_scaled( n, k );
        }
      }
    }
  }
  return numerator.cwiseQuotient( denominator );
}


metric_type GraphTheory::characteristic_path_length() const
{
  return _cm_distance.rowwise().sum() / ( _num_nodes-1 );
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
  Eigen::MatrixXd global_metrics = Eigen::MatrixXd::Zero( 1, 3 );
  global_metrics( 0 ) = strength().mean();
  global_metrics( 1 ) = clustering_coefficient().mean();
  global_metrics( 2 ) = characteristic_path_length().mean();
  std::cout << global_metrics << std::endl;
}


bool GraphTheory::nonzero( const double& value ) const
{
  return std::abs( value ) > EPSILON;
}


std::vector< size_t > GraphTheory::nonzero_indices( const Eigen::RowVectorXd& vec ) const
{
  std::vector< size_t > indices;
  for ( size_t index = 0; index < vec.cols(); index++ )
  {
    if ( nonzero( vec( index ) ) )
    {
      indices.push_back( index );
    }
  }
  return indices;
}


std::vector< size_t > GraphTheory::equal_indices( const Eigen::RowVectorXd& vec,
                                                  const double& value ) const
{
  std::vector< size_t > indices;
  for ( size_t index = 0; index < vec.cols(); index++ )
  {
    if ( !nonzero( vec( index ) - value ) )
    {
      indices.push_back( index );
    }
  }
  return indices;
}


#undef EPSILON


}
}

