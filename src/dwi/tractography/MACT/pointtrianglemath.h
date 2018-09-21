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


#ifndef __dwi_tractography_mact_pointtrianglemath_h__
#define __dwi_tractography_mact_pointtrianglemath_h__


#include "types.h"


namespace MR
{

namespace DWI
{

namespace Tractography
{

namespace MACT
{


bool pointInTriangle( const Eigen::Vector3d& point,
                      const Eigen::Vector3d& vertex1,
                      const Eigen::Vector3d& vertex2,
                      const Eigen::Vector3d& vertex3 );

bool projectionPointInTriangle( const Eigen::Vector3d& point,
                                const Eigen::Vector3d& vertex1,
                                const Eigen::Vector3d& vertex2,
                                const Eigen::Vector3d& vertex3,
                                Eigen::Vector3d& projectionPoint );

double pointToLineSegmentDistance( const Eigen::Vector3d& point,
                                   const Eigen::Vector3d& endPoint1,
                                   const Eigen::Vector3d& endPoint2 );

double pointToTriangleDistance( const Eigen::Vector3d& point,
                                const Eigen::Vector3d& vertex1,
                                const Eigen::Vector3d& vertex2,
                                const Eigen::Vector3d& vertex3,
                                Eigen::Vector3d& projectionPoint );


}

}

}

}

#endif

