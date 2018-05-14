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


#ifndef __dwi_tractography_mact_bresenhamline_h__
#define __dwi_tractography_mact_bresenhamline_h__


#include "dwi/tractography/MACT/boundingbox.h"
#include "dwi/tractography/MACT/keycomp.h"
#include <set>


namespace MR
{

namespace DWI
{

namespace Tractography
{

namespace MACT
{


class BresenhamLine
{

  public:

    BresenhamLine( const BoundingBox< double >& boundingBox,
                   const Eigen::Vector3i& cacheSize );
    virtual ~BresenhamLine();

    const Eigen::Vector3d& resolution() const;
    double minResolution() const;

    void point2voxel( const Eigen::Vector3d& point,
                      Eigen::Vector3i& voxel ) const;

    void neighbouringVoxels( const Eigen::Vector3i& voxel,
                             const int32_t& stride,
                             std::set< Eigen::Vector3i, Vector3iCompare >& voxels,
                             bool clearVoxelsAtBeginning = true ) const;

    void rayVoxels( const Eigen::Vector3d& from,
                    const Eigen::Vector3d& to,
                    std::set< Eigen::Vector3i, Vector3iCompare >& voxels,
                    bool clearVoxelsAtBeginning = true ) const;

    void triangleVoxels( const Eigen::Vector3d& vertex1,
                         const Eigen::Vector3d& vertex2,
                         const Eigen::Vector3d& vertex3,
                         std::set< Eigen::Vector3i, Vector3iCompare >& voxels,
                         bool clearVoxelsAtBeginning = true ) const;

    void discTriangleVoxels( const Eigen::Vector3d& vertex1,
                             const Eigen::Vector3d& vertex2,
                             const Eigen::Vector3d& vertex3,
                             double radiusOfInfluence,
                             std::set< Eigen::Vector3i, Vector3iCompare >& voxels,
                             bool clearVoxelsAtBeginning = true ) const;

  private:

    Eigen::Vector3i _cacheSize;
    Eigen::Vector3i _cacheSizeMinusOne;
    double _lowerX;
    double _upperX;
    double _lowerY;
    double _upperY;
    double _lowerZ;
    double _upperZ;
    double _cacheVoxelFactorX;
    double _cacheVoxelFactorY;
    double _cacheVoxelFactorZ;
    Eigen::Vector3d _resolution;
    double _minResolution;

};


}

}

}

}

#endif

