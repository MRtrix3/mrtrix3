/*
   Copyright 2014 Brain Research Institute, Melbourne, Australia

   Written by David Raffelt 2014.

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
#ifndef __gui_mrview_tool_fixel_fixelimage_h__
#define __gui_mrview_tool_fixel_fixelimage_h__

#include "gui/mrview/displayable.h"
#include "image/header.h"
#include "image/buffer_sparse.h"
#include "image/sparse/fixel_metric.h"
#include "image/sparse/voxel.h"
#include "image/transform.h"
#include "gui/mrview/tool/vector.h"
#include "image/loop.h"
#include <unordered_map>

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
            AbstractFixel (const std::string& filename, Vector& fixel_tool);

              class Shader : public Displayable::Shader {
                public:
                  Shader () : do_crop_to_slice (false), color_type (Direction), length_type (Amplitude) { }
                  std::string vertex_shader_source (const Displayable&) override;
                  std::string geometry_shader_source (const Displayable& fixel_image) override;
                  std::string fragment_shader_source (const Displayable& fixel_image) override;
                  virtual bool need_update (const Displayable& object) const override;
                  virtual void update (const Displayable& object) override;
                protected:
                  bool do_crop_to_slice;
                  FixelColourType color_type;
                  FixelLengthType length_type;
              } fixel_shader;


              void render (const Projection& projection);

              void request_render_colourbar(DisplayableVisitor& visitor) override {
                if(colour_type == CValue && show_colour_bar)
                  visitor.render_fixel_colourbar(*this);
              }

              void load_image ();

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

              void set_length_type (FixelLengthType value) {
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

            protected:
              struct IntPointHasher {
                size_t operator () (const Point<int>& p) const {
                  // This hashing function works best if the fixel image dimensions
                  // are bounded above by 2^10 x 2^10 x 2^10 = 1024 x 1024 x 1024
                  return (p[0] + (p[1] << 10) + (p[2] << 20));
                }
              };

              virtual void load_image_buffer() = 0;
              virtual void request_update_interp_image_buffer (const Projection&) = 0;
              void update_interp_image_buffer(const Projection& projection,
                                              const MR::Image::ConstHeader& fixel_header,
                                              const MR::Image::Transform& header_transform);

              std::string filename;
              MR::Image::Header header;
              std::vector<Point<float>> buffer_pos;
              std::vector<Point<float>> buffer_dir;
              std::vector<float> buffer_val;

              std::vector<Point<float>> regular_grid_buffer_pos;
              std::vector<Point<float>> regular_grid_buffer_dir;
              std::vector<float> regular_grid_buffer_val;

              std::vector<std::vector<std::vector<GLint> > > slice_fixel_indices;
              std::vector<std::vector<std::vector<GLsizei> > > slice_fixel_sizes;
              std::vector<std::vector<GLsizei> > slice_fixel_counts;

              // Flattened buffer used when cropping to slice
              // To support off-axis rendering, we maintain dict mapping voxels to buffer_pos indices
              std::unordered_map <Point<int>, std::vector<GLint>, IntPointHasher> voxel_to_indices_map;

            private:
              Vector& fixel_tool;
              GL::VertexBuffer vertex_buffer;
              GL::VertexBuffer direction_buffer;
              GL::VertexArrayObject vertex_array_object;
              GL::VertexBuffer value_buffer;

              GL::VertexArrayObject regular_grid_vao;
              GL::VertexBuffer regular_grid_vertex_buffer;
              GL::VertexBuffer regular_grid_dir_buffer;
              GL::VertexBuffer regular_grid_val_buffer;

              float voxel_size_length_multipler;
              float user_line_length_multiplier;
              float line_thickness;
              FixelLengthType length_type;
              FixelColourType colour_type;
        };


        // Wrapper to generically store fixel data
        template <typename BufferType> class FixelType : public AbstractFixel
        {
            public:
              FixelType (const std::string& filename, Vector& fixel_tool) :
                AbstractFixel(filename, fixel_tool),
                fixel_data (header),
                fixel_vox (fixel_data),
                header_transform (fixel_vox)
              {
              }

            protected:
              BufferType fixel_data;
              typename BufferType::voxel_type fixel_vox;
              MR::Image::Transform header_transform;

              void request_update_interp_image_buffer (const Projection& projection) override {
                update_interp_image_buffer(projection, fixel_data, header_transform);
              }
        };

        typedef MR::Image::BufferSparse<MR::Image::Sparse::FixelMetric> FixelSparseBufferType;
        typedef MR::Image::Buffer<float> FixelPackedBufferType;

        // Subclassed specialisations of template wrapper
        // This is because loading of image data is dependent on particular buffer type

        class Fixel : public FixelType<FixelSparseBufferType>
        {
            public:
              Fixel (const std::string& filename, Vector& fixel_tool) :
                FixelType (filename, fixel_tool) { load_image(); }
              void load_image_buffer () override;
        };

        class PackedFixel : public FixelType<FixelPackedBufferType>
        {
            public:
              PackedFixel (const std::string& filename, Vector& fixel_tool) :
                FixelType (filename, fixel_tool) { load_image(); }
              void load_image_buffer () override;
        };

      }
    }
  }
}
#endif
