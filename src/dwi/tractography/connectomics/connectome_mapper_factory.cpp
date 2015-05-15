#include "dwi/tractography/connectomics/connectome_mapper_factory.h"
#include "dwi/tractography/connectomics/point2mesh_mapper.h"
#include "dwi/tractography/connectomics/ray2mesh_mapper.h"


namespace MR
{

namespace DWI
{

namespace Tractography
{

namespace Connectomics
{


ConnectomeMapperFactory::ConnectomeMapperFactory()
{
}


ConnectomeMapperFactory::~ConnectomeMapperFactory()
{
}


ConnectomeMapper* ConnectomeMapperFactory::getPoint2MeshMapper(
                                             Mesh::SceneModeller* sceneModeller,
                                             float distanceLimit )
{

  ConnectomeMapper* Point2MeshMapper =
    new Connectomics::Point2MeshMapper( sceneModeller, distanceLimit );
  return Point2MeshMapper;

}


ConnectomeMapper* ConnectomeMapperFactory::getRay2MeshMapper(
                                             Mesh::SceneModeller* sceneModeller,
                                             float distanceLimit )
{

  ConnectomeMapper* Ray2MeshMapper =
    new Connectomics::Ray2MeshMapper( sceneModeller, distanceLimit );
  return Ray2MeshMapper;

}


}

}

}

}

