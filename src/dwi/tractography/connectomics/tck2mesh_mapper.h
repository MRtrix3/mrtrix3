#ifndef __dwi_tractography_connectomics_tck2mesh_mapper_h__
#define __dwi_tractography_connectomics_tck2mesh_mapper_h__

#include "mesh/scene_modeller.h"
#include "dwi/tractography/streamline.h"
#include "math/matrix.h"


namespace MR
{

namespace DWI
{

namespace Tractography
{

namespace Connectomics
{


class Tck2MeshMapper
{

  public:

    Tck2MeshMapper( Mesh::SceneModeller* sceneModeller,
                    float distanceLimit );
    virtual ~Tck2MeshMapper();

    void update( const Streamline< float >& tck );

    void write( const std::string& path );

  protected:

    Mesh::SceneModeller* _sceneModeller;
    float _distanceLimit;

    std::map< Point< int32_t >, int32_t > _polygonLut;
    Math::Matrix< double > _matrix;

    Point< int32_t > getPolygonIndices(
                                      const Mesh::Polygon< 3 >& polygon ) const;
    int32_t getNodeIndex( const Point< float >& point ) const;

};


}

}

}

}


#endif

