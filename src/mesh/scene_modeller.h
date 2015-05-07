#ifndef __mesh_scene_modeller_h__
#define __mesh_scene_modeller_h__

#include "mesh/bresenham_line_algorithm.h"
#include "mesh/scene_mesh_cache.h"


namespace MR
{


namespace Mesh
{


class SceneModeller
{

  public:

    SceneModeller( const BoundingBox< float >& boundingBox,
                   const Point< int32_t >& cacheSize );
    virtual ~SceneModeller();

    const BoundingBox< float >& getBoundingBox() const;
    const Point< int32_t >& getCacheSize() const;
    const BresenhamLineAlgorithm& getBresenhamLineAlgorithm() const;

    void getCacheVoxel( const Point< float >& point,
                        Point< int32_t >& voxel ) const;

    // methods to add scene meshes
    void addSceneMesh( SceneMesh* sceneMesh );
    int32_t getSceneMeshCount() const;
    SceneMesh* getSceneMesh( int32_t index );
    const SceneMeshCache& getSceneMeshCache() const;

    // methods to find the closest polygon from a given point
    void getClosestMeshPolygon( const Point< float >& point,
                                float& distance,
                                SceneMesh*& sceneMesh,
                                Polygon< 3 >& polygon,
                                Point< float >& projectionPoint ) const;

    bool isInsideSceneMesh( const Point< float >& point ) const;

  protected:

    BoundingBox< float > _boundingBox;
    BoundingBox< int32_t > _integerBoundingBox;
    Point< int32_t > _cacheSize;
    BresenhamLineAlgorithm _bresenhamLineAlgorithm;    
    SceneMeshCache _sceneMeshCache;
    std::vector< SceneMesh* > _meshes;

};


}


}


#endif

