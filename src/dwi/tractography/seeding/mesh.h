/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 * 
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * For more details, see www.mrtrix.org
 * 
 */


#ifndef __dwi_tractography_seeding_mesh_h__
#define __dwi_tractography_seeding_mesh_h__


#include "dwi/tractography/seeding/base.h"
#include "surface/mesh.h"


namespace MR
{

namespace DWI
{

namespace Tractography
{

namespace Seeding
{


class Mesh : public Base
{

  public:

    Mesh( const std::string& in );
    virtual ~Mesh();

    bool get_seed( Eigen::Vector3f& point ) const;

  private:

    Surface::Mesh _mesh;
    std::vector< size_t > _indices;
    std::vector< double > _cdf;

    double calculate_area( const Surface::Vertex& v1,
                           const Surface::Vertex& v2,
                           const Surface::Vertex& v3 ) const;

    void sort_index_descend( const std::vector< double >& values,
                             std::vector< size_t >& indices ) const;

};


}

}

}

}

#endif

