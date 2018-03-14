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


#include "command.h"
#include "connectome/lut.h"
#include "dwi/tractography/file.h"
#include "dwi/tractography/mapping/loader.h"
#include "dwi/tractography/properties.h"
#include "dwi/tractography/weights.h"
#include "dwi/tractography/MACT/scenemodeller.h"
#include "surface/freesurfer.h"
#include "surface/meshfactory.h"


using namespace MR;
using namespace App;

using namespace MR::Connectome;
using namespace MR::DWI;
using namespace MR::DWI::Tractography;
using namespace MR::DWI::Tractography::MACT;
using namespace MR::Surface;


class NodePair
{

  public:

    NodePair() : _weight( 1.0 ) {};

    ~NodePair() {};

    void setFirst( const uint32_t& vertex ) { _nodePair.first = vertex; };
    void setSecond( const uint32_t& vertex ) { _nodePair.second = vertex; };

    uint32_t getFirst() const { return _nodePair.first; };
    uint32_t getSecond() const { return _nodePair.second; };

    void setWeight( const float& weight ) { _weight = weight; };
    float getWeight() const { return _weight; };

  protected:

    std::pair< uint32_t, uint32_t > _nodePair;
    float _weight;

};


class Assigner
{

  public:

    Assigner( const std::shared_ptr< SceneModeller >& sceneModeller )
      : _sceneModeller( sceneModeller ){};

    ~Assigner() {};

    bool operator() ( const Streamline< float >& tck, NodePair& nodePair )
    {
      Eigen::Vector3d first = ( *tck.begin() ).cast< double >();
      Intersection ix1;
      _sceneModeller->nearestTissue( first, ix1 );
      nodePair.setFirst( ix1.nearestVertex() );

      Eigen::Vector3d last = ( *tck.rbegin() ).cast< double >();
      Intersection ix2;
      _sceneModeller->nearestTissue( last, ix2 );
      nodePair.setSecond( ix2.nearestVertex() );

      nodePair.setWeight( tck.weight );

      return true;
    }

  protected:

    std::shared_ptr< SceneModeller > _sceneModeller;

};


class Graph
{

  public:

    Graph( const label_vector_type& labels, const LUT& ctable )
      : _labels( labels ), _ctable( ctable )
    {
      // matrix allocation
      std::set< uint32_t > unique_labels;
      for ( uint32_t l = 0; l < _labels.size(); l++ )
      {
        if ( _labels( l ) )
        {
          unique_labels.insert( _labels( l ) );
        }
      }
      uint32_t node_count = unique_labels.size();
      if ( node_count != _ctable.size() )
      {
        throw Exception( "Labels mismatch colour LUT" );
      }
      _M = matrix_type::Zero( node_count, node_count );
    };

    ~Graph() {};

    bool operator() ( const NodePair& nodePair )
    {
      _assignment_pairs.push_back( nodePair );
      return true;
    }

    void update( const NodePair& nodePair )
    {
      _assignment_pairs.push_back( nodePair );
    }
 
    void write_assign( const std::string& path ) const
    {    
      File::OFStream out( path, std::ios_base::out | std::ios_base::trunc );
      auto a = _assignment_pairs.begin(), ae = _assignment_pairs.end();
      while ( a != ae )
      {
        out << a->getFirst() << " " << a->getSecond() << " -> "
            << _labels( a->getFirst() ) << " " << _labels( a->getSecond() ) << "\n";
        ++ a;    
      }
    }

    void write_matrix( const std::string& path )
    {      
      // map assignment to matrix
      auto a = _assignment_pairs.begin(), ae = _assignment_pairs.end();
      while ( a != ae )
      {
        uint32_t node1 = _labels( a->getFirst() ) - 1;
        uint32_t node2 = _labels( a->getSecond() ) - 1;
        if ( node1 <= node2 )
        {
          _M( node1, node2 ) += a->getWeight();
        }
        else
        {
          _M( node2, node1 ) += a->getWeight();
        }
        ++ a;
      }
      // write to file
      File::OFStream out( path );
      Eigen::IOFormat fmt( Eigen::FullPrecision,
                           Eigen::DontAlignCols,
                           " ", "\n", "", "", "", "");
      out << _M.format(fmt);
      out << "\n";
    }

