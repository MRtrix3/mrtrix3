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


#ifndef __dwi_tractography_mact_intersectionset_h__
#define __dwi_tractography_mact_intersectionset_h__


#include "dwi/tractography/MACT/scenemodeller.h"


namespace MR
{

namespace DWI
{

namespace Tractography
{

namespace MACT
{


struct Intersection
{

  public:

    Intersection();
    Intersection( double arcLength,
                  const Eigen::Vector3d& point,
                  const Tissue_ptr& tissue,
                  Surface::Triangle triangle );

    Intersection( const Intersection& other );
    Intersection& operator=( const Intersection& other );

    size_t nearestVertex() const;

    double _arcLength;
    Eigen::Vector3d _point;
    Tissue_ptr _tissue;
    Surface::Triangle _triangle;

};


class IntersectionSet
{

  public:

    IntersectionSet( const SceneModeller& sceneModeller,
                     const Eigen::Vector3d& from,
                     const Eigen::Vector3d& to );
    IntersectionSet( const SceneModeller& sceneModeller,
                     const Eigen::Vector3d& from,
                     const Eigen::Vector3d& to,
                     const Tissue_ptr& targetTissue );
    virtual ~IntersectionSet();

    size_t count() const;
    const Intersection& intersection( size_t index ) const;

  private:

    bool rayTriangleIntersection( const Eigen::Vector3d& from,
                                  const Eigen::Vector3d& to,
                                  const Eigen::Vector3d& vertex1,
                                  const Eigen::Vector3d& vertex2,
                                  const Eigen::Vector3d& vertex3, 
                                  Eigen::Vector3d& intersectionPoint ) const;

    std::vector< double > _arcLengths;
    std::map< double, Intersection > _intersections;

};


}

}

}

}

#endif

