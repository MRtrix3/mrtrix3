/*
   Copyright 2009 Brain Research Institute, Melbourne, Australia

   Written by J-Donald Tournier & David Raffelt, 17/12/12.

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

#ifndef __gui_mrview_tool_tractogram_h__
#define __gui_mrview_tool_tractogram_h__

#include <QAction>
#include <string>

// necessary to avoid conflict with Qt4's foreach macro:
#ifdef foreach
# undef foreach
#endif

#include "gui/mrview/displayable.h"
#include "dwi/tractography/properties.h"
#include "dwi/tractography/file.h"
#include "gui/opengl/shader.h"
#include "gui/mrview/tool/tractography/tractography.h"
#include "gui/mrview/tool/tractography/shader.h"


namespace MR
{
  class ProgressBar;

  namespace GUI
  {
    class Projection;

    namespace MRView
    {
      class Window;

      namespace Tool
      {

        class Tractogram : public Displayable
        {
          Q_OBJECT

          public:
            Tractogram (Window& parent, Tractography& tool, const std::string& filename);

            ~Tractogram ();

            void render2D (const Projection& transform);

            void render3D ();

          signals:
            void scalingChanged ();

          private:
            Window& window;
            Tractography& tool;
            std::string filename;
            std::vector<GLuint> vertex_buffers;
            std::vector<GLuint> vertex_array_objects;
            DWI::Tractography::Reader<float> file;
            DWI::Tractography::Properties properties;
            std::vector<std::vector<GLint> > track_starts;
            std::vector<std::vector<GLint> > track_sizes;
            std::vector<size_t> num_tracks_per_buffer;
            bool use_default_line_thickness;
            float line_thickness;
            Shader shader;

            void set_color () {
            }

            inline void load_data_into_GPU_buffer (std::vector<Point<float> >& buffer,
                                                   std::vector<GLint>& starts,
                                                   std::vector<GLint>& sizes,
                                                   size_t& tck_count) {
              buffer.push_back (Point<float>());
              GLuint vertexbuffer;
              glGenBuffers (1, &vertexbuffer);
              glBindBuffer (GL_ARRAY_BUFFER, vertexbuffer);
              glBufferData (GL_ARRAY_BUFFER, buffer.size() * sizeof(Point<float>), &buffer[0][0], GL_STATIC_DRAW);

              GLuint vertex_array_object;
              glGenVertexArrays (1, &vertex_array_object);
              glBindVertexArray (vertex_array_object);
              glEnableVertexAttribArray (0);
              glVertexAttribPointer (0, 3, GL_FLOAT, GL_FALSE, 0, (void*)(3*sizeof(float)));
              glEnableVertexAttribArray (1);
              glVertexAttribPointer (1, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
              glEnableVertexAttribArray (2);
              glVertexAttribPointer (2, 3, GL_FLOAT, GL_FALSE, 0, (void*)(6*sizeof(float)));

              vertex_array_objects.push_back(vertex_array_object);
              vertex_buffers.push_back (vertexbuffer);
              track_starts.push_back (starts);
              track_sizes.push_back (sizes);
              num_tracks_per_buffer.push_back (tck_count);
              buffer.clear();
              starts.clear();
              sizes.clear();
              tck_count = 0;
            }

            void load_tracks();

        };

      }
    }
  }
}

#endif

