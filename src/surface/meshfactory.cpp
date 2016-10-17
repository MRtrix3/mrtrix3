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


#include "surface/meshfactory.h"


namespace MR
{

namespace Surface
{


MeshFactory::MeshFactory()
{
}


MeshFactory::~MeshFactory()
{
}


Mesh MeshFactory::box( const Eigen::Vector3d& lowerPoint,
                       const Eigen::Vector3d& upperPoint ) const
{
  VertexList vertices;
  vertices.push_back( Eigen::Vector3d( lowerPoint[ 0 ], lowerPoint[ 1 ], lowerPoint[ 2 ] ) );
  vertices.push_back( Eigen::Vector3d( upperPoint[ 0 ], lowerPoint[ 1 ], lowerPoint[ 2 ] ) );
  vertices.push_back( Eigen::Vector3d( lowerPoint[ 0 ], upperPoint[ 1 ], lowerPoint[ 2 ] ) );
  vertices.push_back( Eigen::Vector3d( upperPoint[ 0 ], upperPoint[ 1 ], lowerPoint[ 2 ] ) );
  vertices.push_back( Eigen::Vector3d( lowerPoint[ 0 ], lowerPoint[ 1 ], upperPoint[ 2 ] ) );
  vertices.push_back( Eigen::Vector3d( upperPoint[ 0 ], lowerPoint[ 1 ], upperPoint[ 2 ] ) );
  vertices.push_back( Eigen::Vector3d( lowerPoint[ 0 ], upperPoint[ 1 ], upperPoint[ 2 ] ) );
  vertices.push_back( Eigen::Vector3d( upperPoint[ 0 ], upperPoint[ 1 ], upperPoint[ 2 ] ) );

  TriangleList triangles;
  triangles.push_back( std::vector< size_t >{ 1, 7, 5 } );
  triangles.push_back( std::vector< size_t >{ 1, 3, 7 } );
  triangles.push_back( std::vector< size_t >{ 3, 6, 7 } );
  triangles.push_back( std::vector< size_t >{ 3, 2, 6 } );
  triangles.push_back( std::vector< size_t >{ 0, 4, 6 } );
  triangles.push_back( std::vector< size_t >{ 0, 6, 2 } );
  triangles.push_back( std::vector< size_t >{ 0, 5, 4 } );
  triangles.push_back( std::vector< size_t >{ 0, 1, 5 } );
  triangles.push_back( std::vector< size_t >{ 5, 7, 6 } );
  triangles.push_back( std::vector< size_t >{ 5, 6, 4 } );
  triangles.push_back( std::vector< size_t >{ 2, 3, 1 } );
  triangles.push_back( std::vector< size_t >{ 0, 2, 1 } );

  Mesh mesh;
  mesh.load( vertices, triangles );
  return mesh;
}


Mesh MeshFactory::sphere( const Eigen::Vector3d& centre,
                          const double& radius,
                          const size_t& level ) const
{
  VertexList vertices;

  // create 12 vertices of a icosahedron
  double t = ( 1.0 + std::sqrt( 5.0 ) ) / 2.0;
  vertices.push_back( Eigen::Vector3d( -1,  t,  0 ) );
  vertices.push_back( Eigen::Vector3d(  1,  t,  0 ) );
  vertices.push_back( Eigen::Vector3d( -1, -t,  0 ) );
  vertices.push_back( Eigen::Vector3d(  1, -t,  0 ) );
  vertices.push_back( Eigen::Vector3d(  0, -1,  t ) );
  vertices.push_back( Eigen::Vector3d(  0,  1,  t ) );
  vertices.push_back( Eigen::Vector3d(  0, -1, -t ) );
  vertices.push_back( Eigen::Vector3d(  0,  1, -t ) );
  vertices.push_back( Eigen::Vector3d(  t,  0, -1 ) );
  vertices.push_back( Eigen::Vector3d(  t,  0,  1 ) );
  vertices.push_back( Eigen::Vector3d( -t,  0, -1 ) );
  vertices.push_back( Eigen::Vector3d( -t,  0,  1 ) );
  for ( auto v = vertices.begin(); v != vertices.end(); ++ v )
  {
    ( *v ) *= radius;
    ( *v ) += centre;
  }

  /* map vertices to 20 faces */
  TriangleList triangles;
  triangles.push_back( std::vector< size_t >{  0, 11,  5 } );
  triangles.push_back( std::vector< size_t >{  0,  5,  1 } );
  triangles.push_back( std::vector< size_t >{  0,  1,  7 } );
  triangles.push_back( std::vector< size_t >{  0,  7, 10 } );
  triangles.push_back( std::vector< size_t >{  0, 10, 11 } );

  triangles.push_back( std::vector< size_t >{  1,  5,  9 } );
  triangles.push_back( std::vector< size_t >{  5, 11,  4 } );
  triangles.push_back( std::vector< size_t >{ 11, 10,  2 } );
  triangles.push_back( std::vector< size_t >{ 10,  7,  6 } );
  triangles.push_back( std::vector< size_t >{  7,  1,  8 } );

  triangles.push_back( std::vector< size_t >{  3,  9,  4 } );
  triangles.push_back( std::vector< size_t >{  3,  4,  2 } );
  triangles.push_back( std::vector< size_t >{  3,  2,  6 } );
  triangles.push_back( std::vector< size_t >{  3,  6,  8 } );
  triangles.push_back( std::vector< size_t >{  3,  8,  9 } );

  triangles.push_back( std::vector< size_t >{  4,  9,  5 } );
  triangles.push_back( std::vector< size_t >{  2,  4, 11 } );
  triangles.push_back( std::vector< size_t >{  6,  2, 10 } );
  triangles.push_back( std::vector< size_t >{  8,  6,  7 } );
  triangles.push_back( std::vector< size_t >{  9,  8,  1 } );

  Mesh mesh;
  mesh.load( vertices, triangles );

  if ( level )
  {
    // WIP : THIS DOES NOT WORK YET!!!
    /*
     * can increase mesh resolution by
     * a) looping over triangles
     * b) get midpoints of three edges
     * c) get the centre points of the triange
     * d) refine vertices
     * e) add 4 new sub-triangles per triangle
     */
    size_t offset = mesh.num_vertices();
    VertexList newVertices;
    TriangleList newTriangles;
    for ( size_t i = 0; i < level; i++ )
    {
      for ( auto t = triangles.begin(); t != triangles.end(); ++ t )
      {
        Eigen::Vector3d mid12 = ( mesh.vert( ( *t )[ 0 ] ) + 
                                  mesh.vert( ( *t )[ 1 ] ) ) / 2.0;
        uint32_t new_v12 = offset + 1;

        Eigen::Vector3d mid23 = ( mesh.vert( ( *t )[ 1 ] ) + 
                                  mesh.vert( ( *t )[ 2 ] ) ) / 2.0;
        uint32_t new_v23 = offset + 2;

        Eigen::Vector3d mid31 = ( mesh.vert( ( *t )[ 2 ] ) + 
                                  mesh.vert( ( *t )[ 0 ] ) ) / 2.0;
        uint32_t new_v31 = offset + 3;

        newVertices.push_back( mid12 );
        newVertices.push_back( mid23 );
        newVertices.push_back( mid31 );

        newTriangles.push_back( std::vector< size_t >{ ( *t )[ 0 ], new_v12, new_v31 } );
        newTriangles.push_back( std::vector< size_t >{ ( *t )[ 1 ], new_v23, new_v12 } );
        newTriangles.push_back( std::vector< size_t >{ ( *t )[ 2 ], new_v31, new_v23 } );
        newTriangles.push_back( std::vector< size_t >{ new_v12, new_v23, new_v31 } );

        offset += 3;
      }
      for ( auto v = newVertices.begin(); v != newVertices.end(); ++ v )
      {
        vertices.push_back( *v );
      }
      for ( auto t = newTriangles.begin(); t != newTriangles.end(); ++ t )
      {
        triangles.push_back( *t );
      }
      mesh.load( vertices, triangles );
    }
  }
  return mesh;
}


Mesh MeshFactory::concatenate( const std::vector< Mesh >& meshes ) const
{
  Surface::VertexList vertices;
  Surface::TriangleList triangles;

  size_t offset = 0;
  for ( size_t m = 0; m < meshes.size(); m++ )
  {
    Surface::Mesh thisMesh = meshes[ m ];
    auto v = thisMesh.get_vertices().begin(),
         ve = thisMesh.get_vertices().end();
    while ( v != ve )
    {
      vertices.push_back( *v );
      ++ v;
    }
    auto t = thisMesh.get_triangles().begin(),
         te = thisMesh.get_triangles().end();
    while ( t != te )
    {
      Surface::Triangle triangle{ *t };
      for ( size_t p = 0; p < 3; p++ )
      {
        triangle[ p ] += offset;
      }
      triangles.push_back( triangle );
      ++ t;
    }
    offset += thisMesh.num_vertices();
  }

  Mesh mesh;
  mesh.load( vertices, triangles );
  return mesh;
}


}

}

