#ifndef __dwi_tractography_connectomics_connectome_mapper_factory_h__
#define __dwi_tractography_connectomics_connectome_mapper_factory_h__

#include "singleton.h"
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


class ConnectomeMapperFactory : public Singleton< ConnectomeMapperFactory >
{

  public:

    virtual ~ConnectomeMapperFactory();

    ConnectomeMapper* getPoint2MeshMapper( Mesh::SceneModeller* sceneModeller,
                                           float distanceLimit );

    ConnectomeMapper* getRay2MeshMapper( Mesh::SceneModeller* sceneModeller,
                                         float distanceLimit );

  protected:

    friend class Singleton< ConnectomeMapperFactory >;

    ConnectomeMapperFactory();

};


}

}

}

}


#endif

