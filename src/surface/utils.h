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

#ifndef __surface_utils_h__
#define __surface_utils_h__

#include "surface/mesh.h"
#include "surface/polygon.h"
#include "surface/types.h"


namespace MR
{
  namespace Surface
  {



    inline Vertex normal (const Vertex& one, const Vertex& two, const Vertex& three)
    {
      return (two - one).cross (three - two).normalized();
    }
    inline Vertex normal (const Mesh& mesh, const Triangle& tri)
    {
      return normal (mesh.vert(tri[0]), mesh.vert(tri[1]), mesh.vert(tri[2]));
    }

    inline Vertex normal (const Vertex& one, const Vertex& two, const Vertex& three, const Vertex& four)
    {
      return (normal (one, two, three) + normal (one, three, four)).normalized();
    }

    inline Vertex normal (const Mesh& mesh, const Quad& quad)
    {
      return normal (mesh.vert(quad[0]), mesh.vert(quad[1]), mesh.vert(quad[2]), mesh.vert(quad[3]));
    }



    inline default_type area (const Vertex& one, const Vertex& two, const Vertex& three)
    {
      return 0.5 * ((two - one).cross (three - two).norm());
    }

    inline default_type area (const Mesh& mesh, const Triangle& tri)
    {
      return area (mesh.vert(tri[0]), mesh.vert(tri[1]), mesh.vert(tri[2]));
    }

    inline default_type area (const Vertex& one, const Vertex& two, const Vertex& three, const Vertex& four)
    {
      return area (one, two, three) + area (one, three, four);
    }

    inline default_type area (const Mesh& mesh, const Quad& quad)
    {
      return area (mesh.vert(quad[0]), mesh.vert(quad[1]), mesh.vert(quad[2]), mesh.vert(quad[3]));
    }



  }
}

#endif

