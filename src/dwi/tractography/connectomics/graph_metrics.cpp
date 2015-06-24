#include "dwi/tractography/connectomics/graph_metrics.h"
#include "file/ofstream.h"
#include <set>
#include <iostream>


namespace MR
{

namespace DWI
{

namespace Tractography
{

namespace Connectomics
{


GraphMetrics::GraphMetrics( Connectome* connectome )
             : _connectome( connectome )
{
}


GraphMetrics::~GraphMetrics()
{
}


float GraphMetrics::density() const
{

  // computing egde count without unknown (node index = 0 ) & diagonal elements
  uint32_t globalEdgeCount = ( _connectome->_nodeCount - 3 ) *
                               _connectome->_nodeCount / 2 + 1;

  // computing number of non-zero edges
  uint32_t nonzeroEdgeCount = 0;
  for ( uint32_t n = 1; n < _connectome->_nodeCount; n++ )
  {

    std::map< uint32_t, float >* rowMap = _connectome->_sparseMatrix[ n ];
    if ( !rowMap->empty() )
    {

      if ( rowMap->find( n ) != rowMap->end() )
      {

        // current row has self-connections
        nonzeroEdgeCount += rowMap->size() - 1;

      }
      else
      {

        nonzeroEdgeCount += rowMap->size();

      }

    }

  }
  return ( float )nonzeroEdgeCount / ( float )globalEdgeCount;

}


std::vector< float > GraphMetrics::degree() const
{

  std::vector< float > node_degree( _connectome->_nodeCount, 0.0 );
  for ( uint32_t n = 0; n < _connectome->_nodeCount; n++ )
  {

    node_degree[ n ] = ( _connectome->_sparseMatrix[ n ] )->size();

  }
  return node_degree;

}


std::vector< float > GraphMetrics::strength() const
{

  std::vector< float > node_strength( _connectome->_nodeCount, 0.0 );
  for ( uint32_t n = 0; n < _connectome->_nodeCount; n++ )
  {

    std::map< uint32_t, float >* rowMap = _connectome->_sparseMatrix[ n ];
    if ( !rowMap->empty() )
    {

      std::map< uint32_t, float >::const_iterator
        m = rowMap->begin(),
        me = rowMap->end();
      while ( m != me )
      {

        node_strength[ n ] += ( *m ).second;
        ++ m;

      }

    }

  }
  return node_strength;

}


void GraphMetrics::write( const std::string& path,
                          const std::vector< float >& metric )
{

  File::OFStream out( path, std::ios_base::out | std::ios_base::trunc );
  for ( uint32_t m = 0; m < metric.size(); m++ )
  {

    out << metric[ m ] << "\n";

  }

}


}

}

}

}

