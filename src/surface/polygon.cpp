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