  protected:

    label_vector_type _labels;
    LUT _ctable;
    std::vector< NodePair > _assignment_pairs;

    matrix_type _M;

};


void usage ()
{

  AUTHOR = "Chun-Hung Yeh (chun-hung.yeh@florey.edu.au)";

  SYNOPSIS = "construct a connectivity matrix from a streamline tractography file and a brain surface/mesh file "
             "(this is a temporary command which will be integrated into tck2connectome).";

  ARGUMENTS
  + Argument( "track_in", "the input track file (.tck)" ).type_file_in()

  + Argument( "mesh_in", "the mesh file (.vtk)" ).type_file_in()

  + Argument( "annot_in", "the annotation file in FreeSurfer's annotation format ").type_file_in()

  + Argument( "matrix_out", "the output connectivity matrix file (.csv)" ).type_file_out();

  OPTIONS
  + Tractography::TrackWeightsInOption

  + Option( "lut", "edge length in mm for spatial lookup table (default=0.2mm)" )
    + Argument( "value" ).type_float( 0.0, 25.0 )

  + Option( "out_assign", "output the node assignments of each streamline to a file" )
    + Argument( "path" ).type_file_out();

};


void run ()
{

  // read surface
  std::map< std::string, Mesh > meshes;
  meshes[ "ROI" ] = Mesh{ argument[ 1 ] };

  // read annotation
  LUT ctable;
  label_vector_type labels;
  FreeSurfer::read_annot( argument[ 2 ], labels, ctable );

  // determine the mesh bounding box
  Eigen::Vector3d lower_p = meshes.begin()->second.vert( 0 );
  Eigen::Vector3d upper_p = meshes.begin()->second.vert( 0 );
  for ( auto m = meshes.begin(); m != meshes.end(); ++ m )
  {
    for ( size_t v = 0; v < m->second.num_vertices(); v++ )
    {
      Vertex thisVertex = m->second.vert( v );
      for ( size_t axis = 0; axis < 3; axis++ )
      {
        lower_p[ axis ] = std::min( lower_p[ axis ], thisVertex[ axis ] );
        upper_p[ axis ] = std::max( upper_p[ axis ], thisVertex[ axis ] );
      }
    }
  }

  // build bounding box and scene modeller
  auto opt = get_options( "lut" );
  double edge_length = opt.size() ? ( opt[ 0 ][ 0 ] ) : 0.2;

  BoundingBox< double > boundingBox( lower_p[ 0 ], upper_p[ 0 ],
                                     lower_p[ 1 ], upper_p[ 1 ],
                                     lower_p[ 2 ], upper_p[ 2 ] );
  Eigen::Vector3i lutSize( ( upper_p[ 0 ] - lower_p[ 0 ] ) / edge_length,
                           ( upper_p[ 1 ] - lower_p[ 1 ] ) / edge_length,
                           ( upper_p[ 2 ] - lower_p[ 2 ] ) / edge_length );

  std::shared_ptr< SceneModeller > sceneModeller( new SceneModeller( boundingBox, lutSize ) );

  // build tissues in scene modeller
  std::set< Tissue_ptr > tissues;
  Tissue_ptr tissue;
  tissue.reset( new Tissue( OTHER, "ROI", meshes[ "ROI" ], sceneModeller ) );
  tissues.insert( tissue );
  sceneModeller->addTissues( tissues );

  // preparing methods
  Assigner assigner( sceneModeller );
  Graph graph( labels, ctable );

  // preparing input track file
  Tractography::Properties properties;
  Tractography::Reader< float > reader( argument[ 0 ], properties );

  // Supporting multi-threading application
  Mapping::TrackLoader loader( reader,
                               properties[ "count" ].empty() ? 0 : to< size_t >( properties[ "count" ] ),
                               "Streamline assignment... " );
  Thread::run_queue( loader, 
                     Thread::batch( Tractography::Streamline< float >() ),
                     Thread::multi( assigner ),
                     Thread::batch( NodePair() ),
                     graph );

  opt = get_options( "out_assign" );
  if ( opt.size() )
  {
    graph.write_assign( opt[ 0 ][ 0 ] );
  }
  graph.write_matrix( argument[ 3 ] );

}
