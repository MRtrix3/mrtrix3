/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 * 
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * For more details, see www.mrtrix.org
 * 
 */

#ifndef __gui_mrview_tool_tractogram_h__
#define __gui_mrview_tool_tractogram_h__

#include "gui/mrview/displayable.h"
#include "dwi/tractography/properties.h"
#include "gui/mrview/tool/tractography/tractography.h"
#include "gui/mrview/colourmap.h"


namespace MR
{

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
            Tractogram (Tractography& tool, const std::string& filename);

            ~Tractogram ();

            Window& window () const { return *Window::main; }

            void render (const Projection& transform);

            void request_render_colourbar(DisplayableVisitor& visitor) override {
              if (color_type == ScalarFile && show_colour_bar)
                visitor.render_tractogram_colourbar(*this);
            }

            void load_tracks();

            void load_end_colours();
            void load_track_scalars (const std::string&);
            void erase_nontrack_data();

            void set_colour (float c[3]) { colour = { c[0], c[1], c[2] }; }

            bool scalarfile_by_direction;
            bool show_colour_bar;
            bool should_update_stride;
            TrackColourType color_type;
            Eigen::Array3f colour;
            float original_fov;
            std::string scalar_filename;

            class Shader : public Displayable::Shader {
              public:
                Shader () : do_crop_to_slab (false), scalarfile_by_direction (false), use_lighting (false), color_type (Direction) { }
                std::string vertex_shader_source (const Displayable& displayable) override;
                std::string fragment_shader_source (const Displayable& displayable) override;
                std::string geometry_shader_source (const Displayable&) override;
                virtual bool need_update (const Displayable& object) const override;
                virtual void update (const Displayable& object) override;
              protected:
                bool do_crop_to_slab, scalarfile_by_direction, use_lighting;
                TrackColourType color_type;

            } track_shader;

          signals:
            void scalingChanged ();

          private:
            static const int max_sample_stride = 6;
            Tractography& tractography_tool;
            std::string filename;
            std::vector<GLuint> vertex_buffers;
            std::vector<GLuint> vertex_array_objects;
            std::vector<GLuint> colour_buffers;
            std::vector<GLuint> scalar_buffers;
            DWI::Tractography::Properties properties;
            std::vector<std::vector<GLint> > track_starts;
            std::vector<std::vector<GLint> > track_sizes;
            std::vector<std::vector<GLint> > original_track_sizes;
            std::vector<std::vector<GLint> > original_track_starts;
            std::vector<size_t> num_tracks_per_buffer;
            GLint sample_stride;
            float line_thickness_screenspace;
            bool vao_dirty;


            void load_tracks_onto_GPU (std::vector<Eigen::Vector3f>& buffer,
                                       std::vector<GLint>& starts,
                                       std::vector<GLint>& sizes,
                                       size_t& tck_count);
                                              
            void load_end_colours_onto_GPU (std::vector<Eigen::Vector3f>&);

            void load_scalars_onto_GPU (std::vector<float>& buffer);

            void render_streamlines ();

            void update_stride ();

          private slots:
            void on_FOV_changed() {
              should_update_stride = true;
            }
        };
      }
    }
  }
}

#endif

