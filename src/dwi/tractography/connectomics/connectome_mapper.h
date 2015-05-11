#ifndef __dwi_tractography_connectomics_connectome_mapper_h__
#define __dwi_tractography_connectomics_connectome_mapper_h__

#include "math/matrix.h"
#include "dwi/tractography/streamline.h"


namespace MR
{

namespace DWI
{

namespace Tractography
{

namespace Connectomics
{


class ConnectomeMapper
{

  public:

    ConnectomeMapper();
    virtual ~ConnectomeMapper();

    virtual void findNodePair( const Streamline< float >& tck ) = 0;

    void update( std::pair< uint32_t, uint32_t > nodePair );
    void write( const std::string& path );

  protected:

    Math::Matrix< double > _matrix;

};


}

}

}

}


#endif

