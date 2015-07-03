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
// class ConnectomeMapper
//

ConnectomeMapper::ConnectomeMapper()
{
}


ConnectomeMapper::~ConnectomeMapper()
{
}


//
// class MultiThreadMapper
//

MultiThreadMapper::MultiThreadMapper( ConnectomeMapper* connectomeMapper )
                  : _connectomeMapper( connectomeMapper )
{
}


MultiThreadMapper::~MultiThreadMapper()
{
}


bool MultiThreadMapper::operator() ( const Streamline< float >& tck,
                                     NodePair& nodePair ) const
{

  _connectomeMapper->findNodePair( tck, nodePair );
  return true;

}


}

}

}

}

