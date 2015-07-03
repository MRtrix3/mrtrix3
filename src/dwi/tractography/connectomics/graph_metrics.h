#ifndef __dwi_tractography_connectomics_graph_metrics_h__
#define __dwi_tractography_connectomics_graph_metrics_h__

#include "dwi/tractography/connectomics/connectome.h"


namespace MR
{

namespace DWI
{

namespace Tractography
{

namespace Connectomics
{


class GraphMetrics
{

  public:

    GraphMetrics( Connectome* connectome );
    virtual ~GraphMetrics();

    float density() const;
    std::vector< float > degree() const;
    std::vector< float > strength() const;

    void write( const std::string& path,
                const std::vector< float >& metric ) const;

  protected:

    Connectome* _connectome;

};


}

}

}

}


#endif

