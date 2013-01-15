/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier & David Raffelt 17/12/12

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

*/

#include <QMenu>

#include "progressbar.h"
#include "image/stride.h"
#include "gui/mrview/tool/tractography/tractogram.h"
#include "gui/mrview/window.h"
#include "gui/projection.h"
#include <stdio.h>

const size_t MAX_BUFFER_SIZE = 349525;  // number of points to fill 4MB

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {

        Tractogram::Tractogram (Window& window, Tractography& tool, const std::string& filename) :
          Displayable (filename),
          window (window),
          tool (tool),
          filename (filename),
          VertexArrayID (0),
          use_default_line_thickness (true),
          line_thickness (1.0)
        {
          file.open (filename, properties);
          load_tracks();
        }


        Tractogram::~Tractogram ()
        {
          file.close();
        }


        void Tractogram::render2D (const Projection& transform)
        {

          if (tool.do_shader_update() || !shader)
            shader.set_crop_to_slab (tool.do_crop_to_slab());  // recompile

          if (!VertexArrayID)
            glGenVertexArrays (1, &VertexArrayID);

          glBindVertexArray (VertexArrayID);

          glEnable (GL_DEPTH_TEST);
          glDepthMask (GL_TRUE);

          shader.start();
          GLuint MatrixID = shader.get_uniform ("MVP");

          const Math::Matrix<float>& M (transform.get_MVP());
          GLfloat MVP[] = {
              M(0,0), M(1,0), M(2,0), M(3,0),
              M(0,1), M(1,1), M(2,1), M(3,1),
              M(0,2), M(1,2), M(2,2), M(3,2),
              M(0,3), M(1,3), M(2,3), M(3,3)
          };
          glUniformMatrix4fv (MatrixID, 1, GL_FALSE, MVP);

          if (tool.do_crop_to_slab()) {
            if (tool.get_slab_thickness() < 0.000001) {
              shader.stop();
              return;
            }
            GLuint screen_normalID = shader.get_uniform ("screen_normal");
            GLuint crop_varID = shader.get_uniform ("crop_var");
            GLuint slab_widthID = shader.get_uniform ("slab_width");
            glUniform3f (screen_normalID, transform.screen_normal()[0], transform.screen_normal()[1], transform.screen_normal()[2]);
            glUniform1f (crop_varID, window.focus().dot(transform.screen_normal()) - tool.get_slab_thickness() / 2);
            glUniform1f (slab_widthID, tool.get_slab_thickness());
          }

          for (size_t buf = 0; buf < vertex_buffers.size(); ++buf) {
            glEnableVertexAttribArray (0);
            glBindBuffer (GL_ARRAY_BUFFER, vertex_buffers[buf]);
            glVertexAttribPointer (0, 3, GL_FLOAT, GL_FALSE, 0, (void*)(3*sizeof(float)));

            glEnableVertexAttribArray (1);
            glVertexAttribPointer (1, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

            glEnableVertexAttribArray (2);
            glVertexAttribPointer (2, 3, GL_FLOAT, GL_FALSE, 0, (void*)(6*sizeof(float)));

            if (use_default_line_thickness)
              glLineWidth (tool.get_line_thickness());
            else
              glLineWidth (line_thickness);

            glMultiDrawArrays (GL_LINE_STRIP, &track_starts[buf][0], &track_sizes[buf][0], num_tracks_per_buffer[buf]);
          }

          shader.stop();
          DEBUG_OPENGL;
        }

        void Tractogram::render3D ()
        {

        }


        void Tractogram::load_tracks()
        {
          std::vector<Point<float> > tck;
          std::vector<Point<float> > buffer;
          std::vector<GLint> starts;
          std::vector<GLint> sizes;
          size_t tck_count = 0;

          while (file.next (tck)) {
            starts.push_back (buffer.size());
            buffer.push_back (Point<float>());
            buffer.insert (buffer.end(), tck.begin(), tck.end());
            sizes.push_back(tck.size());
            tck_count++;

            if (buffer.size() >= MAX_BUFFER_SIZE) {
              buffer.push_back (Point<float>());
              GLuint vertexbuffer;
              glGenBuffers (1, &vertexbuffer);
              glBindBuffer (GL_ARRAY_BUFFER, vertexbuffer);
              glBufferData (GL_ARRAY_BUFFER, buffer.size() * sizeof(Point<float>), &buffer[0][0], GL_STATIC_DRAW);
              vertex_buffers.push_back (vertexbuffer);
              track_starts.push_back (starts);
              track_sizes.push_back (sizes);
              num_tracks_per_buffer.push_back (tck_count);
              tck_count = 0;
              buffer.clear();
              starts.clear();
              sizes.clear();
            }
          }
          buffer.push_back (Point<float>());
          GLuint vertexbuffer;
          glGenBuffers (1, &vertexbuffer);
          glBindBuffer (GL_ARRAY_BUFFER, vertexbuffer);
          glBufferData (GL_ARRAY_BUFFER, buffer.size() * sizeof(Point<float>), &buffer[0][0], GL_STATIC_DRAW);
          vertex_buffers.push_back (vertexbuffer);
          track_starts.push_back (starts);
          track_sizes.push_back (sizes);
          num_tracks_per_buffer.push_back (tck_count);
        }

      }
    }
  }
}


