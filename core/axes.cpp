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




  }
}
