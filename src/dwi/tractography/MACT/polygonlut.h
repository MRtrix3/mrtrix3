/*
 * Copyright (c) 2008-2018 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/
 *
 * MRtrix3 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/
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


// The following PolygonCompare can be moved to Polygon.h as a template struct
struct PolygonCompare : public std::binary_function< Surface::Polygon< 3 >,
                                                     Surface::Polygon< 3 >,
                                                     bool >
{
  bool operator()( const Surface::Polygon< 3 >& p1,
                   const Surface::Polygon< 3 >& p2 ) const
  {
    return ( p1[ 2 ] < p2[ 2 ] ) ||
           ( ( p1[ 2 ] == p2[ 2 ] ) &&
               ( ( p1[ 1 ] < p2[ 1 ] ) ||
                 ( ( p1[ 1 ] == p2[ 1 ] ) && ( p1[ 0 ] < p2[ 0 ] ) ) ) );
  }
};
typedef std::set< Surface::Triangle, PolygonCompare > TriangleSet;


class PolygonLut
{

  public:

    PolygonLut( const Tissue_ptr& tissue );
    virtual ~PolygonLut();

    TriangleSet getTriangles( const Eigen::Vector3i& voxel ) const;
    TriangleSet getTriangles( const Eigen::Vector3d& point ) const;

    TriangleSet getTriangles( const std::set< Eigen::Vector3i, Vector3iCompare >& voxels ) const;
    TriangleSet getTriangles( const std::set< Eigen::Vector3d >& points ) const;

  private:

    Tissue_ptr _tissue;
    std::map< Eigen::Vector3i, TriangleSet, Vector3iCompare > _lut;

};


}

}

}

}

#endif

