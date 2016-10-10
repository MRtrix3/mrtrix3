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


#ifndef __dwi_tractography_mact_boundingbox_h__
#define __dwi_tractography_mact_boundingbox_h__


#include "types.h"


namespace MR
{

namespace DWI
{

namespace Tractography
{

namespace MACT
{


template < class T >
class BoundingBox
{

  public:

    BoundingBox( const T& lowerX = 0, const T& upperX = 0,
                 const T& lowerY = 0, const T& upperY = 0,
                 const T& lowerZ = 0, const T& upperZ = 0,
                 const T& lowerT = 0, const T& upperT = 0 );
    BoundingBox( const BoundingBox< T >& other );
    virtual ~BoundingBox();

    BoundingBox< T >& operator=( const BoundingBox< T >& other );

    void setLowerX( const T& lowerX );
    const T& getLowerX() const;

    void setUpperX( const T& upperX );
    const T& getUpperX() const;

    void setLowerY( const T& lowerY );
    const T& getLowerY() const;

    void setUpperY( const T& upperY );
    const T& getUpperY() const;

    void setLowerZ( const T& lowerZ );
    const T& getLowerZ() const;

    void setUpperZ( const T& upperZ );
    const T& getUpperZ() const;

    void setLowerT( const T& lowerT );
    const T& getLowerT() const;

    void setUpperT( const T& upperT );
    const T& getUpperT() const;

    bool contains( const T& x = 0, const T& y = 0,
                   const T& z = 0, const T& t = 0 ) const;
    bool contains( const Eigen::Matrix< T, 3, 1 >& site,
                   const T& t = 0 ) const;
    bool contains( const BoundingBox< T >& other ) const;

    bool onBoundary( const T& x = 0, const T& y = 0,
                     const T& z = 0, const T& t = 0 ) const;
    bool onBoundary( const Eigen::Matrix< T, 3, 1 >& site,
                     const T& t = 0 ) const;

  private:

    T _lowerX, _upperX;
    T _lowerY, _upperY;
    T _lowerZ, _upperZ;
    T _lowerT, _upperT;

};


}

}

}

}

#endif

