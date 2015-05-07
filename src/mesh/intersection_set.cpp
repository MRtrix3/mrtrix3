#include "mesh/intersection_set.h"

#define EPSILON 1e-20


namespace MR
{


namespace Mesh
{


//
// struct Intersection
//

Intersection::Intersection()
             : _arcLength( -1.0 )
{
}


Intersection::Intersection( float theArcLength,
                            const Point< float >& thePoint,
                            SceneMesh* theSceneMesh,
                            const Polygon< 3 >& thePolygon )
             : _arcLength( theArcLength ),
               _point( thePoint ),
               _sceneMesh( theSceneMesh ),
               _polygon( thePolygon )
{
}


Intersection::Intersection( const Intersection& other )
             : _arcLength( other._arcLength ),
               _point( other._point ),
               _sceneMesh( other._sceneMesh ),
               _polygon( other._polygon )
{
}


Intersection&
Intersection::operator=( const Intersection& other )
{

  _arcLength = other._arcLength;
  _point = other._point;
  _sceneMesh = other._sceneMesh;
  _polygon = other._polygon;

  return *this;

}


//
// class IntersectionSet
//

IntersectionSet::IntersectionSet( const SceneModeller& sceneModeller,
                                  const Point< float >& from,
                                  const Point< float >& to )
{

  // Collecting the voxels where the ray is passing through
  std::set< Point< int32_t >,
            PointCompare< int32_t > > voxels;
  sceneModeller.getBresenhamLineAlgorithm().getRayVoxels( from, to, voxels,
                                                          false );

  // calculating from->to vector and its norm
  Point< float > fromTo = to - from;
  float fromToLength = fromTo.norm();

  // looping over voxels and polygons to get interseting points & arc lengths
  std::set< float > arcLengths;
  Point< float > v1, v2, v3;
  Point< float > intersectionPoint;
  Point< float > fromIntersection;
  float fromIntersectionLength;
  bool hasIntersection = false;
  std::set< Point< int32_t >,
            PointCompare< int32_t > >::const_iterator
    v = voxels.begin(),
    ve = voxels.end();
  while ( v != ve )
  {

    // collecting the list of meshes crossing this voxel
    std::vector< SceneMesh* >
      sceneMeshes = sceneModeller.getSceneMeshCache().getSceneMeshes( *v );

    // if the list is not empty
    if ( !sceneMeshes.empty() )
    {

      // looping over the scene meshes
      std::vector< SceneMesh* >::const_iterator
        m = sceneMeshes.begin(),
        me = sceneMeshes.end();
      while ( m != me )
      {

        // collecting the list of polygons corresponding to the current mesh
        // inside the current voxel
        VertexList vertices = ( *m )->getMesh()->vertices;
        std::vector< Polygon< 3 > > 
          polygons = ( *m )->getPolygonCache().getPolygons( *v );

        // looping over the polygons
        std::vector< Polygon< 3 > >::const_iterator
          p = polygons.begin(),
          pe = polygons.end();
        while ( p != pe )
        {

          // obtaining the current vertex positions of the polygon
          v1 = vertices[ ( *p ).indices[ 0 ] ];
          v2 = vertices[ ( *p ).indices[ 1 ] ];
          v3 = vertices[ ( *p ).indices[ 2 ] ];

          // perform ray/triangle intersection to find intersecting point
          hasIntersection = getRayTriangleIntersection( from, to,
                                                        v1, v2, v3,
                                                        intersectionPoint );

          if ( hasIntersection )
          {

            // computing from->intersection vector
            fromIntersection = ( intersectionPoint - from );
            fromIntersectionLength = fromIntersection.norm();

            // check whether the intersecting point is in the segment of
            // [ from , to )
            // if it belongs to the segment: add the arc length, the point,
            // the polygon and the membrane to 'Intersection'
            if ( ( fromIntersectionLength < fromToLength ) &&
                 ( fromIntersection.dot( fromTo ) > 0.0 ) )
            {

              Intersection intersection( fromIntersectionLength,
                                         intersectionPoint,
                                         *m, *p );
              arcLengths.insert( fromIntersectionLength );
              _intersections[ fromIntersectionLength ] = intersection;

            }

          }
          ++ p;

        }
        ++ m;

      }

    }
    ++ v;

  }

  // copying the arclength set to a vector
  _arcLengths.resize( arcLengths.size() );
  std::set< float >::const_iterator a = arcLengths.begin(),
                                    ae = arcLengths.end();
  int32_t index = 0;
  while ( a != ae )
  {

    _arcLengths[ index ] = *a;
    ++ a;
    ++ index;

  }

}


IntersectionSet::~IntersectionSet()
{
}


int32_t IntersectionSet::getCount() const
{

  return ( int32_t )_arcLengths.size();

}


const Intersection& IntersectionSet::getIntersection( int32_t index ) const
{

  std::map< float, Intersection >::const_iterator
    i = _intersections.find( _arcLengths[ index ] );
  if ( i == _intersections.end() )
  {

    throw std::runtime_error( "intersection not found" );

  }
  return i->second;

}


void IntersectionSet::eraseIntersection( int32_t index )
{

  _intersections.erase( _arcLengths[ index ] );
  _arcLengths.erase( _arcLengths.begin() + index );

}


bool IntersectionSet::getRayTriangleIntersection(
                                       const Point< float >& from,
                                       const Point< float >& to,
                                       const Point< float >& vertex1,
                                       const Point< float >& vertex2,
                                       const Point< float >& vertex3, 
                                       Point< float >& intersectionPoint ) const
{

  Point< float > u = vertex2 - vertex1;
  Point< float > v = vertex3 - vertex1;
  Point< float > n = u.cross( v );
  if ( n.norm2() < EPSILON )
  {

    return false;

  }
  n.normalise();

  Point< float > rayDirection = to - from;
  Point< float > w0 = from - vertex1;

  float a = -n.dot( w0 );
  float b = n.dot( rayDirection );
  // ray is parallel to triangle plane
  if ( b == 0.0f )
  {

    // ray lies in triangle plane
    if ( a == 0.0f )
    {

      return false;

    }
    // ray disjoint from plane
    else
    {

      return false;

    }

  }

  float r = a / b;
  if ( r < 0.0 )
  {

    return false;

  }

  // processing ray/triangle plane intersection
  intersectionPoint = from + rayDirection * r;

  // is intersection inside the triangle ?
  float uu = u.dot( u );
  float uv = u.dot( v );
  float vv = v.dot( v );
  Point< float > w = intersectionPoint - vertex1;
  float wu = w.dot( u );
  float wv = w.dot( v );
  float D = uv * uv - uu * vv;
 
  // get and test parametric coords
  float s = ( uv * wv - vv * wu ) / D;

  // intersection is outside of the triangle
  if ( ( s < 0.0 ) || ( s > 1.0 ) )
  {

    return false;

  }
  float t = ( uv * wu - uu * wv ) / D;
  // intersection is outside of the triangle
  if ( ( t < 0.0 ) || ( ( s + t ) > 1.0 ) )
  {

    return false;

  }
 
  return true;

}


}

}


#undef EPSILON

