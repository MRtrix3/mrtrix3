#include "mesh/scene_mesh.h"
#include "mesh/scene_modeller.h"


namespace MR
{

namespace Mesh
{


SceneMesh::SceneMesh( SceneModeller* sceneModeller,
                      Mesh* mesh,
                      float radiusOfInfluence )
          : _sceneModeller( sceneModeller ),
            _mesh( mesh ),
            _radiusOfInfluence( radiusOfInfluence ),
            _polygonCache( this )
{
}


SceneMesh::~SceneMesh()
{
}


SceneModeller* SceneMesh::getSceneModeller() const
{

  return _sceneModeller;

}


Mesh* SceneMesh::getMesh() const
{

  return _mesh;

}


float SceneMesh::getRadiusOfInfluence() const
{

  return _radiusOfInfluence;

}


uint32_t SceneMesh::getPolygonCount() const
{

  return ( uint32_t )_mesh->getPolygons().size();

}


const PolygonCache& SceneMesh::getPolygonCache() const
{

  return _polygonCache;

}


float SceneMesh::getDistanceAtVoxel( const Point< float >& point,
                                     const Point< int32_t >& voxel ) const
{

  // initialise an infinite distance
  float distance = std::numeric_limits< float >::infinity();

  // collecting polygons of the mesh in the target voxel
  std::vector< Polygon< 3U > > polygons = _polygonCache.getPolygons( voxel );

  if ( !polygons.empty() )
  {

    float newDistance;
    Point< float > projectionPoint;

    // looping over the polygons
    std::vector< Polygon< 3U > >::const_iterator
      p = polygons.begin(),
      pe = polygons.end();
    while ( p != pe )
    {

      // getting point-to-triangle distance
      newDistance = getPointToTriangleDistance(
                                          point,
                                          _mesh->getVertex( ( *p ).index( 0 ) ),
                                          _mesh->getVertex( ( *p ).index( 1 ) ),
                                          _mesh->getVertex( ( *p ).index( 2 ) ),
                                          projectionPoint );

      if ( newDistance < distance )
      {

        distance = newDistance;

      }
      ++ p;

    }

  }    
  return distance;

}


void SceneMesh::getClosestPolygonAtLocalVoxel(
                                         const Point< float >& point,
                                         float& distance,
                                         Polygon< 3 >& polygon,
                                         Point< float >& projectionPoint ) const
{

  // initialise an infinite distance
  distance = std::numeric_limits< float >::infinity();

  // getting the voxel from the point
  Point< int32_t > voxel;
  _sceneModeller->getCacheVoxel( point, voxel );

  // collecting the polygons in the target voxel
  std::vector< Polygon< 3U > > polygons = _polygonCache.getPolygons( voxel );

  if ( !polygons.empty() )
  {

    float newDistance;
    Point< float > newProjectionPoint;

    // looping over the polygons
    std::vector< Polygon< 3 > >::const_iterator
      p = polygons.begin(),
      pe = polygons.end();
    while ( p != pe )
    {
  
      // getting the point to the triangle distance
      newDistance = getPointToTriangleDistance(
                                          point,
                                          _mesh->getVertex( ( *p ).index( 0 ) ),
                                          _mesh->getVertex( ( *p ).index( 1 ) ),
                                          _mesh->getVertex( ( *p ).index( 2 ) ),
                                          newProjectionPoint );

      if ( newDistance < distance )
      {

        distance = newDistance;
        polygon = *p;
        projectionPoint = newProjectionPoint;

      }
      ++ p;

    }

  }

}


void SceneMesh::getClosestPolygonAtVoxel(
                                         const Point< float >& point,
                                         const Point< int32_t >& voxel,
                                         float& distance,
                                         Polygon< 3 >& polygon,
                                         Point< float >& projectionPoint ) const
{

  // initialise an infinite distance
  distance = std::numeric_limits< float >::infinity();

  // collecting the polygons in the target voxel
  std::vector< Polygon< 3 > > polygons = _polygonCache.getPolygons( voxel );

  if ( !polygons.empty() )
  {

    float newDistance;
    Point< float > newProjectionPoint;

    // looping over the polygons
    std::vector< Polygon< 3 > >::const_iterator
      p = polygons.begin(),
      pe = polygons.end();
    while ( p != pe )
    {

      // getting the point to the triangle distance
      newDistance = getPointToTriangleDistance(
                                          point,
                                          _mesh->getVertex( ( *p ).index( 0 ) ),
                                          _mesh->getVertex( ( *p ).index( 1 ) ),
                                          _mesh->getVertex( ( *p ).index( 2 ) ),
                                          newProjectionPoint );

      if ( newDistance < distance )
      {

        distance = newDistance;
        polygon = *p;
        projectionPoint = newProjectionPoint;

      }    
      ++ p;

    }

  }

}


float SceneMesh::getPointToTriangleDistance(
                                         const Point< float >& point,
                                         const Point< float >& vertex1,
                                         const Point< float >& vertex2,
                                         const Point< float >& vertex3,
                                         Point< float >& projectionPoint ) const
{

  float distance = -1.0;

  ////////////////////////////////////////////////////////////////////////////
  //  Projection of a point on a plane
  ////////////////////////////////////////////////////////////////////////////
  //  plane: A x + B y + C z = k, contains a point v( vx, vy, vz )
  //  
  //  project the point p( px, py, pz ) to the plane
  //  line perpendicular to the plane:
  //    
  //        x = px + A * t
  //        y = py + B * t
  //        z = pz + C * t
  //
  //  thus  A ( px + A * t ) + B ( py + B * t ) + C ( pz + C * t ) = k
  //
  //        t = ( k - ( A px + B py + C pz ) ) / ( A^2 + B^2 + C^2 );
  //  where k = A vx + B vy + C vz
  ////////////////////////////////////////////////////////////////////////////

  Point< float > v12 = vertex2 - vertex1;
  Point< float > v13 = vertex3 - vertex1;
  Point< float > v23 = vertex3 - vertex2;

  Point< float > normal = v12.cross( v13 );

  if ( normal.norm2() == 0 )
  {

    throw std::runtime_error( "normal is a null vector" );

  }
  normal.normalise();

  float t = normal.dot( vertex1 - point );
  projectionPoint = point + normal * t;

  // checking if the projection point is inside the triangle
  if ( ( ( ( projectionPoint - vertex1 ).cross( v12 ) ).
                                               dot( v13.cross( v12 ) ) > 0 ) &&
       ( ( ( projectionPoint - vertex2 ).cross( v23 ) ).
                                               dot( -v12.cross( v23 ) ) > 0 ) &&
       ( ( ( projectionPoint - vertex3 ).cross( -v13 ) ).
                                               dot( v23.cross( v13 ) ) > 0 ) )
  {

    // case: the projection point is inside the triangle
    if ( t == 0 )
    {

      // the point locates on the triangle
      distance = 0;

    }
    else
    {

      distance = ( point - projectionPoint ).norm();

    }

  }
  else
  {

    // case: the projection point is outside the triangle -> comparing the
    // distances between the projection point and each line segment defined
    // by the vertices
    float pointToSegmentDistance = std::min( std::min(
                     getPointToLineSegmentDistance( point, vertex1, vertex2 ),
                     getPointToLineSegmentDistance( point, vertex2, vertex3 ) ),
                     getPointToLineSegmentDistance( point, vertex3, vertex1 ) );

    distance = std::sqrt( ( point - projectionPoint ).norm2() +
                            pointToSegmentDistance * pointToSegmentDistance );

  }

  if ( distance < 0 )
  {

    throw std::runtime_error( "distance should not be negative." );

  }
  return distance;

}


float SceneMesh::getPointToLineSegmentDistance(
                                         const Point< float >& point,
                                         const Point< float >& endPoint1,
                                         const Point< float >& endPoint2 ) const
{

  Point< float > direction = endPoint2 - endPoint1;
  float t = direction.dot( point - endPoint1 ) / direction.norm2();

  float distance = -1.0f;
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

    throw std::runtime_error( "distance should not be negative." );

  }
  return distance;

}


}

}

