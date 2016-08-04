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
#include "exception.h"
#include <iomanip>


namespace MR {
namespace Connectome {


#define EPSILON std::numeric_limits< double >::epsilon()


GraphTheory::GraphTheory()
{
}


GraphTheory::~GraphTheory()
{
}


void GraphTheory::exclude( matrix_type& cm, const node_t& node ) const
{
  node_t num_rows = cm.rows();
  node_t num_cols = cm.cols();
  cm.block( node  , 0, num_rows-node  , num_cols ) =
  cm.block( node+1, 0, num_rows-node-1, num_cols );
  cm.conservativeResize( num_rows-1, num_cols );

  num_rows = cm.rows();
  cm.block( 0, node  , num_rows, cm.cols()-node   ) =
  cm.block( 0, node+1, num_rows, cm.cols()-node-1 );
  cm.conservativeResize( num_rows, num_cols-1 );
}


void GraphTheory::exclude( matrix_type& cm, const std::set< node_t >& nodes ) const
{
  for ( auto n = nodes.crbegin(); n != nodes.crend() ; ++n )
  {
    exclude( cm, *n );
  }
}


void GraphTheory::zero_diagonal( matrix_type& cm ) const
{
  cm.diagonal() = Eigen::MatrixXd::Zero( cm.rows(), 1 );
}


void GraphTheory::symmetrise( matrix_type& cm ) const
{
  Eigen::MatrixXd upper = cm.triangularView< Eigen::Upper >();
  cm = upper + upper.transpose() -
       upper.cwiseProduct( Eigen::MatrixXd::Identity( cm.rows(), cm.cols() ) );
}


matrix_type GraphTheory::weight_to_max_scaled( const matrix_type& cm ) const
{
  /* convert a connectivity matrix to a max-normalised matrix */
  double maxValue = cm.maxCoeff();
  if ( !nonzero( maxValue ) )
  {
    throw Exception( "Maximum matrix entry is zero." );
  }
  return cm / maxValue;
}


matrix_type GraphTheory::weight_to_length( const matrix_type& cm ) const
{
  /* convert a connectivity matrix to a length matrix */
  node_t num_nodes = cm.rows();
  matrix_type cm_length( num_nodes, num_nodes );
  for ( node_t r = 0; r < num_nodes; r++ )
  {
    for ( node_t c = 0; c < num_nodes; c++ )
    {
      double value = cm( r, c );
      cm_length( r, c ) = nonzero( value ) ? ( 1.0 / value ) : 0.0;
    }
  }
  return cm_length;
}


matrix_type GraphTheory::weight_to_distance( const matrix_type& cm ) const
{
  /* convert a connectivity matrix to a distance matrix */
  return length_to_distance( weight_to_length( cm ) );
}


matrix_type GraphTheory::length_to_distance( const matrix_type& cm_length ) const
{
  /* convert a length matrix to a distance matrix */
  // initialise a matrix with zero diagonal & Inf for pairwise connections
  node_t num_nodes = cm_length.rows();
  matrix_type cm_distance = Eigen::MatrixXd::Ones( num_nodes, num_nodes ) * Inf;
  cm_distance.diagonal() = Eigen::MatrixXd::Zero( num_nodes, 1 );
  for ( node_t n = 0; n < num_nodes; n++ )
  {
    std::vector< bool > S( num_nodes, true );
    matrix_type L = cm_length;
    node_t V = n;
    while ( 1 )
    {
      S[ V ] = false;
      L.col( V ).fill( 0 );
      Eigen::RowVectorXd vec = L.row( V );
      auto nonzeros = nonzero_indices( vec );
      for ( size_t index = 0; index < nonzeros.size(); index++ )
      {
        double d = std::min( cm_distance( n, nonzeros[ index ] ),
                             cm_distance( n, V ) + L( V, nonzeros[ index ] ) );
        cm_distance( n, nonzeros[ index ] ) = d;
      }
      double min_distance = Inf;
      for ( node_t u = 0; u < num_nodes; u++ )
      {
        if ( S[ u ] )
        {
          min_distance = std::min( min_distance, cm_distance( n, u ) );
        }
      }
      if ( std::isinf( min_distance ) )
      {
        break;
      }
      auto equals = equal_indices( cm_distance.row( n ), min_distance );
      V = equals[ 0 ];
    }
  }
  return cm_distance;
}


metric_type GraphTheory::strength( const matrix_type& cm ) const
{
  return cm.rowwise().sum();
}


metric_type GraphTheory::clustering_coefficient( const matrix_type& cm ) const
{
  // Method from Zhang et al., Stat Appl Genet Mol Biol 2005, 4(1), 17
  node_t num_nodes = cm.rows();
  matrix_type cm_max_scaled = weight_to_max_scaled( cm );
  metric_type numerator = ( cm_max_scaled *
                            cm_max_scaled *
                            cm_max_scaled ).diagonal();
  metric_type denominator = Eigen::MatrixXd::Zero( num_nodes, 1 );
  for ( node_t n = 0; n != num_nodes; n++ )
  {
    for ( node_t j = 0; j != num_nodes; j++ )
    {
      for ( node_t k = 0; k != num_nodes; k++ )
      {
        if ( j != n && k != n && j != k )
        {
          denominator( n ) += cm_max_scaled( n, j ) * cm_max_scaled( n, k );
        }
      }
    }
  }
  return numerator.cwiseQuotient( denominator );
}


metric_type GraphTheory::characteristic_path_length( const matrix_type& cm ) const
{
  return weight_to_distance( cm ).rowwise().sum() / ( cm.rows()-1 );
}


metric_type GraphTheory::local_efficiency( const matrix_type& cm ) const
{
  matrix_type W = weight_to_max_scaled( cm );
  matrix_type L = weight_to_length( W );

  node_t num_nodes = cm.rows();
  metric_type Elocal = Eigen::MatrixXd::Zero( num_nodes, 1 );
  for ( node_t n = 0; n < num_nodes; n++ )
  {
    auto nonzeros = nonzero_indices( W.row( n ) );
    metric_type s_Wn( nonzeros.size(), 1 );
    for ( auto nz = 0; nz < nonzeros.size(); nz++ )
    {
      s_Wn( nz ) = 2.0 * std::pow( W( n, nonzeros[ nz ] ), 1.0 / 3.0 );
    }
    matrix_type L_sub = L;
    auto zeros = equal_indices( L_sub.row( n ), 0.0 );
    exclude( L_sub, std::set< node_t >( zeros.begin(), zeros.end() ) );
    matrix_type D_inv_sub = length_to_distance( L_sub ).cwiseInverse();
    D_inv_sub.diagonal() = Eigen::MatrixXd::Zero( D_inv_sub.rows(), 1 );
    matrix_type s_D_inv_sub = 2.0 * D_inv_sub.array().pow( 1.0 / 3.0 );

    double numerator = 0.5 * ( s_Wn * s_Wn.transpose() ).cwiseProduct( s_D_inv_sub ).sum();
    if ( numerator )
    {
      auto s_A = 2.0 * Eigen::MatrixXd::Ones( 1, nonzeros.size() );
      double denominator = s_A.sum() * s_A.sum() - s_A.array().square().sum();
      Elocal( n ) = numerator / denominator;
    }
  }  
  return Elocal;
}


double GraphTheory::global_efficiency( const matrix_type& cm ) const
{
  matrix_type cm_distance_inv = weight_to_distance( weight_to_max_scaled( cm )
                                                   ).cwiseInverse();
  node_t num_nodes = cm.cols();
  cm_distance_inv.diagonal() = Eigen::MatrixXd::Zero( num_nodes, 1 );
  return cm_distance_inv.sum() / ( num_nodes * ( num_nodes - 1 ) );
}


double GraphTheory::vulnerability( const matrix_type& cm ) const
{
  // Method from Latora et al., Phys. Rev., EStat. Nonlinear Soft Matter Phys. 71
  node_t num_nodes = cm.rows();
  double e_g = global_efficiency( cm );
  metric_type node_vulnerability = Eigen::MatrixXd::Zero( num_nodes, 1 );
  for ( node_t n = 0; n < num_nodes; n++ )
  {
    matrix_type cm_sub = cm;
    exclude( cm_sub, n );
    node_vulnerability( n ) = ( e_g - global_efficiency( cm_sub ) ) / e_g;
  }
  return node_vulnerability.maxCoeff();
}


void GraphTheory::write_matrix( const matrix_type& cm, const std::string& path ) const
{
  std::ofstream file( path );
  for ( node_t r = 0; r != cm.rows(); r++ )
  {
    for ( node_t c = 0; c != cm.cols(); c++ )
    {
      file << cm( r, c ) << " " << std::flush;
    }
    file << '\n';
  }
}


void GraphTheory::print_global( const matrix_type& cm ) const
{
  Eigen::MatrixXd global_metrics = Eigen::MatrixXd::Zero( 1, 6 );
  global_metrics( 0 ) = strength( cm ).mean();
  global_metrics( 1 ) = clustering_coefficient( cm ).mean();
  global_metrics( 2 ) = characteristic_path_length( cm ).mean();
  global_metrics( 3 ) = local_efficiency( cm ).mean();
  global_metrics( 4 ) = global_efficiency( cm );
  global_metrics( 5 ) = vulnerability( cm );
  std::cout << std::setw( 12 ) << std::right << "Kw"
            << std::setw( 12 ) << std::right << "Cw"
            << std::setw( 12 ) << std::right << "Lw"
            << std::setw( 12 ) << std::right << "Ew-l"
            << std::setw( 12 ) << std::right << "Ew-g"
            << std::setw( 12 ) << std::right << "Vw" << std::endl;
  std::cout << " " << global_metrics << std::endl;
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

