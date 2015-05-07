#ifndef __mesh_scene_mesh_h__
#define __mesh_scene_mesh_h__

#include "mesh/polygon_cache.h"


namespace MR
{


namespace Mesh
{


class SceneModeller;


class SceneMesh
{

  public:

    SceneMesh( SceneModeller* sceneModeller,
               Mesh::Mesh* mesh,
               float radiusOfInfluence = 0.0 );
    virtual ~SceneMesh();

    SceneModeller* getSceneModeller() const;
    Mesh* getMesh() const;
    float getRadiusOfInfluence() const;
    uint32_t getPolygonCount() const;
    const PolygonCache& getPolygonCache() const;

    float getDistanceAtVoxel( const Point< float >& point,
                              const Point< int32_t >& voxel ) const;

    void getClosestPolygonAtLocalVoxel( const Point< float >& point,
                                        float& distance,
                                        Polygon< 3U >& polygon,
                                        Point< float >& projectionPoint ) const;

    void getClosestPolygonAtVoxel( const Point< float >& point,
                                   const Point< int32_t >& voxel,
                                   float& distance,
                                   Polygon< 3U >& polygon,
                                   Point< float >& projectionPoint ) const;

  protected:

    SceneModeller* _sceneModeller;
    Mesh* _mesh;
    float _radiusOfInfluence;
    PolygonCache _polygonCache;

    float getPointToTriangleDistance( const Point< float >& point,
                                      const Point< float >& vertex1,
                                      const Point< float >& vertex2,
                                      const Point< float >& vertex3,
                                      Point< float >& projectionPoint ) const;

    float getPointToLineSegmentDistance( const Point< float >& point,
                                         const Point< float >& endPoint1,
                                         const Point< float >& endPoint2 ) const;

};


}


}

#endif

