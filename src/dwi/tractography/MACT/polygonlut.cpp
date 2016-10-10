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
  std::set< Eigen::Vector3i, Vector3iCompare > voxels;
  for ( Polygon_i p = 0; p < tissue->polygonCount(); p++ )
  {
    auto mesh = tissue->mesh();
    auto triangle = tissue->mesh().tri( p );
    auto v1 = mesh.vert( triangle[ 0 ] );
    auto v2 = mesh.vert( triangle[ 1 ] );
    auto v3 = mesh.vert( triangle[ 2 ] );

    // collecting associated voxels using Bresenham's line algorithm
    _tissue->sceneModeller()->bresenhamLine().discTriangleVoxels(
                       v1, v2, v3, _tissue->radiusOfInfluence(), voxels, true );
    auto v = voxels.begin(), ve = voxels.end();
    while ( v != ve )
    {
      if ( _lut.count( *v ) == 0 )
      {
        // LUT does not have this voxel :
        // initialise a new set of polygon indices and add to this voxel
        std::set< Polygon_i > newPolygons{ p };
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


std::set< Polygon_i >
PolygonLut::getTriangles( const Eigen::Vector3i& voxel ) const
{
  if ( _lut.count( voxel ) == 0 )
  {
    return std::set< Polygon_i >();
  }
  else
  {
    return _lut.find( voxel )->second;
  }
}


std::set< Polygon_i >
PolygonLut::getTriangles( const Eigen::Vector3d& point ) const
{
  Eigen::Vector3i voxel;
  _tissue->sceneModeller()->lutVoxel( point, voxel );
  return getTriangles( voxel );
}


}

}

}

}

