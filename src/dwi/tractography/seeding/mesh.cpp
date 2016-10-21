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


#include "dwi/tractography/seeding/mesh.h"
#include "dwi/tractography/rng.h"


#define EPSILON std::numeric_limits< double >::epsilon()


namespace MR
{

namespace DWI
{

namespace Tractography
{

namespace Seeding
{


Mesh::Mesh( const std::string& in )
     : Base( in, "surface mesh", MAX_TRACKING_SEED_ATTEMPTS_FIXED ),
       _mesh( in )
{
  size_t triangleCount = _mesh.num_triangles();
  std::vector< double > areas( triangleCount );
  double totalArea = 0.0;
  for ( size_t t = 0; t < triangleCount; t++ )
  {
    auto triangle = _mesh.tri( t );
    areas[ t ] = calculate_area( _mesh.vert( triangle[ 0 ] ),
                                 _mesh.vert( triangle[ 1 ] ),
                                 _mesh.vert( triangle[ 2 ] ) );
    totalArea += areas[ t ];
  }
  for ( size_t t = 0; t < triangleCount; t++ )
  {
    areas[ t ] /= totalArea;
  }
  sort_index_descend( areas, _indices );
  _cdf.resize( triangleCount );
  for ( size_t t = 0; t < triangleCount; t++ )
  {
    // pdf in descending order
    _cdf[ t ] = areas[ _indices[ t ] ];
  }
  for ( size_t t = 1; t < triangleCount; t++ )
  {
    // pdf to cdf
    _cdf[ t ] += _cdf[ t - 1 ];
  }
  /*
   * If using FreeSurfer ?h.white or ?h.pial, will need to also import the
   * vertex label as the meshes contain corpus callosum...
   */ 
}


Mesh::~Mesh()
{
}


bool Mesh::get_seed( Eigen::Vector3f& point ) const
{
  // choose a triangle with probability defined by triangle areas
  std::uniform_real_distribution< double > uniform( 0.0, 1.0 );
  int32_t index = -1;
  double rn = uniform( *rng );
  for ( size_t c = 0; c < _cdf.size(); c++ )
  {
    if ( rn <= _cdf[ c ] )
    {
      index = _indices[ c ];
      break;
    }
  }
  if ( index < 0 )
  {
    throw Exception( "Mesh seeding: Fail to get a polygon index" );
  }
  auto triangle = _mesh.tri( index );
  auto v1 = _mesh.vert( triangle[ 0 ] );
  auto v2 = _mesh.vert( triangle[ 1 ] );
  auto v3 = _mesh.vert( triangle[ 2 ] );
  Eigen::Vector3d v12 = v2 - v1;
  Eigen::Vector3d v13 = v3 - v1;
  Eigen::Vector3d v23 = v3 - v2;
  Eigen::Vector3d normal = v12.cross( v13 );
  if ( std::abs( normal.dot( normal ) ) < EPSILON )
  {
    throw Exception( "triangle normal is a null vector: invalid triangle" );
  }
  normal.normalize();

  // create a bounding box with dimension defined by the target triangle
  Eigen::Vector3d lower_p = v1;
  Eigen::Vector3d upper_p = v1;
  for ( size_t a = 0; a < 3; a++ )
  {
    lower_p[ a ] = std::min( std::min( v1[ a ], v2[ a ] ), v3[ a ] );
    upper_p[ a ] = std::max( std::max( v1[ a ], v2[ a ] ), v3[ a ] );
  }

  // generate a random point inside the box and project it onto the triangle
  std::uniform_real_distribution< double > rnx( lower_p[ 0 ], upper_p[ 0 ] );
  std::uniform_real_distribution< double > rny( lower_p[ 1 ], upper_p[ 1 ] );
  std::uniform_real_distribution< double > rnz( lower_p[ 2 ], upper_p[ 2 ] );
  do
  {
    Eigen::Vector3d rand_p( rnx( *rng ), rny( *rng ), rnz( *rng ) );
    double t = normal.dot( v1 - rand_p );
    Eigen::Vector3d proj_p = rand_p + normal * t;
    // check whether the projection point is inside the triangle or on the edge
    if ( ( ( ( proj_p - v1 ).cross( v12 ) ).dot( v13.cross( v12 ) ) >= 0 ) &&
         ( ( ( proj_p - v2 ).cross( v23 ) ).dot( -v12.cross( v23 ) ) >= 0 ) &&
         ( ( ( proj_p - v3 ).cross( -v13 ) ).dot( v23.cross( v13 ) ) >= 0 ) )
    {
      // true: assign projection point to seed point
      for ( size_t a = 0; a < 3; a++ )
      {
        point[ a ] = ( float )proj_p[ a ];
      }
      return true;
    }
  } while ( 1 );

  return false;
}


double Mesh::calculate_area( const Surface::Vertex& v1,
                             const Surface::Vertex& v2,
                             const Surface::Vertex& v3 ) const
{
  return 0.5 * ( ( v2 - v1 ).cross( v3 - v1 ) ).norm();
}


void Mesh::sort_index_descend( const std::vector< double >& values,
                               std::vector< size_t >& indices ) const
{
  indices.resize( values.size() );
  std::iota( indices.begin(), indices.end(), 0 );
  std::sort( indices.begin(), indices.end(),
             [ &values ]( size_t i1, size_t i2 )
             { return values[ i1 ] > values[ i2 ]; } );
}


}

}

}

}


#undef EPSILON

