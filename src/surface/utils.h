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

