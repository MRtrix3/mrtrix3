/* Copyright (c) 2008-2017 the MRtrix3 contributors
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


#ifndef __gui_shapes_sphere_h__
#define __gui_shapes_sphere_h__

#include <vector>

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
            Eigen::Map<Eigen::Vector3f> (p).normalize();
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

