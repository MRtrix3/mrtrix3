#ifndef __dwi_tractography_connectomics_connectome_h__
#define __dwi_tractography_connectomics_connectome_h__

#include "math/matrix.h"
#include "dwi/tractography/connectomics/tckmesh_mapper.h"


namespace MR
{

namespace DWI
{

namespace Tractography
{

namespace Connectomics
{


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

