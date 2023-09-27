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

#include "axes.h"

#include "exception.h"
#include "mrtrix.h"


namespace MR
{
  namespace Axes
  {



    std::string dir2id (const Eigen::Vector3d& axis)
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



    Eigen::Vector3d id2dir (const std::string& id)
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



    void get_shuffle_to_make_axial(const transform_type &T, std::array<size_t, 3> &perm, std::array<bool, 3> &flip)
    {
      perm = closest(T.matrix().topLeftCorner<3, 3>());
      // Figure out whether any of the rows of the transform point in the
      //   opposite direction to the MRtrix convention
      flip[perm[0]] = T(0, perm[0]) < 0.0;
      flip[perm[1]] = T(1, perm[1]) < 0.0;
      flip[perm[2]] = T(2, perm[2]) < 0.0;
    }

    std::array<size_t, 3> closest(const Eigen::Matrix3d &M)
    {
      std::array<size_t, 3> result;
      // Find which row of the transform is closest to each scanner axis
      Eigen::Matrix3d::Index index;
      M.row(0).cwiseAbs().maxCoeff(&index);
      result[0] = index;
      M.row(1).cwiseAbs().maxCoeff(&index);
      result[1] = index;
      M.row(2).cwiseAbs().maxCoeff(&index);
      result[2] = index;

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
      if (result[0] == result[1])
        result[1] = not_any_of(result[0], result[2]);
      if (result[0] == result[2])
        result[2] = not_any_of(result[0], result[1]);
      if (result[1] == result[2])
        result[2] = not_any_of(result[0], result[1]);
      assert(result[0] != result[1] && result[1] != result[2] && result[2] != result[0]);

      return result;
    }




  }
}
