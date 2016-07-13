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
#include "sparse/image.h"
#include "sparse/fixel_metric.h"
#include "fixel_format/helpers.h"

#include "gui/mrview/displayable.h"
#include "gui/mrview/tool/vector.h"


namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {

        enum FixelColourType { CValue, Direction };
        enum FixelLengthType { Unity, Amplitude, LValue };

        class AbstractFixel : public Displayable {
          public:
            AbstractFixel (const std::string&, Vector&);
            ~AbstractFixel();

              class Shader : public Displayable::Shader {
                public:
                  Shader () : do_crop_to_slice (false), color_type (Direction), length_type (Amplitude) { }
                  std::string vertex_shader_source   (const Displayable&) override;
                  std::string geometry_shader_source (const Displayable&) override;
                  std::string fragment_shader_source (const Displayable&) override;
                  virtual bool need_update (const Displayable&) const override;
                  virtual void update (const Displayable&) override;
                protected:
                  bool do_crop_to_slice;
                  FixelColourType color_type;
                  FixelLengthType length_type;
              } fixel_shader;


              void render (const Projection& projection);

              void request_render_colourbar (DisplayableVisitor& visitor) override {
                if(colour_type == CValue && show_colour_bar)
                  visitor.render_fixel_colourbar(*this);
              }

              void load_image ();
              void reload_dir_and_value_buffers ();

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

              virtual void set_length_type (FixelLengthType value) {
                length_type = value;
              }

              FixelLengthType get_length_type () const {
                return length_type;
              }

              void set_colour_type (FixelColourType value) {
                colour_type = value;
              }

              FixelColourType get_colour_type () const {
                return colour_type;
              }

              bool virtual internal_buffers_dirty () const {
                return false;
              }

              void load_scaleby_vector_opts (ComboBoxWithErrorMsg& combo_box) const {
                combo_box.clear ();
                for (const auto& value_name: value_types)
                  combo_box.addItem (tr (value_name.c_str ()));
                combo_box.setCurrentIndex (0);
              }

            protected:
              struct IntPointHasher {
                size_t operator () (const std::array<int, 3>& v) const {
                  // This hashing function works best if the fixel image dimensions
                  // are bounded above by 2^10 x 2^10 x 2^10 = 1024 x 1024 x 1024
                  return (v[0] + (v[1] << 10) + (v[2] << 20));
                }
              };

              virtual void load_image_buffer() = 0;
              virtual void update_image_buffer() {}
              virtual void request_update_interp_image_buffer (const Projection&) = 0;
              void update_interp_image_buffer (const Projection&, const MR::Header&, const MR::Transform&);

              std::string filename;
              MR::Header header;
              std::vector<std::string> value_types;

              std::unique_ptr<std::vector<Eigen::Vector3f>> buffer_pos;
              std::vector<Eigen::Vector3f>* buffer_dir;
              std::vector<float>* buffer_val;

              std::vector<Eigen::Vector3f> regular_grid_buffer_pos;
              std::vector<Eigen::Vector3f> regular_grid_buffer_dir;
              std::vector<float> regular_grid_buffer_val;

              std::vector<std::vector<std::vector<GLint> > > slice_fixel_indices;
              std::vector<std::vector<std::vector<GLsizei> > > slice_fixel_sizes;
              std::vector<std::vector<GLsizei> > slice_fixel_counts;

              // Flattened buffer used when cropping to slice
              // To support off-axis rendering, we maintain dict mapping voxels to buffer_pos indices
              std::unordered_map <std::array<int, 3>, std::vector<GLint>, IntPointHasher> voxel_to_indices_map;

              FixelLengthType length_type;
            private:
              Vector& fixel_tool;
              GL::VertexBuffer vertex_buffer;
              GL::VertexBuffer direction_buffer;
              GL::VertexBuffer value_buffer;
              GL::VertexArrayObject vertex_array_object;


              GL::VertexArrayObject regular_grid_vao;
              GL::VertexBuffer regular_grid_vertex_buffer;
              GL::VertexBuffer regular_grid_dir_buffer;
              GL::VertexBuffer regular_grid_val_buffer;

              float voxel_size_length_multipler;
              float user_line_length_multiplier;
              float line_thickness;
              FixelColourType colour_type;
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

        typedef MR::Sparse::Image<MR::Sparse::FixelMetric> FixelSparseImageType;
        typedef MR::Image<float> FixelPackedImageType;
        typedef MR::Image<uint32_t> FixelIndexImageType;

        // Subclassed specialisations of template wrapper
        // This is because loading of image data is dependent on particular buffer type

        class Fixel : public FixelType<FixelSparseImageType>
        {
            public:
              Fixel (const std::string& filename, Vector& fixel_tool) :
                FixelType (filename, fixel_tool)
              {
                value_types = {"Unity", "Fixel size", "Associated value"};

                buffer_pos.reset (new std::vector<Eigen::Vector3f> ());
                buffer_dir = &buffer_dir_store;
                buffer_val = &buffer_val_store;
                fixel_data.reset (new FixelSparseImageType (header));
                load_image ();
              }
              void load_image_buffer () override;

            private:
              std::vector<Eigen::Vector3f> buffer_dir_store;
              std::vector<float> buffer_val_store;
        };


        class PackedFixel : public FixelType<FixelPackedImageType>
        {
            public:
              PackedFixel (const std::string& filename, Vector& fixel_tool) :
                FixelType (filename, fixel_tool)
              {
                value_types = {"Unity", "Fixel size"};

                buffer_pos.reset (new std::vector<Eigen::Vector3f> ());
                buffer_dir = &buffer_dir_store;
                buffer_val = &buffer_val_store;
                fixel_data.reset (new FixelPackedImageType (header.get_image<float> ()));
                load_image ();
              }
              void load_image_buffer () override;
            private:
              std::vector<Eigen::Vector3f> buffer_dir_store;
              std::vector<float> buffer_val_store;
        };


        class FixelFolder : public FixelType<FixelIndexImageType>
        {
            public:
              FixelFolder (const std::string& dirname, Vector& fixel_tool) :
                FixelType (FixelFormat::find_index_header (dirname).name (), fixel_tool)
              {
                value_types = {"Unity"};

                buffer_pos.reset (new std::vector<Eigen::Vector3f> ());
                fixel_data.reset (new FixelIndexImageType (header.get_image<uint32_t> ()));
                load_image ();
              }
              void load_image_buffer () override;
              void set_length_type (FixelLengthType value) override;
              virtual bool internal_buffers_dirty () const override { return buffer_dirty; }
            protected:
              void update_image_buffer () override;
            private:
              bool buffer_dirty;
              std::string c_buffer_dir, c_buffer_val;

              std::map<const std::string, std::vector<Eigen::Vector3f>> buffer_dir_dict;
              std::map<const std::string, std::vector<float>> buffer_val_dict;
              std::map<const std::string, std::pair<float, float>> buffer_min_max_dict;
        };

      }
    }
  }
}
#endif
