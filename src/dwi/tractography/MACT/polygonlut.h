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


#ifndef __dwi_tractography_mact_polygonlut_h__
#define __dwi_tractography_mact_polygonlut_h__


#include "dwi/tractography/MACT/keycomp.h"
#include "surface/mesh.h"
#include <set>


namespace MR
{

namespace DWI
{

namespace Tractography
{

namespace MACT
{


class Tissue;

typedef std::shared_ptr< Tissue > Tissue_ptr; /*tissue pointer*/
typedef uint32_t Polygon_i; /*polygon index*/


class PolygonLut
{

  public:

    PolygonLut( const Tissue_ptr& tissue );
    virtual ~PolygonLut();

    std::set< Polygon_i > getTriangles( const Eigen::Vector3i& voxel ) const;
    std::set< Polygon_i > getTriangles( const Eigen::Vector3d& point ) const;

  private:

    Tissue_ptr _tissue;
    std::map< Eigen::Vector3i, std::set< Polygon_i >, Vector3iCompare > _lut;

};


}

}

}

}

#endif

