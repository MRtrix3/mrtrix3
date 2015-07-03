#include "dwi/tractography/connectomics/connectome.h"
#include "file/ofstream.h"
#include "exception.h"


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


void NodePair::setNodePair( const uint32_t& firstNode,
                            const uint32_t& secondNode )
{

  _nodePair.first = firstNode;
  _nodePair.second = secondNode;

}


uint32_t NodePair::getFirstNode() const
{

  return _nodePair.first;

}


uint32_t NodePair::getSecondNode() const
{

  return _nodePair.second;

}


//
// class Connectome
//

Connectome::Connectome()
{
}


Connectome::~Connectome()
{
}


void Connectome::allocate( const uint32_t& nodeCount )
{

  // allocating connectivity matrix
  _sparseMatrix.clear();
  _sparseMatrix.resize( nodeCount );
  for ( uint32_t n = 0; n < nodeCount; n++ )
  {

    _sparseMatrix[ n ] = new std::map< uint32_t, float >();

  }
  _nodeCount = nodeCount;

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


void Connectome::read( const std::string& path )
{

  std::ifstream in( path.c_str(), std::ios_base::in );
  if ( !in )
  {

    throw Exception( "Error opening input file!" );

  }

  std::string line;
  // reading global node count
  std::getline( in, line );
  if ( line.substr( 0, 5 ) == "NODES" )
  {

    line = line.substr( 6 );
    const size_t whiteSpace = line.find( ' ' );
    _nodeCount = std::stoi( line.substr( 0, whiteSpace ) );

  }
  else
  {

    throw Exception( "Unable to find node count in input file!" );

  }
  allocate( _nodeCount );

  // reading node data
  uint32_t row, column;
  float value;
  while ( std::getline( in, line ) && !line.empty() )
  {

    sscanf( line.c_str(), "%u %u %f", &row, &column, &value );
    ( *_sparseMatrix[ row ] )[ column ] = value;

  }

}


void Connectome::write( const std::string& path ) const
{

  File::OFStream out( path, std::ios_base::out | std::ios_base::trunc );

  // writing global node count
  out << "NODES " << _nodeCount << "\n";

  // writing sparse connectome data
  std::map< uint32_t, float >::const_iterator c, ce;
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

