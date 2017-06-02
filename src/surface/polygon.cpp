/* Copyright (c) 2008-2017 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
 */


#include "surface/polygon.h"


namespace MR
{
  namespace Surface
  {



    template <>
    bool Polygon<3>::shares_edge (const Polygon<3>& that) const
    {
      uint32_t shared_vertices = 0;
      if (indices[0] == that[0] || indices[0] == that[1] || indices[0] == that[2]) ++shared_vertices;
      if (indices[1] == that[0] || indices[1] == that[1] || indices[1] == that[2]) ++shared_vertices;
      if (indices[2] == that[0] || indices[2] == that[1] || indices[2] == that[2]) ++shared_vertices;
      return (shared_vertices == 2);
    }



  }
}


