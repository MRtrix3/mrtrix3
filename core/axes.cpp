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


#include "axes.h"

#include "exception.h"
#include "mrtrix.h"


namespace MR
{
  namespace Axes
  {



    std::string dir2id (const Eigen::Vector3& axis)
    {
      if (axis[0] == -1) {
        assert (!axis[1]); assert (!axis[2]); return "i-";
      } else if (axis[0] == 1) {
        assert (!axis[1]); assert (!axis[2]); return "i";
      } else if (axis[1] == -1) {
        assert (!axis[0]); assert (!axis[2]); return "j-";
      } else if (axis[1] == 1) {
        assert (!axis[0]); assert (!axis[2]); return "j";
      } else if (axis[2] == -1) {
        assert (!axis[0]); assert (!axis[1]); return "k-";
      } else if (axis[2] == 1) {
        assert (!axis[0]); assert (!axis[1]); return "k";
      } else {
        throw Exception ("Malformed axis direction: \"" + str(axis.transpose()) + "\"");
      }
    }



    Eigen::Vector3 id2dir (const std::string& id)
    {
      if (id == "i-")
        return { -1,  0,  0 };
      else if (id == "i")
        return {  1,  0,  0 };
      else if (id == "j-")
        return {  0, -1,  0 };
      else if (id == "j")
        return {  0,  1,  0 };
      else if (id == "k-")
        return {  0,  0, -1 };
      else if (id == "k")
        return {  0,  0,  1 };
      else
        throw Exception ("Malformed image axis identifier: \"" + id + "\"");
    }



    void get_permutation_to_make_axial (const transform_type& T, size_t perm[3], bool flip[3])
    {
      // Find which row of the transform is closest to each scanner axis
      decltype(T.matrix().topLeftCorner<3,3>())::Index index;
      T.matrix().topLeftCorner<3,3>().row(0).cwiseAbs().maxCoeff (&index); perm[0] = index;
      T.matrix().topLeftCorner<3,3>().row(1).cwiseAbs().maxCoeff (&index); perm[1] = index;
      T.matrix().topLeftCorner<3,3>().row(2).cwiseAbs().maxCoeff (&index); perm[2] = index;

      // Disambiguate permutations
      auto not_any_of = [] (size_t a, size_t b) -> size_t
      {
        for (size_t i = 0; i < 3; ++i) {
          if (a == i || b == i)
            continue;
          return i;
        }
        assert (0);
        return std::numeric_limits<size_t>::max();
      };
      if (perm[0] == perm[1])
        perm[1] = not_any_of (perm[0], perm[2]);
      if (perm[0] == perm[2])
        perm[2] = not_any_of (perm[0], perm[1]);
      if (perm[1] == perm[2])
        perm[2] = not_any_of (perm[0], perm[1]);
      assert (perm[0] != perm[1] && perm[1] != perm[2] && perm[2] != perm[0]);

      // Figure out whether any of the rows of the transform point in the
      //   opposite direction to the MRtrix convention
      flip[perm[0]] = T(0,perm[0]) < 0.0;
      flip[perm[1]] = T(1,perm[1]) < 0.0;
      flip[perm[2]] = T(2,perm[2]) < 0.0;
    }




  }
}
