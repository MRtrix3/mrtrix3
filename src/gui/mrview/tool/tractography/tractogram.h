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

// necessary to avoid conflict with Qt4's foreach macro:
#ifdef foreach
# undef foreach
#endif

#include "gui/mrview/displayable.h"
#include "dwi/tractography/properties.h"
#include "gui/mrview/tool/tractography/tractography.h"


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

        namespace Tractography
        {

          enum colour_type { Direction, Colour, ScalarFile };

          class Tractogram : public Displayable
          {
            Q_OBJECT

            public:
              Tractogram (Window& parent, Tractography& tool, const std::string& filename);

              ~Tractogram ();

              void render2D (const Projection& transform);

              void render3D ();

              std::string get_scalar_filename () const {
                return scalar_filename;
              }
              std::string get_filename () const {
                return filename;
              }

              // Shader
              void set_crop_to_slab (bool crop_to_slab) {
                do_crop_to_slab = crop_to_slab;
              }

              void set_colour (float color[3]) {
                colour[0] = color[0];
                colour[1] = color[1];
                colour[2] = color[2];
              }

              void set_colour_type (colour_type type) {
                color_type = type;
              }

              colour_type get_colour_type () {
                return color_type;
              }

              bool load_track_scalars (std::string filename);
              bool load_tracks();

              virtual void recompile ();

            signals:
              void scalingChanged ();

            private:
              Window& window;
              Tractography& tool;
              std::string filename;
              std::vector<GLuint> vertex_buffers;
              std::vector<GLuint> vertex_array_objects;
              std::vector<GLuint> scalar_buffers;
              DWI::Tractography::Properties properties;
              std::vector<std::vector<GLint> > track_starts;
              std::vector<std::vector<GLint> > track_sizes;
              std::vector<size_t> num_tracks_per_buffer;
              bool use_default_line_thickness;
              float line_thickness;
              std::string scalar_filename;
              GLint scalar_file_buffer;
              GLint scalar_file_array_object;
              float tck_scalar_max;
              float tck_scalar_min;

              // Shader
              bool do_crop_to_slab;
              colour_type color_type;
              float colour[3];


              inline void load_tracks_onto_GPU (std::vector<Point<float> >& buffer,
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

                vertex_array_objects.push_back (vertex_array_object);
                vertex_buffers.push_back (vertexbuffer);
                track_starts.push_back (starts);
                track_sizes.push_back (sizes);
                num_tracks_per_buffer.push_back (tck_count);
                buffer.clear();
                starts.clear();
                sizes.clear();
                tck_count = 0;
              }

              inline void load_scalars_onto_GPU (std::vector<float>& buffer) {
                buffer.push_back (NAN);
                GLuint vertexbuffer;
                glGenBuffers (1, &vertexbuffer);
                glBindBuffer (GL_ARRAY_BUFFER, vertexbuffer);
                glBufferData (GL_ARRAY_BUFFER, buffer.size() * sizeof(float), &buffer[0], GL_STATIC_DRAW);

                glBindVertexArray (vertex_array_objects[scalar_buffers.size()]);
                glEnableVertexAttribArray (3);
                glVertexAttribPointer (3, 1, GL_FLOAT, GL_FALSE, 0, (void*)(sizeof(float)));
                scalar_buffers.push_back (vertexbuffer);
                buffer.clear();
              }

          };
        }
      }
    }
  }
}

#endif

