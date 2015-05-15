#include "dwi/tractography/connectomics/connectome.h"


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
// class Connectome
//

Connectome::Connectome( const int32_t nodeCount )
{

  _matrix.allocate( nodeCount, nodeCount );

}


Connectome::~Connectome()
{
}


void Connectome::update( const NodePair& nodePair )
{

  int32_t firstNode = nodePair.getFirstNode();
  int32_t secondNode = nodePair.getSecondNode();
  assert( firstNode < data.rows() );
  assert( secondNode < data.rows() );
  assert( firstNode <= secondNode );
  if ( firstNode >= 0 && secondNode >= 0 )
  {

    ++ _matrix( firstNode, secondNode );

  }

}


// functor for multithreading application
bool Connectome::operator() ( const NodePair& nodePair )
{

  int32_t firstNode = nodePair.getFirstNode();
  int32_t secondNode = nodePair.getSecondNode();
  assert( firstNode < data.rows() );
  assert( secondNode < data.rows() );
  assert( firstNode <= secondNode );
  if ( firstNode >= 0 && secondNode >= 0 )
  {

    ++ _matrix( firstNode, secondNode );

  }
  return true;

}


void Connectome::write( const std::string& path )
{

  _matrix.save( path );

}


}

}

}

}

