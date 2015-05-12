#ifndef __dwi_tractography_connectomics_tckmesh_mapper_h__
#define __dwi_tractography_connectomics_tckmesh_mapper_h__

#include "mesh/scene_modeller.h"
#include "dwi/tractography/connectomics/connectome_mapper.h"


namespace MR
{

namespace DWI
{

namespace Tractography
{

namespace Connectomics
{


class TckMeshMapper : public ConnectomeMapper
{

  public:

    TckMeshMapper( Mesh::SceneModeller* sceneModeller,
                   float distanceLimit );
    virtual ~TckMeshMapper();

    void findNodePair( const Streamline< float >& tck,
                       NodePair& nodePair );

    // functor for supporting multithreading application
    bool operator() ( const Streamline< float >& tck,
                      NodePair& nodePair );

    int32_t getNodeCount() const;
    Point< int32_t > getPolygonIndices(
                                      const Mesh::Polygon< 3 >& polygon ) const;

    int32_t getNodeIndex( const Point< float >& point ) const;
    bool findNode( const Point< float >& point,
                   float& distance,
                   Mesh::SceneMesh*& sceneMesh,
                   Mesh::Polygon< 3 >& polygon ) const;

  protected:

    Mesh::SceneModeller* _sceneModeller;
    float _distanceLimit;
    std::map< Point< int32_t >, int32_t > _polygonLut;

};


}

}

}

}


#endif

