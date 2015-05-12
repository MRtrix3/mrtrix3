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


class NodePair
{

  public:

    NodePair();
    virtual ~NodePair();

    void setNodePair( const int32_t firstNode,
                      const int32_t secondNode );
    const int32_t& getFirstNode() const;    
    const int32_t& getSecondNode() const;

  protected:

    std::pair< int32_t, int32_t > _nodePair;

};


class ConnectomeMapper
{

  public:

    ConnectomeMapper();
    virtual ~ConnectomeMapper();

    virtual void findNodePair( const Streamline< float >& tck,
                               NodePair& nodePair ) = 0;
    // functor for supporting multithreading application
    virtual bool operator() ( const Streamline< float >& tck,
                              NodePair& nodePair ) = 0;

};


}

}

}

}


#endif

