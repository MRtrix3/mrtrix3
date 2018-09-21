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


#include "dwi/tractography/MACT/pointtrianglemath.h"
#include "exception.h"


namespace MR
{

namespace DWI
{

namespace Tractography
{

namespace MACT
{


bool pointInTriangle( const Eigen::Vector3d& point,
                      const Eigen::Vector3d& vertex1,
                      const Eigen::Vector3d& vertex2,
                      const Eigen::Vector3d& vertex3 )
{
  // the test point must be coplanar with the triangle
  Eigen::Vector3d u = vertex2 - vertex1;
  Eigen::Vector3d v = vertex3 - vertex1;
  Eigen::Vector3d w = point - vertex1;

  // compute barycentric coordinates
  double uu = u.dot( u );
  double uv = u.dot( v );
  double uw = u.dot( w );
  double vv = v.dot( v );
  double vw = v.dot( w );  
  double D = 1.0 / ( uu * vv - uv * uv );
  double s = ( vv * uw - uv * vw ) * D;
  if ( ( s < 0.0 ) || ( s > 1.0 ) )
  {
    return false;
  }
  double t = ( uu * vw - uv * uw ) * D;
  if ( ( t < 0.0 ) || ( ( s + t ) > 1.0 ) )
  {
    return false;
  }
  return true;
}


bool projectionPointInTriangle( const Eigen::Vector3d& point,
                                const Eigen::Vector3d& vertex1,
                                const Eigen::Vector3d& vertex2,
                                const Eigen::Vector3d& vertex3,
                                Eigen::Vector3d& projectionPoint )
{
  // get the projection point onto the plane defined by the triangle
  Eigen::Vector3d u = vertex2 - vertex1;
  Eigen::Vector3d v = vertex3 - vertex1;
  Eigen::Vector3d n = u.cross( v );
  n.normalize();
  double r = n.dot( vertex1 - point );
  projectionPoint = point + n * r;

  // compute barycentric coordinates
  Eigen::Vector3d w = projectionPoint - vertex1;
  double uu = u.dot( u );
  double uv = u.dot( v );
  double uw = u.dot( w );
  double vv = v.dot( v );
  double vw = v.dot( w );  
  double D = 1.0 / ( uu * vv - uv * uv );
  double s = ( vv * uw - uv * vw ) * D;
  if ( ( s < 0.0 ) || ( s > 1.0 ) )
  {
    return false;
  }
  double t = ( uu * vw - uv * uw ) * D;
  if ( ( t < 0.0 ) || ( ( s + t ) > 1.0 ) )
  {
    return false;
  }
  return true;
}


double pointToLineSegmentDistance( const Eigen::Vector3d& point,
                                   const Eigen::Vector3d& endPoint1,
                                   const Eigen::Vector3d& endPoint2 )
{
  Eigen::Vector3d r = endPoint2 - endPoint1;

  Eigen::Vector3d u = point - endPoint1;
  double ru = r.dot( u );
  if ( ru <= 0.0 )
  {
    // closest point is endpoint1
    return u.norm();
  }

  Eigen::Vector3d v = point - endPoint2;
  if ( r.dot( v ) >= 0.0 )
  {
    // closest point is endpoint2
    return v.norm();
  }

  // closest point lies in the line segment
  double d = 1.0 / r.dot( r );
  Eigen::Vector3d s = u - r * ru * d;
  return s.norm();
}


double pointToTriangleDistance( const Eigen::Vector3d& point,
                                const Eigen::Vector3d& vertex1,
                                const Eigen::Vector3d& vertex2,
                                const Eigen::Vector3d& vertex3,
                                Eigen::Vector3d& projectionPoint )
{
  // checking if the projection point is inside the triangle or on the edge
  if ( projectionPointInTriangle( point, vertex1, vertex2, vertex3,
                                  projectionPoint ) )
  {
    // case: the projection point is inside the triangle or on the edge
    return ( point - projectionPoint ).norm();
  }
  else
  {
    // case: the projection point is outside the triangle -> comparing the
    // distances between the point and the triangle edges
    return std::min( std::min(
                        pointToLineSegmentDistance( point, vertex1, vertex2 ),
                        pointToLineSegmentDistance( point, vertex2, vertex3 ) ),
                        pointToLineSegmentDistance( point, vertex3, vertex1 ) );
  }
}


}

}

}

}

