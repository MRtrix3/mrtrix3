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

const size_t MAX_BUFFER_SIZE = 2796200;  // number of points to fill 32MB

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
          use_default_line_thickness (true),
          line_thickness (1.0)
        {
          load_tracks();
        }


        Tractogram::~Tractogram ()
        {
          glDeleteBuffers (vertex_buffers.size(), &vertex_buffers[0]);
          glDeleteVertexArrays (vertex_array_objects.size(), &vertex_array_objects[0]);
        }


        void Tractogram::render2D (const Projection& transform)
        {

          if (tool.do_crop_to_slab() && tool.get_slab_thickness() <= 0.0)
            return;

          if (tool.do_shader_update() || !shader)
            shader.set_crop_to_slab (tool.do_crop_to_slab());  // recompile

          shader.start (transform);

          if (tool.do_crop_to_slab()) {
            glUniform3f (shader.get_uniform ("screen_normal"), 
                transform.screen_normal()[0], transform.screen_normal()[1], transform.screen_normal()[2]);
            glUniform1f (shader.get_uniform ("crop_var"),
                window.focus().dot(transform.screen_normal()) - tool.get_slab_thickness() / 2);
            glUniform1f (shader.get_uniform ("slab_width"), 
                tool.get_slab_thickness());
          }

          glShadeModel(GL_SMOOTH);
          if (tool.get_opacity() < 1.0) {
            glEnable (GL_BLEND);
            glDisable (GL_DEPTH_TEST);
            glDepthMask (GL_FALSE);
            glBlendEquation (GL_FUNC_ADD);
            glBlendFunc (GL_CONSTANT_ALPHA, GL_ONE);
            glBlendColor (1.0, 1.0, 1.0, tool.get_opacity());
          } else {
            glEnable (GL_DEPTH_TEST);
            glDepthMask (GL_TRUE);
          }

          if (use_default_line_thickness)
            glLineWidth (tool.get_line_thickness());
          else
            glLineWidth (line_thickness);

          for (size_t buf = 0; buf < vertex_buffers.size(); ++buf) {
            glBindVertexArray (vertex_array_objects[buf]);
            glMultiDrawArrays (GL_LINE_STRIP, &track_starts[buf][0], &track_sizes[buf][0], num_tracks_per_buffer[buf]);
          }

          if (tool.get_opacity() < 1.0) {
            glDisable (GL_BLEND);
            glEnable (GL_DEPTH_TEST);
            glDepthMask (GL_TRUE);
          }
          shader.stop();
        }

        void Tractogram::render3D ()
        {

        }


        void Tractogram::load_tracks()
        {
          file.open (filename, properties);
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
            if (buffer.size() >= MAX_BUFFER_SIZE)
              load_data_into_GPU_buffer (buffer, starts, sizes, tck_count);
          }
          if (buffer.size())
            load_data_into_GPU_buffer (buffer, starts, sizes, tck_count);
          file.close();
        }

      }
    }
  }
}


