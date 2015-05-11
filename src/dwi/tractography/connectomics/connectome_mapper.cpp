#include "dwi/tractography/connectomics/connectome_mapper.h"


namespace MR
{

namespace DWI
{

namespace Tractography
{

namespace Connectomics
{


ConnectomeMapper::ConnectomeMapper()
{
}


ConnectomeMapper::~ConnectomeMapper()
{
}


void ConnectomeMapper::update( std::pair< uint32_t, uint32_t > nodePair )
{

  if ( nodePair.first < nodePair.second )
  {

    ++ _matrix( nodePair.first, nodePair.second );

  }
  else
  {

    ++ _matrix( nodePair.second, nodePair.first );

  }

}


void ConnectomeMapper::write( const std::string& path )
{

  _matrix.save( path );

}


}

}

}

}

