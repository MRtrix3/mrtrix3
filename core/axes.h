/* Copyright (c) 2008-2023 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Covered Software is provided under this License on an "as is"
 * basis, without warranty of any kind, either expressed, implied, or
 * statutory, including, without limitation, warranties that the
 * Covered Software is free of defects, merchantable, fit for a
 * particular purpose or non-infringing.
 * See the Mozilla Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
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
    std::string    dir2id (const Eigen::Vector3d&);
    Eigen::Vector3d id2dir (const std::string&);



    //! determine the axis permutations and flips necessary to make an image
    //!   appear approximately axial
    void get_permutation_to_make_axial (const transform_type& T, std::array<size_t, 3>& perm, std::array<bool, 3>& flip);



  }
}

#endif

