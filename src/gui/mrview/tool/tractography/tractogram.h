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

#include "gui/mrview/displayable.h"
#include "dwi/tractography/properties.h"
#include "gui/mrview/tool/tractography/tractography.h"
#include "gui/mrview/colourmap.h"


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

        enum TrackColourType { Direction, Ends, Manual, ScalarFile };

        class Tractogram : public Displayable
        {
          Q_OBJECT

          public:
            Tractogram (Window& parent, Tractography& tool, const std::string& filename);

            ~Tractogram ();

            void render (const Projection& transform);

            void renderColourBar (const Projection& transform) {
              if (color_type == ScalarFile && show_colour_bar)
                colourbar_renderer.render (transform, *this, colourbar_position_index, this->scale_inverted());
            }

            void load_tracks();

            void load_end_colours();
            void load_track_scalars (const std::string&);
            void erase_nontrack_data();

            void set_colour (float c[3])
            {
              colour[0] = c[0];
              colour[1] = c[1];
              colour[2] = c[2];
            }

            bool scalarfile_by_direction;
            bool show_colour_bar;
            TrackColourType color_type;
            float colour[3];
            std::string scalar_filename;

            class Shader : public Displayable::Shader {
              public:
                Shader () : do_crop_to_slab (false), scalarfile_by_direction (false), use_lighting (false), color_type (Direction) { }
                virtual std::string vertex_shader_source (const Displayable& tractogram);
                virtual std::string fragment_shader_source (const Displayable& tractogram);
                virtual bool need_update (const Displayable& object) const;
                virtual void update (const Displayable& object);
              protected:
                bool do_crop_to_slab, scalarfile_by_direction, use_lighting;
                TrackColourType color_type;

            } track_shader;

          signals:
            void scalingChanged ();

          private:
            Window& window;
            Tractography& tractography_tool;
            std::string filename;
            std::vector<GLuint> vertex_buffers;
            std::vector<GLuint> vertex_array_objects;
            std::vector<GLuint> colour_buffers;
            std::vector<GLuint> scalar_buffers;
            DWI::Tractography::Properties properties;
            std::vector<std::vector<GLint> > track_starts;
            std::vector<std::vector<GLint> > track_sizes;
            std::vector<size_t> num_tracks_per_buffer;
            ColourMap::Renderer colourbar_renderer;
            int colourbar_position_index;


            void load_tracks_onto_GPU (std::vector<Point<float> >& buffer,
                                       std::vector<GLint>& starts,
                                       std::vector<GLint>& sizes,
                                       size_t& tck_count);
                                              
            void load_end_colours_onto_GPU (std::vector<Point<float> >& buffer);

            void load_scalars_onto_GPU (std::vector<float>& buffer);

        };
      }
    }
  }
}

#endif

