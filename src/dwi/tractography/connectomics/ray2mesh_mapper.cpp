#include "dwi/tractography/connectomics/ray2mesh_mapper.h"
#include "mesh/intersection_set.h"


namespace MR
{

namespace DWI
{

namespace Tractography
{

namespace Connectomics
{


Ray2MeshMapper::Ray2MeshMapper( Mesh::SceneModeller* sceneModeller,
                                float distanceLimit )
               : _sceneModeller( sceneModeller ),
                 _distanceLimit( distanceLimit )
{

  // preparing index = 0 for failing to find an associated polygon
  uint32_t polygonIndex = 0;
  _polygonLut[ Point< int32_t >( 0, 0, 0 ) ] = polygonIndex;
  ++ polygonIndex;

  // preparing a polygon lut
  int32_t polygonCount = 0;
  int32_t sceneMeshCount = _sceneModeller->getSceneMeshCount();
  for ( int32_t m = 0; m < sceneMeshCount; m++ )
  {

    Mesh::SceneMesh* currentSceneMesh = _sceneModeller->getSceneMesh( m );
    Mesh::PolygonList polygons = currentSceneMesh->getMesh()->polygons;
    polygonCount = currentSceneMesh->getPolygonCount();
    for ( int32_t p = 0; p < polygonCount; p++ )
    {

      _polygonLut[ Point< int32_t >( polygons[ p ].indices[ 0 ],
                                     polygons[ p ].indices[ 1 ],
                                     polygons[ p ].indices[ 2 ] )
                  ] = polygonIndex;
      ++ polygonIndex;

    }

  }

}


Ray2MeshMapper::~Ray2MeshMapper()
{
}


void Ray2MeshMapper::findNodePair( const Streamline< float >& tck,
                                   NodePair& nodePair )
{

  //
  // This is a naive implementation (for testing purpose) of finding possible 
  // intersection between a streamline and a mesh. It simply casts a ray from
  // streamline endpoint along the direction defined by the first two (or last
  // two) points of the streamline.
  //

  // casting a ray from one endpoint ( tck[ 0 ] )
  Point< float > rayDirection = ( tck[ 0 ] - tck[ 1 ] ).normalise();
  Point< float > to = tck[ 0 ] + rayDirection * _distanceLimit;
  uint32_t node1 = getNodeIndex( tck[ 0 ], to );

  // casting a ray from the other endpoint ( tck[ tck.size() ] )
  int32_t pointCount = tck.size();
  rayDirection = ( tck[ pointCount ] - tck[ pointCount - 1 ] ).normalise();
  to = tck[ pointCount ] + rayDirection * _distanceLimit;
  uint32_t node2 = getNodeIndex( tck[ pointCount ], to );

  // setting the node pair
  if ( node1 <= node2 )
  {

    nodePair.setNodePair( node1, node2 );

  }
  else
  {

    nodePair.setNodePair( node2, node1 );

  }

}


uint32_t Ray2MeshMapper::getNodeCount() const
{

  return _polygonLut.size();

}


uint32_t Ray2MeshMapper::getNodeIndex( const Point< float >& from,
                                       const Point< float >& to ) const
{

  Mesh::IntersectionSet intersectionSet( *_sceneModeller, from, to );
  if ( intersectionSet.getCount() )
  {

    Mesh::Polygon< 3 > polygon = intersectionSet.getIntersection( 0 )._polygon;
    return _polygonLut.find( Point< int32_t >( polygon.indices[ 0 ],
                                               polygon.indices[ 1 ],
                                               polygon.indices[ 2 ] ) )->second;

  }
  else
  {

    return 0;

  }

}


}

}

}

}

