#ifndef __mesh_scene_mesh_cache_h__
#define __mesh_scene_mesh_cache_h__

#include "mesh/scene_mesh.h"
#include <map>


namespace MR
{


namespace Mesh
{


class SceneModeller;


class SceneMeshCache
{

  public:

    SceneMeshCache( SceneModeller* sceneModeller );
    virtual ~SceneMeshCache();

    std::vector< SceneMesh* >
      getSceneMeshes( const Point< int32_t >& voxel ) const;
    std::vector< SceneMesh* >
      getSceneMeshes( const Point< float >& point ) const;
    
    void update( SceneMesh* SceneMesh );

  protected:

    SceneModeller* _sceneModeller;
    std::map< Point< int32_t >,
              std::vector< SceneMesh* > > _lut;

};


}


}

#endif

