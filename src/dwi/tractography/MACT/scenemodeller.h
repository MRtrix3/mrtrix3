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


#ifndef __dwi_tractography_mact_scenemodeller_h__
#define __dwi_tractography_mact_scenemodeller_h__


#include "dwi/tractography/MACT/bresenhamline.h"
#include "dwi/tractography/MACT/tissuelut.h"
#include "dwi/tractography/MACT/intersectionset.h"


namespace MR
{

namespace DWI
{

namespace Tractography
{

namespace MACT
{


class SceneModeller
{

  public:

    SceneModeller( const BoundingBox< double >& boundingBox,
                   const Eigen::Vector3i& lutSize );
    virtual ~SceneModeller();

    const BoundingBox< double >& boundingBox() const;
    const BoundingBox< int32_t >& integerBoundingBox() const;
    const Eigen::Vector3i& lutSize() const;
    const BresenhamLine& bresenhamLine() const;
    void lutVoxel( const Eigen::Vector3d& point, Eigen::Vector3i& voxel ) const;

    // methods to add tissues
    void addTissues( const std::set< Tissue_ptr >& tissues );
    const TissueLut& tissueLut() const;

    // methods to find the nearest tissue from a given point
    bool nearestTissue( const Eigen::Vector3d& point,
                        struct Intersection& intersection ) const;

    // methods to check whether if a point is inside a closed mesh
    bool insideTissue( const Eigen::Vector3d& point,
                       const std::string& name ) const;

  private:

    BoundingBox< double > _boundingBox;
    BoundingBox< int32_t > _integerBoundingBox;
    Eigen::Vector3i _lutSize;
    BresenhamLine _bresenhamLine;
    std::map< std::string, Tissue_ptr > _tissues;
    TissueLut _tissueLut;

    void intersectionAtVoxel( const Eigen::Vector3d& point,
                              const Eigen::Vector3i& voxel,
                              const Tissue_ptr& tissue,
                              struct Intersection& intersection ) const;
    double pointToTriangleDistance( const Eigen::Vector3d& point,
                                    const Eigen::Vector3d& vertex1,
                                    const Eigen::Vector3d& vertex2,
                                    const Eigen::Vector3d& vertex3,
                                    Eigen::Vector3d& projectionPoint ) const;
    double pointToLineSegmentDistance( const Eigen::Vector3d& point,
                                       const Eigen::Vector3d& endPoint1,
                                       const Eigen::Vector3d& endPoint2 ) const;

};


}

}

}

}

#endif

