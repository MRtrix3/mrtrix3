/*
    Copyright 2015 Brain Research Institute, Melbourne, Australia

    Written by Chun-Hung Jimmy Yeh, 2015.

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


#include "command.h"
#include "dwi/tractography/mapping/loader.h"
#include "dwi/tractography/connectomics/connectome_mapper_factory.h"


using namespace MR;
using namespace App;
using namespace MR::DWI;
using namespace MR::DWI::Tractography;


const char* modes[] = { "search_by_endpoint",
                        "search_by_tangent",
                        NULL };

const OptionGroup AssignmentOption =
  OptionGroup( "Structural connectome streamline assignment option" )

  + Option( "search_by_endpoint",
            "find the closest polygon/node from streamline endpoint.\n"
            "Argument is the maximum distance in mm; if no polygon is found "
            "within this value, the streamline endpoint is not assigned to any "
            "node (default = 2mm)." )
    + Argument( "max_dist" ).type_float(
                                  1e-9, 0.0, std::numeric_limits< int >::max() )

  + Option( "search_by_tangent",
            "find intersecting polygons/nodes from a tangent line.\n"
            "Argument is the maximum distance in mm; if not any intersecting "
            "polygon exists within this value, the streamline endpoint is not "
            "assigned to any node (default = 2mm)." )
    + Argument( "max_dist" ).type_float(
                                 1e-9, 0.0, std::numeric_limits< int >::max() );

void usage ()
{

  AUTHOR = "C.-H. Jimmy Yeh (j.yeh@brain.org.au)";

  DESCRIPTION
  + "construct a connectivity matrix from a streamline tractography file and a "
    "brain surface/mesh file";

  ARGUMENTS
  + Argument( "tracks_in",
              "the input track file (.tck)" ).type_file_in()

  + Argument( "mesh_in",
              "the input mesh file (must be vtk format)" ).type_file_in()

  + Argument( "connectome_out",
              "the output .csv file" ).type_file_out();

  OPTIONS
  + Option( "cache_size",
            "the cache size for dividing the global space into several small "
            "partitions. Each subvolume store a subset of mesh polygons and "
            "vertices. This can help speed up process for finding the relevant "
            "polygon from a streamline endpoint (default = 100,100,100)." )
    + Argument( "x,y,z" ).type_sequence_int()

  + AssignmentOption;

};


void run ()
{

  // Reading the cache size
  Options opt = get_options( "cache_size" );
  Point< int32_t > cacheSize( 100, 100, 100 );
  if ( opt.size() )
  {

    std::vector< int32_t > cache_size_in = opt[ 0 ][ 0 ];
    cacheSize[ 0 ] = cache_size_in[ 0 ];
    cacheSize[ 1 ] = cache_size_in[ 1 ];
    cacheSize[ 2 ] = cache_size_in[ 2 ];

  }

  // Reading the mesh data
  Mesh::Mesh* mesh = new Mesh::Mesh( argument[ 1 ] );

  std::cout << "Preparing connectome mapper: " << std::flush;

  // Building the bounding box from the mesh
  Mesh::VertexList vertices = mesh->getVertices();
  Point< float > lower_point = vertices[ 0 ];
  Point< float > upper_point = vertices[ 0 ];
  for ( int32_t v = 0; v < vertices.size(); v++ )
  {

    Point< float > currentVertex = vertices[ v ];
    for ( int32_t axis = 0; axis < 3; axis++ )
    {

      lower_point[ axis ] = std::min( lower_point[ axis ],
                                      currentVertex[ axis ] );
      upper_point[ axis ] = std::max( upper_point[ axis ],
                                      currentVertex[ axis ] );

    }

  }
  Mesh::BoundingBox< float > boundingBox( lower_point[ 0 ], upper_point[ 0 ],
                                          lower_point[ 1 ], upper_point[ 1 ],
                                          lower_point[ 2 ], upper_point[ 2 ] );

  // Building the scene modeller
  Mesh::SceneModeller* sceneModeller = new Mesh::SceneModeller( boundingBox,
                                                                cacheSize );

  // Building the scene mesh
  Mesh::SceneMesh* sceneMesh = new Mesh::SceneMesh( sceneModeller, mesh, 0.0 );

  // Adding the scene mesh to the scene modeller
  sceneModeller->addSceneMesh( sceneMesh );

  // Building a connectome mapper
  Connectomics::ConnectomeMapper* connectomeMapper = NULL;
  for ( size_t index = 0; modes[ index ]; index++ )
  {

    Options opt = get_options( modes[ index ] );
    if ( opt.size() )
    {

      if ( connectomeMapper )
      {
      
        delete connectomeMapper;
        connectomeMapper = NULL;
        throw Exception( "Please only request one streamline assignment "
                         "mechanism" );
      
      }
      switch ( index )
      {
      
        case 0: connectomeMapper =
                  Connectomics::ConnectomeMapperFactory::getInstance().
                   getPoint2MeshMapper( sceneModeller, float( opt[ 0 ][ 0 ] ) );
        break;
        
        case 1: connectomeMapper =
                  Connectomics::ConnectomeMapperFactory::getInstance().
                     getRay2MeshMapper( sceneModeller, float( opt[ 0 ][ 0 ] ) );
        break;

      }

    }

  }
  if ( !connectomeMapper )
  {

    // using default connectome mapper using endpoint search
    connectomeMapper = Connectomics::ConnectomeMapperFactory::getInstance().
                                      getPoint2MeshMapper( sceneModeller, 2.0 );

  }
  Connectomics::MultiThreadMapper multiThreadMapper( connectomeMapper );

  // Preparing output connectome
  Connectomics::Connectome connectome;
  connectome.allocate( connectomeMapper->getNodeCount() );

  std::cout << "[Done]" << std::endl;

  // Reading track data
  Tractography::Properties properties;
  Tractography::Reader< float > reader( argument[ 0 ], properties );

  // Supporting multi-threading application
  Mapping::TrackLoader loader( reader,
                               properties[ "count" ].empty() ?
                                 0 : to< size_t >( properties[ "count" ] ),
                               "Constructing connectome... " );
  Thread::run_queue( loader, 
                     Thread::batch( Tractography::Streamline< float >() ),
                     Thread::multi( multiThreadMapper ),
                     Thread::batch( Connectomics::NodePair() ),
                     connectome );

  // Saving the output connectome
  std::cout << "starting writing the output file" << std::endl;
  connectome.write( argument[ 2 ] );

}

