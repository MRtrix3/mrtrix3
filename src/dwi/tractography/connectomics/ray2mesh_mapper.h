#ifndef __dwi_tractography_connectomics_ray2mesh_mapper_h__
#define __dwi_tractography_connectomics_ray2mesh_mapper_h__

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


class Ray2MeshMapper : public ConnectomeMapper
{

  public:

    Ray2MeshMapper( Mesh::SceneModeller* sceneModeller,
                    float distanceLimit );
    virtual ~Ray2MeshMapper();

    void findNodePair( const Streamline< float >& tck,
                       NodePair& nodePair );
    uint32_t getNodeCount() const;

  protected:

    Mesh::SceneModeller* _sceneModeller;
    float _distanceLimit;
    std::map< Point< int32_t >, int32_t > _polygonLut;

    uint32_t getNodeIndex( const Point< float >& from,
                           const Point< float >& to ) const;

};


}

}

}

}


#endif

