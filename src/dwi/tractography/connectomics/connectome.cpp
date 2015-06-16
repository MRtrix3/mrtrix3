#include "dwi/tractography/connectomics/connectome.h"
#include "file/ofstream.h"
#include <map>


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

  _matrix.allocate( nodeCount, nodeCount );

}


Connectome::~Connectome()
{
}


void Connectome::update( const NodePair& nodePair )
{

  ++ _matrix( nodePair.getFirstNode(), nodePair.getSecondNode() );

}


// functor for multithreading application
bool Connectome::operator() ( const NodePair& nodePair )
{

  ++ _matrix( nodePair.getFirstNode(), nodePair.getSecondNode() );
  return true;

}


void Connectome::write( const std::string& path )
{

  File::OFStream out( path );

  std::vector< std::map< uint32_t, uint32_t > > sparseMatrix( _nodeCount );
  std::map< uint32_t, uint32_t > rowMap;
  for ( uint32_t row = 0; row < _nodeCount; row++ )
  {

    Math::Matrix< float >::VectorView rowVector = _matrix.row( row );
    for ( uint32_t column = row; column < _nodeCount; column++ )
    {

      if ( rowVector[ column ] )
      {

        rowMap[ column ] = rowVector[ column ];
        //out << row << " " << column << " " << rowVector[ column ] << "\n";

      }

    }
    sparseMatrix[ row ] = rowMap;
    rowMap.clear();

  }

}


}

}

}

}

