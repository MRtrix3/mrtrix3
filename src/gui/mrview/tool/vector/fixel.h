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
#ifndef __gui_mrview_tool_fixel_fixelimage_h__
#define __gui_mrview_tool_fixel_fixelimage_h__

#include <unordered_map>

#include "header.h"
#include "image.h"
#include "transform.h"

#include "algo/loop.h"
#include "formats/fixel/legacy/image.h"
#include "formats/fixel/legacy/fixel_metric.h"
#include "formats/fixel/helpers.h"

#include "gui/mrview/displayable.h"
#include "gui/mrview/tool/vector/vector.h"
#include "gui/mrview/tool/vector/vector_structs.h"


namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {
        class AbstractFixel : public Displayable {
          public:
            AbstractFixel (const std::string&, Vector&);
            ~AbstractFixel();

              class Shader : public Displayable::Shader {
                public:
                  Shader () : do_crop_to_slice (false), color_type (Direction), scale_type (Value) { }
                  std::string vertex_shader_source   (const Displayable&) override;
                  std::string geometry_shader_source (const Displayable&) override;
                  std::string fragment_shader_source (const Displayable&) override;
                  virtual bool need_update (const Displayable&) const override;
                  virtual void update (const Displayable&) override;
                protected:
                  bool do_crop_to_slice;
                  FixelColourType color_type;
                  FixelScaleType scale_type;
              } fixel_shader;


              void render (const Projection& projection);

              void request_render_colourbar (DisplayableVisitor& visitor) override {
                if(colour_type == CValue && show_colour_bar)
                  visitor.render_fixel_colourbar(*this);
              }

              void load_image (const std::string &filename);

              void reload_directions_buffer ();

              void reload_colours_buffer ();

              void reload_values_buffer ();

              void reload_threshold_buffer ();

              void set_line_length_multiplier (float value) {
                user_line_length_multiplier = value;
              }

              float get_line_length_multiplier () const {
                return user_line_length_multiplier;
              }

              void set_line_thickness (float value) {
                line_thickness = value;
              }

              float get_line_thickenss () const {
                return line_thickness;
              }

              size_t get_scale_type_index () const {
                return scale_type_index;
              }

              void set_scale_type_index (size_t index)
              {
                if (index != scale_type_index) {
                  scale_type_index = index;
                  scale_type = index == 0 ? Unity : Value;
                  value_buffer_dirty = true;
                }
              }

              size_t get_threshold_type_index () const {
                return threshold_type_index;
              }

              void set_threshold_type_index (size_t index)
              {
                if (index != threshold_type_index) {
                  threshold_type_index = index;
                  if (colour_type == CValue) {
                    lessthan = get_threshold_lower ();
                    greaterthan = get_threshold_upper ();
                  }

                  threshold_buffer_dirty = true;
                }
              }

              size_t get_colour_type_index () const {
                return colour_type_index;
              }

              void set_colour_type_index (size_t index) {
                auto& fixel_val = current_fixel_colour_state ();
                fixel_val.current_min = std::isfinite (scaling_min ()) ? scaling_min () : fixel_val.current_min;
                fixel_val.current_max = std::isfinite (scaling_max ()) ? scaling_max () : fixel_val.current_max;

                if (index != colour_type_index) {
                  colour_type_index = index;
                  colour_type = index == 0 ? Direction : CValue;
                  colour_buffer_dirty = true;
                }

                auto& new_fixel_val = current_fixel_colour_state ();
                value_min = new_fixel_val.value_min;
                value_max = new_fixel_val.value_max;
                if (colour_type == CValue) {
                  lessthan = get_threshold_lower ();
                  greaterthan = get_threshold_upper ();
                }
                set_windowing (new_fixel_val.current_min, new_fixel_val.current_max);
              }

              FixelColourType get_colour_type () const {
                return colour_type;
              }

              float get_threshold_lower () const {
                FixelValue& fixel_thresh = current_fixel_threshold_state ();
                FixelValue& fixel_val = current_fixel_colour_state ();
                return fixel_thresh.get_relative_threshold_lower (fixel_val);
              }

              float get_unscaled_threshold_lower () const {
                return current_fixel_threshold_state ().lessthan;
              }

              void set_threshold_lower (float value) {
                FixelValue& fixel_threshold = current_fixel_threshold_state ();
                fixel_threshold.lessthan = value;
                if (colour_type == CValue)
                  lessthan = get_threshold_lower ();
              }

              float get_threshold_upper () const {
                FixelValue& fixel_thresh = current_fixel_threshold_state ();
                FixelValue& fixel_val = current_fixel_colour_state ();
                return fixel_thresh.get_relative_threshold_upper (fixel_val);
              }

              float get_unscaled_threshold_upper () const {
                return current_fixel_threshold_state ().greaterthan;
              }

              void set_threshold_upper (float value) {
                FixelValue& fixel_threshold = current_fixel_threshold_state ();
                fixel_threshold.greaterthan = value;
                if (colour_type == CValue)
                  greaterthan = get_threshold_upper ();
              }

              float get_unscaled_threshold_rate () const {
                FixelValue& fixel_threshold = current_fixel_threshold_state ();
                return 1e-3 * (fixel_threshold.value_max - fixel_threshold.value_min);
              }

              void load_colourby_combobox_options (ComboBoxWithErrorMsg& combo_box) const {
                combo_box.clear ();
                for (size_t i = 0, N = colour_types.size (); i < N; ++i)
                  combo_box.addItem (tr (colour_types[i].c_str ()));
                combo_box.setCurrentIndex (colour_type_index);
              }

              void load_scaleby_combobox_options (ComboBoxWithErrorMsg& combo_box) const {
                combo_box.clear ();
                for (const auto& value_name: value_types)
                  combo_box.addItem (tr (value_name.c_str ()));
                combo_box.setCurrentIndex (scale_type_index);
              }

              void load_threshold_combobox_options (ComboBoxWithErrorMsg& combo_box) const {
                combo_box.clear ();
                for (size_t i = 1, N = value_types.size (); i < N; ++i)
                  combo_box.addItem (tr (value_types[i].c_str ()));
                combo_box.setCurrentIndex (threshold_type_index);
              }

              bool has_values () const {
                return fixel_values.size();
              }

            protected:
              struct IntPointHasher {
                size_t operator () (const std::array<int, 3>& v) const {
                  // This hashing function works best if the fixel image dimensions
                  // are bounded above by 2^10 x 2^10 x 2^10 = 1024 x 1024 x 1024
                  return (v[0] + (v[1] << 10) + (v[2] << 20));
                }
              };

              void update_image_buffers ();
              virtual void load_image_buffer() = 0;
              virtual void request_update_interp_image_buffer (const Projection&) = 0;
              void update_interp_image_buffer (const Projection&, const MR::Header&, const MR::Transform&);

              inline FixelValue& current_fixel_value_state () const {
                return get_fixel_value (value_types[scale_type_index]);
              }

              inline FixelValue& current_fixel_threshold_state () const {
                return get_fixel_value (threshold_types[threshold_type_index]);
              }

              inline FixelValue& current_fixel_colour_state () const {
                return get_fixel_value (colour_types[colour_type_index]);
              }

              virtual FixelValue& get_fixel_value (const std::string& key) const {
                return fixel_values[key];
              }

              MR::Header header;
              std::vector<std::string> colour_types;
              std::vector<std::string> value_types;
              std::vector<std::string> threshold_types;
              mutable std::map<const std::string, FixelValue> fixel_values;
              mutable FixelValue dummy_fixel_val_state;

              std::vector<Eigen::Vector3f> pos_buffer_store;
              std::vector<Eigen::Vector3f> dir_buffer_store;

              std::vector<Eigen::Vector3f> regular_grid_buffer_pos;
              std::vector<Eigen::Vector3f> regular_grid_buffer_dir;
              std::vector<float> regular_grid_buffer_colour;
              std::vector<float> regular_grid_buffer_val;
              std::vector<float> regular_grid_buffer_threshold;

              std::vector<std::vector<std::vector<GLint> > > slice_fixel_indices;
              std::vector<std::vector<std::vector<GLsizei> > > slice_fixel_sizes;
              std::vector<std::vector<GLsizei> > slice_fixel_counts;

              // Flattened buffer used when cropping to slice
              // To support off-axis rendering, we maintain dict mapping voxels to buffer_pos indices
              std::unordered_map <std::array<int, 3>, std::vector<GLint>, IntPointHasher> voxel_to_indices_map;

              FixelColourType colour_type;
              FixelScaleType scale_type;
              size_t colour_type_index;
              size_t scale_type_index;
              size_t threshold_type_index;

              bool colour_buffer_dirty;
              bool value_buffer_dirty;
              bool threshold_buffer_dirty;
              bool dir_buffer_dirty;
            private:
              Vector& fixel_tool;
              GL::VertexBuffer vertex_buffer;
              GL::VertexBuffer direction_buffer;
              GL::VertexBuffer colour_buffer;
              GL::VertexBuffer value_buffer;
              GL::VertexBuffer threshold_buffer;
              GL::VertexArrayObject vertex_array_object;

              GL::VertexArrayObject regular_grid_vao;
              GL::VertexBuffer regular_grid_vertex_buffer;
              GL::VertexBuffer regular_grid_dir_buffer;
              GL::VertexBuffer regular_grid_colour_buffer;
              GL::VertexBuffer regular_grid_val_buffer;
              GL::VertexBuffer regular_grid_threshold_buffer;

              float voxel_size_length_multipler;
              float user_line_length_multiplier;
              float line_thickness;
        };


        // Wrapper to generically store fixel data

        template <typename ImageType> class FixelType : public AbstractFixel
        {
          public:
            FixelType (const std::string& filename, Vector& fixel_tool) :
            AbstractFixel (filename, fixel_tool),
              transform (header) { }

          protected:
            std::unique_ptr<ImageType> fixel_data;
            MR::Transform transform;

            void request_update_interp_image_buffer (const Projection& projection) override {
              update_interp_image_buffer (projection, *fixel_data, transform);
            }
          };

          typedef MR::Sparse::Legacy::Image<MR::Sparse::Legacy::FixelMetric> FixelSparseImageType;
          typedef MR::Image<float> FixelPackedImageType;
          typedef MR::Image<uint32_t> FixelIndexImageType;
       }
    }
  }
}
#endif
