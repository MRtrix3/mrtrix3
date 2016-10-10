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


#ifndef __dwi_tractography_mact_keycomp_h__
#define __dwi_tractography_mact_keycomp_h__


#include "types.h"


namespace MR
{

namespace DWI
{

namespace Tractography
{

namespace MACT
{


struct Vector3iCompare : public std::binary_function< Eigen::Vector3i,
                                                      Eigen::Vector3i,
                                                      bool >
{
  bool operator()( const Eigen::Vector3i& v1, const Eigen::Vector3i& v2 ) const
  {
    return ( v1[ 2 ] < v2[ 2 ] ) ||
           ( ( v1[ 2 ] == v2[ 2 ]  ) && ( ( v1[ 1 ] < v2[ 1 ] ) ||
                                    ( ( v1[ 1 ] == v2[ 1 ] ) && ( v1[ 0 ] < v2[ 0 ] ) ) ) );
  }
};


}

}

}

}

#endif

