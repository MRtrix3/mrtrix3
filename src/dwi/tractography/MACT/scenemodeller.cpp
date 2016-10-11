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


#include "dwi/tractography/MACT/scenemodeller.h"


#define EPSILON std::numeric_limits< double >::epsilon()


namespace MR
{

namespace DWI
{

namespace Tractography
{

namespace MACT
{


SceneModeller::SceneModeller( const BoundingBox< double >& boundingBox,
                              const Eigen::Vector3i& lutSize )
              : _boundingBox( boundingBox ),
                _integerBoundingBox( 0, lutSize[ 0 ] - 1,
                                     0, lutSize[ 1 ] - 1,
                                     0, lutSize[ 2 ] - 1 ),
                _lutSize( lutSize ),
                _bresenhamLine( boundingBox, lutSize ),
                _tissueLut( std::shared_ptr< SceneModeller >( this ) )
{
}


SceneModeller::~SceneModeller()
{
}


const BoundingBox< double >& SceneModeller::boundingBox() const
{
  return _boundingBox;
}


const BoundingBox< int32_t >& SceneModeller::integerBoundingBox() const
{
  return _integerBoundingBox;
}


const Eigen::Vector3i& SceneModeller::lutSize() const
{
  return _lutSize;
}


const BresenhamLine& SceneModeller::bresenhamLine() const
{
  return _bresenhamLine;
}


void SceneModeller::lutVoxel( const Eigen::Vector3d& point,
                              Eigen::Vector3i& voxel ) const
{
  _bresenhamLine.point2voxel( point, voxel );
}


void SceneModeller::addTissues( const std::set< Tissue_ptr >& tissues )
{
  auto t = tissues.begin(), te = tissues.end();
  while ( t != te )
  {
    if ( _tissues.find( ( *t )->name() ) != _tissues.end() )
    {
      throw Exception( "Duplicate tissue name" );
    }
    _tissues[ ( *t )->name() ] = *t;
    _tissueLut.update( *t );
    ++ t;
  }
}


const TissueLut& SceneModeller::tissueLut() const
{
  return _tissueLut;
}


bool SceneModeller::nearestTissue( const Eigen::Vector3d& point,
                                   Intersection& intersection ) const
{
  if ( _tissues.empty() )
  {
    return false;
  }
  else
  {
    // getting the local voxel from the given point
    Eigen::Vector3i voxel;
    _bresenhamLine.point2voxel( point, voxel );

    // increasing the stride until the nearest mesh tissue is found
    Eigen::Vector3i thisVoxel;
    int32_t stride = 1;
    while ( intersection._tissue == 0 )
    {
      for ( int32_t x = -stride; x <= stride; x++ )
      {
        thisVoxel[ 0 ] = voxel[ 0 ] + x;
        for ( int32_t y = -stride; y <= stride; y++ )
        {
          thisVoxel[ 1 ] = voxel[ 1 ] + y;
          for ( int32_t z = -stride; z <= stride; z++ )
          {
            thisVoxel[ 2 ] = voxel[ 2 ] + z;
            if ( _integerBoundingBox.contains( thisVoxel ) )
            {
              auto tissues = _tissueLut.getTissues( thisVoxel );
              if ( !tissues.empty() )
              {
                auto t = tissues.begin(), te = tissues.end();
                while ( t != te )
                {
                  Intersection newIntersection;
                  intersectionAtVoxel( point, thisVoxel, *t, newIntersection );
                  if ( newIntersection._arcLength < intersection._arcLength )
                  {
                    // updating distance / tissue / polygon / projection point
                    intersection = newIntersection;
                  }
                  ++ t;
                }
              }
            }
          }
        }
      }
      ++ stride;
    }
    return true;
  }
}


bool SceneModeller::insideTissue( const Eigen::Vector3d& point,
                                  const std::string& name ) const
{
  // ************ Note: the method only functions for a closed mesh ************
  if ( _tissues.find( name ) == _tissues.end() )
  {
    throw Exception( "Input tissue name not found" );
  }
  else
  {
    auto theTissue = _tissues.find( name )->second;
    Eigen::Vector3d projectionPoint( point );

    ////// casting a ray in +x or -x direction
    double upperX = _boundingBox.getUpperX();
    double lowerX = _boundingBox.getLowerX();
    projectionPoint[ 0 ] = ( upperX - point[ 0 ] ) <
                           ( point[ 0 ] - lowerX ) ? upperX : lowerX;
    IntersectionSet iX( *this, point, projectionPoint, theTissue );

    ////// casting a ray in +y or -y direction
    projectionPoint = point;
    double upperY = _boundingBox.getUpperY();
    double lowerY = _boundingBox.getLowerY();
    projectionPoint[ 1 ] = ( upperY - point[ 1 ] ) <
                           ( point[ 1 ] - lowerY ) ? upperY : lowerY;
    IntersectionSet iY( *this, point, projectionPoint, theTissue );

    ////// casting a ray in +z or -z direction
    projectionPoint = point;
    double upperZ = _boundingBox.getUpperZ();
    double lowerZ = _boundingBox.getLowerZ();
    projectionPoint[ 2 ] = ( upperZ - point[ 2 ] ) <
                           ( point[ 2 ] - lowerZ ) ? upperZ : lowerZ;
    IntersectionSet iZ( *this, point, projectionPoint, theTissue );

    if ( ( iX.count() % 2 ) && ( iY.count() % 2 ) && ( iZ.count() % 2 ) )
    {
      // an odd number of intersections --> inside
      return true;
    }
    else
    {
      // an even number of intersections --> outside
      return false;
    }
    /*
    / need method to handle the case where the point locates on the mesh
    / (need to deal with the numerical approximation problem)
    */
  }
}


void SceneModeller::intersectionAtVoxel( const Eigen::Vector3d& point,
                                         const Eigen::Vector3i& voxel,
                                         const Tissue_ptr& tissue,
                                         Intersection& intersection ) const
{
  auto triangleIndices = tissue->polygonLut().getTriangles( voxel );
  if ( !triangleIndices.empty() )
  {
    auto t = triangleIndices.begin(), te = triangleIndices.end();
    while ( t != te )
    {
      auto mesh = tissue->mesh();
      Eigen::Vector3d newProjectionPoint;
      double newDistance = pointToTriangleDistance(
                                               point,
                                               mesh.vert( mesh.tri( *t )[ 0 ] ),
                                               mesh.vert( mesh.tri( *t )[ 1 ] ),
                                               mesh.vert( mesh.tri( *t )[ 2 ] ),
                                               newProjectionPoint );
      if ( newDistance < intersection._arcLength )
      {
        intersection._arcLength = newDistance;
        intersection._polygonIndex = *t;
        intersection._point = newProjectionPoint;
      }    
      ++ t;
    }
    intersection._tissue = tissue;
  }
}


double SceneModeller::pointToTriangleDistance(
                                        const Eigen::Vector3d& point,
                                        const Eigen::Vector3d& vertex1,
                                        const Eigen::Vector3d& vertex2,
                                        const Eigen::Vector3d& vertex3,
                                        Eigen::Vector3d& projectionPoint ) const
{
  double distance = -1.0;

  //  Projection of a point on a plane
  Eigen::Vector3d v12 = vertex2 - vertex1;
  Eigen::Vector3d v13 = vertex3 - vertex1;
  Eigen::Vector3d v23 = vertex3 - vertex2;
  Eigen::Vector3d normal = v12.cross( v13 );
  if ( std::abs( normal.dot( normal ) ) < EPSILON )
  {
    throw Exception( "triangle normal is a null vector: invalid triangle" );
  }
  normal.normalize();
  double t = normal.dot( vertex1 - point );
  projectionPoint = point + normal * t;
  // checking if the projection point is inside the triangle or on the edge
  if ( ( ( ( projectionPoint - vertex1 ).cross( v12 ) ).
                                               dot( v13.cross( v12 ) ) >= 0 ) &&
       ( ( ( projectionPoint - vertex2 ).cross( v23 ) ).
                                              dot( -v12.cross( v23 ) ) >= 0 ) &&
       ( ( ( projectionPoint - vertex3 ).cross( -v13 ) ).
                                               dot( v23.cross( v13 ) ) >= 0 ) )
  {
    // case: the projection point is inside the triangle or on the edge
    distance = ( t == 0 ) ? 0 : ( point - projectionPoint ).norm();
  }
  else
  {
    // case: the projection point is outside the triangle -> comparing the
    // distances between the projection point and each line segment defined
    // by the vertices
    distance = std::min( std::min(
                        pointToLineSegmentDistance( point, vertex1, vertex2 ),
                        pointToLineSegmentDistance( point, vertex2, vertex3 ) ),
                        pointToLineSegmentDistance( point, vertex3, vertex1 ) );
  }
  if ( distance < 0 )
  {
    throw Exception( "Invalid point to triangle distance: negative distance" );
  }
  return distance;
}


double SceneModeller::pointToLineSegmentDistance(
                                        const Eigen::Vector3d& point,
                                        const Eigen::Vector3d& endPoint1,
                                        const Eigen::Vector3d& endPoint2 ) const
{
  Eigen::Vector3d direction = endPoint2 - endPoint1;
  double t = direction.dot( point - endPoint1 ) / direction.dot( direction );

  double distance = -1.0;
  if ( t <= 0 )
  {
    distance = ( point - endPoint1 ).norm();
  }
  else if ( ( 0 < t ) && ( t < 1 ) )
  {
    distance = ( point - ( endPoint1 + direction * t ) ).norm();
  }
  else if ( t >= 1 )
  {
    distance = ( point - endPoint2 ).norm();
  }

  if ( distance < 0 )
  {
    throw Exception( "Invalid point to segment distance: negative distance" );
  }
  return distance;
}


}

}

}

}


#undef EPSILON

