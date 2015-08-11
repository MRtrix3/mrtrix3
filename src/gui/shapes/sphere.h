/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier and Robert E. Smith, 2015.

    This file is part of MRtrix.

    MRtrix is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    MRtrix is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with MRtrix.  If not, see <http://www.gnu.org/licenses/>.


    24-07-2008 J-Donald Tournier <d.tournier@brain.org.au>
    * added support of overlay of orientation plot on main window

*/

#ifndef __gui_shapes_sphere_h__
#define __gui_shapes_sphere_h__

#include <vector>

#include "math/vector.h"

#include "gui/opengl/gl.h"
#include "gui/opengl/gl_core_3_3.h"

namespace MR
{
  namespace GUI
  {
    namespace Shapes
    {


    class Sphere
    {
      public:
        // TODO Initialise sphere & buffers at construction;
        //   currently it doesn't seem to work as a GL context has not yet been
        //   created, so gl::GenBuffers() returns zero
        Sphere () : num_indices (0) { }

        void LOD (const size_t);

        class Vertex;
        std::vector<Vertex> vertices;
        size_t num_indices;
        GL::VertexBuffer vertex_buffer;
        GL::IndexBuffer index_buffer;


        class Vertex {
        public:
          Vertex () { }
          Vertex (const float x[3]) { p[0] = x[0]; p[1] = x[1]; p[2] = x[2]; }
          Vertex (const std::vector<Vertex>& vertices, size_t i1, size_t i2) {
            p[0] = vertices[i1][0] + vertices[i2][0];
            p[1] = vertices[i1][1] + vertices[i2][1];
            p[2] = vertices[i1][2] + vertices[i2][2];
            Math::normalise (p);
          }

          float& operator[] (const int n) { return p[n]; }
          float operator[] (const int n) const { return p[n]; }

        private:
          float p[3];

      };

    };


    }
  }
}

#endif

