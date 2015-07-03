#include "mesh/polygon_cache.h"
#include "mesh/scene_mesh.h"
#include "mesh/scene_modeller.h"


namespace MR
{

namespace Mesh
{


PolygonCache::PolygonCache( SceneMesh* sceneMesh )
             : _sceneMesh( sceneMesh )
{

  // collecting the polygons and associated vertices of the scene mesh
  VertexList vertices = _sceneMesh->getMesh()->getVertices();
  PolygonList polygons = _sceneMesh->getMesh()->getPolygons();

  std::set< Point< int32_t >, PointCompare< int32_t > > voxels;

  PolygonList::const_iterator p = polygons.begin(),
                              pe = polygons.end();
  while ( p != pe )
  {

    // collecting vertices of this polygon
    Point< float > v1 = vertices[ ( *p )[ 0 ] ];
    Point< float > v2 = vertices[ ( *p )[ 1 ] ];
    Point< float > v3 = vertices[ ( *p )[ 2 ] ];

    // collecting a list of associated voxels using Bresenham's line algorithm
    _sceneMesh->getSceneModeller()
              ->getBresenhamLineAlgorithm().getThickTriangleVoxels(
                 v1, v2, v3, _sceneMesh->getRadiusOfInfluence(), voxels, true );

    // then looping over the collected voxels
    std::set< Point< int32_t >,
              PointCompare< int32_t > >::const_iterator
      v = voxels.begin(),
      ve = voxels.end();
    while ( v != ve )
    {

      if ( _lut.count( *v ) )
      {

        // LUT has this voxel --> add this polygon to the voxel
        _lut[ *v ].push_back( *p );

      }
      else
      {

        // LUT does not have this voxel :
        // No polygon exists --> initialise a list of new polygons
        std::vector< Polygon< 3 > >
          newPolygonList = std::vector< Polygon< 3 > >();
        newPolygonList.push_back( *p );
        _lut[ *v ] = newPolygonList;

      }
      ++ v;

    }
    ++ p;

  }

}


PolygonCache::~PolygonCache()
{
}


std::vector< Polygon< 3 > >
PolygonCache::getPolygons( const Point< int32_t >& voxel ) const
{

  if ( _lut.count( voxel ) == 0 )
  {

    return std::vector< Polygon< 3 > >();

  }
  else
  {

    return _lut.find( voxel )->second;

  }

}


std::vector< Polygon< 3 > >
PolygonCache::getPolygons( const Point< float >& point ) const
{

  Point< int32_t > voxel;
  _sceneMesh->getSceneModeller()->getCacheVoxel( point, voxel );

  return getPolygons( voxel );

}


}

}

