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


#ifndef __axes_h__
#define __axes_h__


#include <string>

#include "types.h"



namespace MR
{
  namespace Axes
  {



    //! convert axis directions between formats
    /*! these helper functions convert the definition of
       *  phase-encoding direction between a 3-vector (e.g.
       *  [0 1 0] ) and a NIfTI axis identifier (e.g. 'i-')
       */
    std::string    dir2id (const Eigen::Vector3&);
    Eigen::Vector3 id2dir (const std::string&);




  }
}

#endif

