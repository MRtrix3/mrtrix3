#ifndef __dwi_tractography_connectomics_connectome_h__
#define __dwi_tractography_connectomics_connectome_h__

#include <map>
#include <vector>


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

    void setNodePair( const uint32_t& firstNode,
                      const uint32_t& secondNode );
    uint32_t getFirstNode() const;    
    uint32_t getSecondNode() const;

  protected:

    std::pair< uint32_t, uint32_t > _nodePair;

};


class Connectome
{

  public:

    Connectome();
    virtual ~Connectome();

    void allocate( const uint32_t& nodeCount );
    void update( const NodePair& nodePair );
    // functor for multi-thread
    bool operator() ( const NodePair& nodePair );

    void read( const std::string& path );
    void write( const std::string& path ) const;

  protected:

    friend class GraphMetrics;

    uint32_t _nodeCount;
    std::vector< std::map< uint32_t, float >* > _sparseMatrix;

};


}

}

}

}


#endif

