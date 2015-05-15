#ifndef __dwi_tractography_connectomics_point2mesh_mapper_h__
#define __dwi_tractography_connectomics_point2mesh_mapper_h__

#include "dwi/tractography/connectomics/connectome_mapper.h"
#include "mesh/scene_modeller.h"


namespace MR
{

namespace DWI
{

namespace Tractography
{

namespace Connectomics
{


class Point2MeshMapper : public ConnectomeMapper
{

  public:

    Point2MeshMapper( Mesh::SceneModeller* sceneModeller,
                      float distanceLimit );
    virtual ~Point2MeshMapper();

    void findNodePair( const Streamline< float >& tck,
                       NodePair& nodePair );
    int32_t getNodeCount() const;

  protected:

    Mesh::SceneModeller* _sceneModeller;
    float _distanceLimit;
    std::map< Point< int32_t >, int32_t > _polygonLut;

    Point< int32_t > getPolygonIndices(
                                      const Mesh::Polygon< 3 >& polygon ) const;

    int32_t getNodeIndex( const Point< float >& point ) const;
    bool findNode( const Point< float >& point,
                   float& distance,
                   Mesh::SceneMesh*& sceneMesh,
                   Mesh::Polygon< 3 >& polygon ) const;

};


}

}

}

}


#endif

