/*
    Copyright 2015 Brain Research Institute, Melbourne, Australia

    Written by Chun-Hung Jimmy Yeh, 04/2015.

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
#include "mesh/scene_modeller.h"
#include "mesh/scene_mesh.h"
#include "dwi/tractography/file.h"
#include "dwi/tractography/properties.h"
#include "dwi/tractography/mapping/loader.h"
#include "dwi/tractography/connectomics/tckmesh_mapper.h"
#include "dwi/tractography/connectomics/connectome.h"
#include "progressbar.h"
#include "thread_queue.h"


using namespace MR;
using namespace App;
using namespace MR::DWI;
using namespace MR::DWI::Tractography;


void usage ()
{

  AUTHOR = "C.-H. Jimmy Yeh (j.yeh@brain.org.au)";

  DESCRIPTION
  + "construct a connectivity matrix from a tractography file and a brain mesh."

  + "note: this is a test command which is NOT ready for use. It still needs "
    "improvement (e.g. supporting multi-threading application).";

  ARGUMENTS
  + Argument( "tracks_in",
              "the input track file (.tck)").type_file_in()
  + Argument( "mesh_in",
              "the input mesh file (must be vtk format)").type_image_in()
  + Argument( "connectome_out",
              "the output .csv file").type_file_out();

  OPTIONS
  + Option( "cache_size",
            "the cache size for dividing the global space into several small "
            "partitions. Each cube would therefore contain a subset of mesh "
            "polygons and vertices. This can help speed up process for finding "
            "the closest polygon from a streamline endpoint. "
            "(default = 100,100,100)")
    + Argument( "x,y,z" ).type_sequence_int()

  + Option( "threshold",
            "the distance threshold. If the value is greater than the "
            "point-to-polygon (triangle) distance, the streamline will not "
            "contribute to the connectome edge. (default = 1mm)" )
    + Argument( "value" ).type_float(
                                 1e-9, 0.0, std::numeric_limits< int >::max() );

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

  // Reading distance threshold
  opt = get_options( "threshold" );
  float distanceThreshold = 1.0;
  if ( opt.size() )
  {

    float threshold = opt[ 0 ][ 0 ];
    distanceThreshold = threshold;

  }

  // Reading the mesh data
  std::cout << "Reading input vtk mesh: " << std::flush;
  Mesh::Mesh* mesh = new Mesh::Mesh( argument[ 1 ] );
  std::cout << "[ Done ]" << std::endl;

  // Collecting the vertices
  Mesh::VertexList vertices = mesh->vertices;
  Mesh::PolygonList polygons = mesh->polygons;

  Point< float > lower_point = vertices[ 0 ];
  Point< float > upper_point = vertices[ 0 ];
  int32_t vertexCount = vertices.size();
  Point< float > currentVertex;
  int32_t v;
  for ( v = 0; v < vertexCount; v++ )
  {

    currentVertex = vertices[ v ];

    // finding the lower bound
    if ( currentVertex[ 0 ] < lower_point[ 0 ] )
    {
      lower_point[ 0 ] = currentVertex[ 0 ];
    }
    if ( currentVertex[ 1 ] < lower_point[ 1 ] )
    {
      lower_point[ 1 ] = currentVertex[ 1 ];
    }
    if ( currentVertex[ 2 ] < lower_point[ 2 ] )
    {
      lower_point[ 2 ] = currentVertex[ 2 ];
    }
    // finding the upper bound
    if ( currentVertex[ 0 ] > upper_point[ 0 ] )
    {
      upper_point[ 0 ] = currentVertex[ 0 ];
    }
    if ( currentVertex[ 1 ] > upper_point[ 1 ] )
    {
      upper_point[ 1 ] = currentVertex[ 1 ];
    }
    if ( currentVertex[ 2 ] > upper_point[ 2 ] )
    {
      upper_point[ 2 ] = currentVertex[ 2 ];
    }

  }

  // Building the bounding box
  Mesh::BoundingBox< float > boundingBox( lower_point[ 0 ], upper_point[ 0 ],
                                          lower_point[ 1 ], upper_point[ 1 ],
                                          lower_point[ 2 ], upper_point[ 2 ] );

  // Building the scene modeller
  std::cout << "Building scene modeller: " << std::flush;
  Mesh::SceneModeller* sceneModeller = new Mesh::SceneModeller( boundingBox,
                                                                cacheSize );
  std::cout << "[ Done ]" << std::endl;


  // Building the scene mesh
  std::cout << "Building scene mesh: " << std::flush;
  Mesh::SceneMesh* sceneMesh = new Mesh::SceneMesh( sceneModeller, mesh, 0.0 );
  std::cout << "[ Done ]" << std::endl;

  // Adding the scene mesh to the scene modeller
  std::cout << "Adding scene mesh to scene modeller and building "
            << "polygon cache: " << std::flush;
  sceneModeller->addSceneMesh( sceneMesh );
  std::cout << "[ Done ]" << std::endl;

  // Building a connectome mapper
  Connectomics::TckMeshMapper tckMeshMapper( sceneModeller, distanceThreshold );

  // Preparing output connectome
  int32_t nodeCount = tckMeshMapper.getNodeCount();
  Connectomics::Connectome connectome( nodeCount );

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
                     Thread::multi( tckMeshMapper ),
                     Thread::batch( Connectomics::NodePair() ),
                     connectome );

  // Saving the output connectome
  /*std::cout << "starting writing the output file" << std::endl;
  connectome.write( argument[ 2 ] );*/

}

