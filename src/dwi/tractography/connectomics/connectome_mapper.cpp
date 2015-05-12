#include "dwi/tractography/connectomics/connectome_mapper.h"


namespace MR
{

namespace DWI
{

namespace Tractography
{

namespace Connectomics
{


//
// class NodePair
//

NodePair::NodePair()
{
}


NodePair::~NodePair()
{
}


void NodePair::setNodePair( const int32_t firstNode,
                            const int32_t secondNode )
{

  _nodePair.first = firstNode;
  _nodePair.second = secondNode;

}


const int32_t& NodePair::getFirstNode() const
{

  return _nodePair.first;

}


const int32_t& NodePair::getSecondNode() const
{

  return _nodePair.second;

}


//
// class ConnectomeMapper
//

ConnectomeMapper::ConnectomeMapper()
{
}


ConnectomeMapper::~ConnectomeMapper()
{
}


}

}

}

}

