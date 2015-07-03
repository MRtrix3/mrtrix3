#include "mesh/scene_mesh_cache.h"
#include "mesh/scene_modeller.h"


namespace MR
{

namespace Mesh
{


SceneMeshCache::SceneMeshCache( SceneModeller* sceneModeller )
               : _sceneModeller( sceneModeller )
{
}


SceneMeshCache::~SceneMeshCache()
{
}


std::vector< SceneMesh* >
SceneMeshCache::getSceneMeshes( const Point< int32_t >& voxel ) const
{

  if ( _lut.count( voxel ) == 0 )
  {

    return std::vector< SceneMesh* >();

  }
  else
  {

    return _lut.find( voxel )->second;

  }

}


std::vector< SceneMesh* >
SceneMeshCache::getSceneMeshes( const Point< float >& point ) const
{

  Point< int32_t > voxel;
  _sceneModeller->getCacheVoxel( point, voxel );

  return getSceneMeshes( voxel );

}


void SceneMeshCache::update( SceneMesh* sceneMesh )
{

  // looping over each polygon (triangle) and use Bresenham's line algorithm to
  // collect the voxels contained in the polygon (triangle) with the
  // consideration of the radius of influence

  // collecting the polygons and associated vertices of the mesh
  VertexList vertices = sceneMesh->getMesh()->getVertices();
  PolygonList polygons = sceneMesh->getMesh()->getPolygons();

  std::set< Point< int32_t >, PointCompare< int32_t > > voxels;
  PolygonList::const_iterator p = polygons.begin(),
                              pe = polygons.end();
  while ( p != pe )
  {

    // collecting vertices of this polygon
    Point< float > v1 = vertices[ ( *p )[ 0 ] ];
    Point< float > v2 = vertices[ ( *p )[ 1 ] ];
    Point< float > v3 = vertices[ ( *p )[ 2 ] ];

    // collecting a set of associated voxels using Bresenham line algorithm
    _sceneModeller->getBresenhamLineAlgorithm().getThickTriangleVoxels(
                  v1, v2, v3, sceneMesh->getRadiusOfInfluence(), voxels, true );

    // looping over each voxel
    std::set< Point< int32_t >, PointCompare< int32_t > >::const_iterator
      v = voxels.begin(),
      ve = voxels.end();
    while ( v != ve )
    {

      if ( _lut.count( *v ) == 0 )
      {

        // LUT does not have this voxel
        // --> add this voxel and initialise a new set of scene meshes
        std::vector< SceneMesh* >
          newSceneMeshList = std::vector< SceneMesh* >();
        newSceneMeshList.push_back( sceneMesh );
        _lut[ *v ] = newSceneMeshList;

      }
      else
      {

        // this voxel already contains some scene meshes
        // --> check if the input scene mesh is already inside the voxel
        bool doesNotExist = true;
        std::vector< SceneMesh* >::const_iterator
          m = _lut[ *v ].begin(),
          me = _lut[ *v ].end();
        while ( m != me )
        {

          if ( ( *m ) == sceneMesh )
          {

            doesNotExist = false;

          }
          ++ m;

        }
        if ( doesNotExist )
        {

          _lut[ *v ].push_back( sceneMesh );

        }

      }
      ++ v;

    }
    ++ p;

  }

}


}

}

