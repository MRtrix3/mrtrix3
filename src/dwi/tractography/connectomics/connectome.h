#ifndef __dwi_tractography_connectomics_connectome_h__
#define __dwi_tractography_connectomics_connectome_h__

#include "math/matrix.h"


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


class Connectome
{

  public:

    Connectome( const int32_t nodeCount );
    virtual ~Connectome();

    void update( const NodePair& nodePair );
    // functor for multi-thread
    bool operator() ( const NodePair& nodePair );

    void write( const std::string& path );

  protected:

    Math::Matrix< float > _matrix;

};


}

}

}

}


#endif

