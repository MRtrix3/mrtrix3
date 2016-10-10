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


#include "dwi/tractography/MACT/bresenhamline.h"


namespace MR
{

namespace DWI
{

namespace Tractography
{

namespace MACT
{


BresenhamLine::BresenhamLine( const BoundingBox< double >& boundingBox,
                              const Eigen::Vector3i& cacheSize )
              : _cacheSize( cacheSize ),
                _cacheSizeMinusOne( cacheSize[ 0 ] - 1,
                                    cacheSize[ 1 ] - 1,
                                    cacheSize[ 2 ] - 1 )
{
  _lowerX = boundingBox.getLowerX();
  _upperX = boundingBox.getUpperX();
  _lowerY = boundingBox.getLowerY();
  _upperY = boundingBox.getUpperY();
  _lowerZ = boundingBox.getLowerZ();
  _upperZ = boundingBox.getUpperZ();
  _cacheVoxelFactorX = ( double )_cacheSize[ 0 ] / ( _upperX - _lowerX );
  _cacheVoxelFactorY = ( double )_cacheSize[ 1 ] / ( _upperY - _lowerY );
  _cacheVoxelFactorZ = ( double )_cacheSize[ 2 ] / ( _upperZ - _lowerZ );
  _resolution[ 0 ] = ( _upperX - _lowerX ) / _cacheSize[ 0 ];
  _resolution[ 1 ] = ( _upperY - _lowerY ) / _cacheSize[ 1 ];
  _resolution[ 2 ] = ( _upperZ - _lowerZ ) / _cacheSize[ 2 ];
  _minResolution = std::min( std::min( _resolution[ 0 ],
                                       _resolution[ 1 ] ),
                                       _resolution[ 2 ] );
}


BresenhamLine::~BresenhamLine()
{
}


const Eigen::Vector3d& BresenhamLine::resolution() const
{
  return _resolution;
}


double BresenhamLine::minResolution() const
{
  return _minResolution;
}


void BresenhamLine::point2voxel( const Eigen::Vector3d& point,
                                 Eigen::Vector3i& voxel ) const
{
  // x coordinate
  if ( point[ 0 ] < _lowerX )
  {
    voxel[ 0 ] = 0;
  }
  else if ( point[ 0 ] >= _upperX )
  {
    voxel[ 0 ] = _cacheSizeMinusOne[ 0 ];
  }
  else
  {
    voxel[ 0 ] = ( int32_t )( ( point[ 0 ] - _lowerX ) * _cacheVoxelFactorX );
    if ( voxel[ 0 ] < 0 )
    {
      voxel[ 0 ] = 0;
    }
    else if ( voxel[ 0 ] > _cacheSizeMinusOne[ 0 ] )
    {
      voxel[ 0 ] = _cacheSizeMinusOne[ 0 ];
    }
  }

  // y coordinate
  if ( point[ 1 ] < _lowerY )
  {
    voxel[ 1 ] = 0;
  }
  else if ( point[ 1 ] >= _upperY )
  {
    voxel[ 1 ] = _cacheSizeMinusOne[ 1 ];
  }
  else
  {
    voxel[ 1 ] = ( int32_t )( ( point[ 1 ] - _lowerY ) * _cacheVoxelFactorY );
    if ( voxel[ 1 ] < 0 )
    {
      voxel[ 1 ] = 0;
    }
    else if ( voxel[ 1 ] > _cacheSizeMinusOne[ 1 ] )
    {
      voxel[ 1 ] = _cacheSizeMinusOne[ 1 ];
    }
  }

  // z coordinate
  if ( point[ 2 ] < _lowerZ )
  {
    voxel[ 2 ] = 0;
  }
  else if ( point[ 2 ] >= _upperZ )
  {
    voxel[ 2 ] = _cacheSizeMinusOne[ 2 ];
  }
  else
  {
    voxel[ 2 ] = ( int32_t )( ( point[ 2 ] - _lowerZ ) * _cacheVoxelFactorZ );
    if ( voxel[ 2 ] < 0 )
    {
      voxel[ 2 ] = 0;
    }
    else if ( voxel[ 2 ] > _cacheSizeMinusOne[ 2 ] )
    {
      voxel[ 2 ] = _cacheSizeMinusOne[ 2 ];
    }
  }
}


void BresenhamLine::neighbouringVoxels(
                                     const Eigen::Vector3i& voxel,
                                     Eigen::Vector3i stride,
                                     std::vector< Eigen::Vector3i >& neighbours,
                                     std::vector< bool >& neighbourMasks ) const
{
  // resizing the neighbourhood offsets and masks according to the stride
  Eigen::Vector3i size( 2 * stride[ 0 ] + 1,
                        2 * stride[ 1 ] + 1,
                        2 * stride[ 2 ] + 1 );
  neighbours.resize( size[ 0 ] * size[ 1 ] * size[ 2 ] );
  neighbourMasks.resize( size[ 0 ] * size[ 1 ] * size[ 2 ] );

  // processing the neighbourhood offsets and masks
  Eigen::Vector3i offset;
  int32_t index = 0;
  int32_t x, y, z;
  for ( x = -stride[ 0 ]; x <= stride[ 0 ]; x++ )
  {
    offset[ 0 ] = x;
    for ( y = -stride[ 1 ]; y <= stride[ 1 ]; y++ )
    {
      offset[ 1 ] = y;
      for ( z = -stride[ 2 ]; z <= stride[ 2 ]; z++ )
      {
        offset[ 2 ] = z;
        Eigen::Vector3i& neighbour = neighbours[ index ];
        neighbour = voxel + offset;
        if ( ( neighbour[ 0 ] >= 0 ) &&
             ( neighbour[ 0 ] <= _cacheSizeMinusOne[ 0 ] ) &&
             ( neighbour[ 1 ] >= 0 ) &&
             ( neighbour[ 1 ] <= _cacheSizeMinusOne[ 1 ] ) &&
             ( neighbour[ 2 ] >= 0 ) &&
             ( neighbour[ 2 ] <= _cacheSizeMinusOne[ 2 ] ) )
        {
          neighbourMasks[ index ] = true;
        }
        else
        {
          neighbourMasks[ index ] = false;
        }
        ++ index;
      }
    }
  }  
}


void BresenhamLine::rayVoxels(
                           const Eigen::Vector3d& from,
                           const Eigen::Vector3d& to,
                           std::set< Eigen::Vector3i, Vector3iCompare >& voxels,
                           bool clearVoxelsAtBeginning ) const
{
  if ( clearVoxelsAtBeginning )
  {
    voxels.clear();
  }

  // processing initial and last voxel
  Eigen::Vector3i fromVoxel, toVoxel;
  point2voxel( from, fromVoxel );
  point2voxel( to, toVoxel );
  voxels.insert( fromVoxel );
  if ( toVoxel != fromVoxel )
  {
    voxels.insert( toVoxel );

    // getting the segment count and the step
    double length = ( to - from ).norm();
    size_t segmentCount = ( size_t )( length / _minResolution ) + 1;
    Eigen::Vector3d step = ( to - from ) / ( double )segmentCount;

    Eigen::Vector3i currentVoxel = fromVoxel;
    Eigen::Vector3d newPoint = from;
    Eigen::Vector3i newVoxel;
    Eigen::Vector3i offset;
    Eigen::Vector3i neighbour;
    for ( size_t s = 1; s <= segmentCount; s++ )
    {
      // getting the current point and voxel
      newPoint += step;
      point2voxel( newPoint, newVoxel );
      voxels.insert( newVoxel );

      // processing the offset and collecting the voxels
      offset = newVoxel - currentVoxel;
      if ( ( std::abs( offset[ 0 ] ) +
             std::abs( offset[ 1 ] ) +
             std::abs( offset[ 2 ] ) ) > 1 )
      {
        neighbour[ 0 ] = currentVoxel[ 0 ] + offset[ 0 ];
        neighbour[ 1 ] = currentVoxel[ 1 ];
        neighbour[ 2 ] = currentVoxel[ 2 ];
        voxels.insert( neighbour ); 

        neighbour[ 0 ] = currentVoxel[ 0 ];
        neighbour[ 1 ] = currentVoxel[ 1 ] + offset[ 1 ];
        neighbour[ 2 ] = currentVoxel[ 2 ];
        voxels.insert( neighbour ); 

        neighbour[ 0 ] = currentVoxel[ 0 ];
        neighbour[ 1 ] = currentVoxel[ 1 ];
        neighbour[ 2 ] = currentVoxel[ 2 ] + offset[ 2 ];
        voxels.insert( neighbour ); 

        neighbour[ 0 ] = currentVoxel[ 0 ] + offset[ 0 ];
        neighbour[ 1 ] = currentVoxel[ 1 ] + offset[ 1 ];
        neighbour[ 2 ] = currentVoxel[ 2 ];
        voxels.insert( neighbour ); 

        neighbour[ 0 ] = currentVoxel[ 0 ] + offset[ 0 ];
        neighbour[ 1 ] = currentVoxel[ 1 ];
        neighbour[ 2 ] = currentVoxel[ 2 ] + offset[ 2 ];
        voxels.insert( neighbour ); 

        neighbour[ 0 ] = currentVoxel[ 0 ];
        neighbour[ 1 ] = currentVoxel[ 1 ] + offset[ 1 ];
        neighbour[ 2 ] = currentVoxel[ 2 ] + offset[ 2 ];
        voxels.insert( neighbour ); 
      }
      // updating the current voxel
      currentVoxel = newVoxel;
    }
  }
}


void BresenhamLine::triangleVoxels(
                           const Eigen::Vector3d& vertex1,
                           const Eigen::Vector3d& vertex2,
                           const Eigen::Vector3d& vertex3,
                           std::set< Eigen::Vector3i, Vector3iCompare >& voxels,
                           bool clearVoxelsAtBeginning ) const
{
  if ( clearVoxelsAtBeginning )
  {
    voxels.clear();
  }

  // I: collecting the voxels along the rays defined by the three vertices
  rayVoxels( vertex1, vertex2, voxels, false );
  rayVoxels( vertex2, vertex3, voxels, false );
  rayVoxels( vertex3, vertex1, voxels, false );

  // II: collecting the voxels along the rays defined by sub-triangles
  // 1) getting the length of the three line segments defined by the triangle
  double segment12 = ( vertex1 - vertex2 ).norm();
  double segment23 = ( vertex2 - vertex3 ).norm();
  double segment31 = ( vertex3 - vertex1 ).norm();

  // 2) getting the voxels between vertex3 and the points located on the segment
  //    defined by vertex1 and vertex2
  Eigen::Vector3d point = vertex1;
  size_t segmentCount = ( size_t )( segment12 / _minResolution ) + 1;
  Eigen::Vector3d step = ( vertex2 - vertex1 ) / ( double )segmentCount;
  for ( size_t s = 1; s < segmentCount; s++ )
  {
    point += step;
    rayVoxels( vertex3, point, voxels, false );
  }

  // 3) getting the voxels between vertex1 and the points located on the segment
  //    defined by vertex2 and vertex3
  point = vertex2;
  segmentCount = ( size_t )( segment23 / _minResolution ) + 1;
  step = ( vertex3 - vertex2 ) / ( double )segmentCount;
  for ( size_t s = 1; s < segmentCount; s++ )
  {
    point += step;
    rayVoxels( vertex1, point, voxels, false );
  }

  // 4) getting the voxels between vertex2 and the points located on the segment
  //    defined by vertex3 and vertex1
  point = vertex3;
  segmentCount = ( size_t )( segment31 / _minResolution ) + 1;
  step = ( vertex1 - vertex3 ) / ( double )segmentCount;
  for ( size_t s = 1; s < segmentCount; s++ )
  {
    point += step;
    rayVoxels( vertex2, point, voxels, false );
  }
}


void BresenhamLine::discTriangleVoxels(
                           const Eigen::Vector3d& vertex1,
                           const Eigen::Vector3d& vertex2,
                           const Eigen::Vector3d& vertex3,
                           double radiusOfInfluence,
                           std::set< Eigen::Vector3i, Vector3iCompare >& voxels,
                           bool clearVoxelsAtBeginning ) const

{
  if ( clearVoxelsAtBeginning )
  {
    voxels.clear();
  }

  if ( radiusOfInfluence == 0 )
  {
    triangleVoxels( vertex1, vertex2, vertex3, voxels, false );
  }
  else
  {
    // getting the shift vector for each vertex of the triangle
    Eigen::Vector3d shift1 = vertex1 - ( vertex2 + vertex3 ) / 2.0;
    shift1.normalize();
    shift1 *= radiusOfInfluence;
    Eigen::Vector3d shift2 = vertex2 - ( vertex3 + vertex1 ) / 2.0;
    shift2.normalize();
    shift2 *= radiusOfInfluence;
    Eigen::Vector3d shift3 = vertex3 - ( vertex1 + vertex2 ) / 2.0;
    shift3.normalize();
    shift3 *= radiusOfInfluence;

    // getting the normal vector of the plane defined by the vertices
    Eigen::Vector3d normal = ( vertex2 - vertex1 ).cross( vertex3 - vertex1 );
    normal.normalize();

    // 1) getting new vertices;
    // 2) moving the new triangle along its normal and collecting the voxels
    //    contained in the triangle for each movement;
    // 3) adding the voxels into a set to keep the voxel unique
    if ( ( 2.0 * radiusOfInfluence ) < _minResolution )
    {
      Eigen::Vector3d shiftAlongNormal;
      for ( size_t s = 0; s < 3; s++ )
      {
        shiftAlongNormal = normal * radiusOfInfluence * ( double )( 1 - s );
        triangleVoxels( vertex1 + shift1 + shiftAlongNormal,
                        vertex2 + shift2 + shiftAlongNormal,
                        vertex3 + shift3 + shiftAlongNormal,
                        voxels, false );
      }
    }
    else
    {
      // first adding the central triangle
      triangleVoxels( vertex1 + shift1, vertex2 + shift2, vertex3 + shift3,
                      voxels, false );

      // then adding the offset triangles
      size_t segmentCount = ( size_t )( radiusOfInfluence / _minResolution ) + 1;
      double step = radiusOfInfluence / ( double )segmentCount;
      Eigen::Vector3d shiftAlongNormal;
      for ( size_t s = 1; s <= segmentCount; s++ )
      {
        shiftAlongNormal = normal * step * ( double )s;
        // along +normal direction
        triangleVoxels( vertex1 + shift1 + shiftAlongNormal,
                        vertex2 + shift2 + shiftAlongNormal,
                        vertex3 + shift3 + shiftAlongNormal,
                        voxels, false );
        // along -normal direction
        triangleVoxels( vertex1 + shift1 - shiftAlongNormal,
                        vertex2 + shift2 - shiftAlongNormal,
                        vertex3 + shift3 - shiftAlongNormal,
                        voxels, false );
      }
    }
  }
}


}

}

}

}

