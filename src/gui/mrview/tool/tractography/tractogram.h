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


#ifndef __gui_mrview_tool_tractogram_h__
#define __gui_mrview_tool_tractogram_h__

//#include "gui/mrview/tool/tractography/tractogram_enums.h"
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
        class Tractogram : public Displayable
        { MEMALIGN(Tractogram)
          Q_OBJECT

          public:
            Tractogram (Tractography& tool, const std::string& file_path);

            ~Tractogram ();

            Window& window () const { return *Window::main; }

            void render (const Projection& transform);

            void request_render_colourbar (DisplayableVisitor& visitor) override {
              if (color_type == TrackColourType::ScalarFile && show_colour_bar)
                visitor.render_tractogram_colourbar(*this);
            }

            void load_tracks();

            void load_end_colours();
            void load_intensity_track_scalars (const std::string&);
            void load_threshold_track_scalars (const std::string&);
            void erase_colour_data();
            void erase_intensity_scalar_data ();
            void erase_threshold_scalar_data ();

            void set_color_type (const TrackColourType);
            void set_threshold_type (const TrackThresholdType);
            void set_geometry_type (const TrackGeometryType);
            TrackColourType get_color_type() const { return color_type; }
            TrackThresholdType get_threshold_type() const { return threshold_type; }
            TrackGeometryType get_geometry_type() const { return geometry_type; }

            void set_colour (float c[3]) { colour = { c[0], c[1], c[2] }; }

            float get_threshold_rate() const {
              switch (threshold_type) {
                case TrackThresholdType::None: return NaN;
                case TrackThresholdType::UseColourFile: return scaling_rate();
                case TrackThresholdType::SeparateFile: return (1e-3 * (threshold_max - threshold_min));
              }
              assert (0);
              return NaN;
            }
            float get_threshold_min()  const { return threshold_min; }
            float get_threshold_max()  const { return threshold_max; }

            static TrackGeometryType default_tract_geom;
            static constexpr float default_line_thickness = 2e-3f;
            static constexpr float default_point_size = 4e-3f;

            bool scalarfile_by_direction;
            bool show_colour_bar;
            bool should_update_stride;
            Eigen::Array3f colour;
            float original_fov;
            float line_thickness;
            std::string intensity_scalar_filename;
            std::string threshold_scalar_filename;

            class Shader : public Displayable::Shader { MEMALIGN(Shader)
              public:
                Shader () :
                    do_crop_to_slab (false),
                    use_lighting (false),
                    color_type (TrackColourType::Direction),
                    threshold_type (TrackThresholdType::None),
                    geometry_type (Tractogram::default_tract_geom) { }
                std::string vertex_shader_source (const Displayable&) override;
                std::string fragment_shader_source (const Displayable&) override;
                std::string geometry_shader_source (const Displayable&) override;
                virtual bool need_update (const Displayable&) const override;
                virtual void update (const Displayable&) override;
              protected:
                bool do_crop_to_slab, use_lighting;
                TrackColourType color_type;
                TrackThresholdType threshold_type;
                TrackGeometryType geometry_type;

            } track_shader;

          signals:
            void scalingChanged ();

          private:
            static const int track_padding = 6;
            Tractography& tractography_tool;

            const std::string filename;

            TrackColourType color_type;
            TrackThresholdType threshold_type;
            TrackGeometryType geometry_type;

            // Instead of tracking the file path, pre-calculate the
            //   streamline tangents and store them; then, if colour by
            //   endpoint is requested, generate the buffer based on these
            //   and the known track sizes
            vector<Eigen::Vector3f> endpoint_tangents;

            vector<GLuint> vertex_buffers;
            vector<GLuint> vertex_array_objects;
            vector<GLuint> colour_buffers;
            vector<GLuint> intensity_scalar_buffers;
            vector<GLuint> threshold_scalar_buffers;
            DWI::Tractography::Properties properties;
            vector<vector<GLint> > track_starts;
            vector<vector<GLint> > track_sizes;
            vector<vector<GLint> > original_track_sizes;
            vector<vector<GLint> > original_track_starts;
            vector<size_t> num_tracks_per_buffer;
            GLint sample_stride;
            bool vao_dirty;

            // Extra members now required since different scalar files
            //   may be used for streamline colouring and thresholding
            float threshold_min, threshold_max;


            void load_tracks_onto_GPU (vector<Eigen::Vector3f>& buffer,
                                       vector<GLint>& starts,
                                       vector<GLint>& sizes,
                                       size_t& tck_count);
                            
            void load_end_colours_onto_GPU (vector<Eigen::Vector3f>&);

            void load_intensity_scalars_onto_GPU (vector<float>& buffer);
            void load_threshold_scalars_onto_GPU (vector<float>& buffer);

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

