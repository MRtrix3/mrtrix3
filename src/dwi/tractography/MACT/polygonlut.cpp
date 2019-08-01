/*
 * Copyright (c) 2008-2018 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/
 *
 * MRtrix3 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/
 */


#include "dwi/tractography/MACT/polygonlut.h"
#include "dwi/tractography/MACT/scenemodeller.h"


namespace MR
{

namespace DWI
{

namespace Tractography
{

namespace MACT
{


PolygonLut::PolygonLut( const std::shared_ptr< Tissue >& tissue )
           : _tissue( tissue )
{
  Surface::VertexList vertices = tissue->mesh().get_vertices();
  Surface::TriangleList polygons = tissue->mesh().get_triangles();

  std::set< Eigen::Vector3i, Vector3iCompare > voxels;
  for ( uint32_t p = 0; p < polygons.size(); p++ )
  {
    _tissue->sceneModeller()->bresenhamLine().discTriangleVoxels(
                                   vertices[ polygons[ p ][ 0 ] ],
                                   vertices[ polygons[ p ][ 1 ] ],
                                   vertices[ polygons[ p ][ 2 ] ],
                                   _tissue->radiusOfInfluence(), voxels, true );
    auto v = voxels.begin(), ve = voxels.end();
    while ( v != ve )
    {
      if ( !_lut.count( *v ) )
      {
        // LUT does not have this voxel :
        // initialise a new set of polygon indices and add to this voxel
        std::set< uint32_t > newPolygons{ p };
        _lut[ *v ] = newPolygons;
      }
      else
      {
        // LUT has this voxel --> add the polygon index to the list of the voxel
        _lut[ *v ].insert( p );
      }
      ++ v;
    }
  }
}


PolygonLut::~PolygonLut()
{
}


std::set< uint32_t > 
PolygonLut::getPolygonIds( const Eigen::Vector3i& voxel ) const
{
  if ( _lut.count( voxel ) == 0 )
  {
    return std::set< uint32_t >();
  }
  else
  {
    return _lut.find( voxel )->second;
  }
}


std::set< uint32_t > 
PolygonLut::getPolygonIds( const Eigen::Vector3d& point ) const
{
  Eigen::Vector3i voxel;
  _tissue->sceneModeller()->lutVoxel( point, voxel );
  return getPolygonIds( voxel );
}


std::set< uint32_t > 
PolygonLut::getPolygonIds( const std::set< Eigen::Vector3i, Vector3iCompare >& voxels ) const
{
  std::set< uint32_t > triangleIds;
  for ( auto v = voxels.begin(); v != voxels.end(); ++v )
  {
    auto t = getPolygonIds( *v );
    triangleIds.insert( t.begin(), t.end() );
  }
  return triangleIds;
}


std::set< uint32_t > 
PolygonLut::getPolygonIds( const std::set< Eigen::Vector3d >& points ) const
{
  std::set< uint32_t > triangleIds;
  for ( auto p = points.begin(); p != points.end(); ++p )
  {
    Eigen::Vector3i voxel;
    _tissue->sceneModeller()->lutVoxel( *p, voxel );
    auto t = getPolygonIds( voxel );
    triangleIds.insert( t.begin(), t.end() );
  }
  return triangleIds;
}


}

}

}

}

