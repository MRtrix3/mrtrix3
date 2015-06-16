#include "dwi/tractography/connectomics/connectome.h"
#include "file/ofstream.h"


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


void NodePair::setNodePair( const uint32_t firstNode,
                            const uint32_t secondNode )
{

  _nodePair.first = firstNode;
  _nodePair.second = secondNode;

}


const uint32_t& NodePair::getFirstNode() const
{

  return _nodePair.first;

}


const uint32_t& NodePair::getSecondNode() const
{

  return _nodePair.second;

}


//
// class Connectome
//

Connectome::Connectome( const uint32_t nodeCount )
           : _nodeCount( nodeCount )
{

  // allocating connectivity matrix
  _sparseMatrix.resize( nodeCount );
  for ( uint32_t n = 0; n < nodeCount; n++ )
  {

    _sparseMatrix[ n ] = new std::map< uint32_t, uint32_t >();

  }

}


Connectome::~Connectome()
{
}


void Connectome::update( const NodePair& nodePair )
{

  uint32_t row = nodePair.getFirstNode();
  uint32_t column = nodePair.getSecondNode();
  ++ ( *_sparseMatrix[ row ] )[ column ];

}


// functor for multithreading application
bool Connectome::operator() ( const NodePair& nodePair )
{

  uint32_t row = nodePair.getFirstNode();
  uint32_t column = nodePair.getSecondNode();
  ++ ( *_sparseMatrix[ row ] )[ column ];
  return true;

}


void Connectome::write( const std::string& path )
{

  File::OFStream out( path );
  std::map< uint32_t, uint32_t >::const_iterator c, ce;
  for ( uint32_t r = 0; r < _nodeCount; r++ )
  {

    c = _sparseMatrix[ r ]->begin(),
    ce = _sparseMatrix[ r ]->end();
    while ( c != ce )
    {

      out << r << " " << c->first << " " << c->second << "\n";
      ++ c;

    }

  }

}


}

}

}

}

