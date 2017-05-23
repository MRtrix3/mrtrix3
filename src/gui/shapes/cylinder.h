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


#ifndef __gui_shapes_cylinder_h__
#define __gui_shapes_cylinder_h__

#include "gui/opengl/gl.h"
#include "gui/opengl/gl_core_3_3.h"

namespace MR
{
  namespace GUI
  {
    namespace Shapes
    {


    class Cylinder
    { MEMALIGN(Cylinder)
      public:
        Cylinder () : num_indices (0) { }

        void LOD (const size_t);

        size_t num_indices;
        GL::VertexBuffer vertex_buffer, normal_buffer;
        GL::IndexBuffer index_buffer;

    };


    }
  }
}

#endif

