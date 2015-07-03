#ifndef __dwi_tractography_connectomics_connectome_mapper_h__
#define __dwi_tractography_connectomics_connectome_mapper_h__

#include "dwi/tractography/streamline.h"
#include "dwi/tractography/connectomics/connectome.h"


namespace MR
{

namespace DWI
{

namespace Tractography
{

namespace Connectomics
{


class ConnectomeMapper /* common interface */
{

  public:

    ConnectomeMapper();
    virtual ~ConnectomeMapper();

    virtual void findNodePair( const Streamline< float >& tck,
                               NodePair& nodePair ) const = 0;
    virtual uint32_t getNodeCount() const = 0;

};


class MultiThreadMapper /* providing functor for supporting multithreading */
{

  public:

    MultiThreadMapper( ConnectomeMapper* connectomeMapper );
    virtual ~MultiThreadMapper();

    bool operator() ( const Streamline< float >& tck,
                      NodePair& nodePair ) const;

  protected:

    ConnectomeMapper* _connectomeMapper;

};


}

}

}

}


#endif

