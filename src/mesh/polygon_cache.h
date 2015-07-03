#ifndef __mesh_polygon_cache_h__
#define __mesh_polygon_cache_h__

#include "mesh/mesh.h"
#include <map>


namespace MR
{


namespace Mesh
{


class SceneMesh;


class PolygonCache
{

  public:

    PolygonCache( SceneMesh* sceneMesh );
    virtual ~PolygonCache();

    std::vector< Polygon< 3 > > 
      getPolygons( const Point< int32_t >& voxel ) const;
    std::vector< Polygon< 3 > >
      getPolygons( const Point< float >& point ) const;

  protected:

    SceneMesh* _sceneMesh;
    std::map< Point< int32_t >,
              std::vector< Polygon< 3 > > > _lut;

};


}


}

#endif

