/* Copyright (c) 2008-2017 the MRtrix3 contributors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
 */


#ifndef __gui_mrview_volume_h__
#define __gui_mrview_volume_h__

#include "header.h"
#include "transform.h"

#include "file/config.h"
#include "gui/opengl/gl.h"
#include "gui/mrview/displayable.h"


namespace MR
{
  namespace GUI
  {
    class Projection;

    namespace MRView
    {

      class Window;

      class Volume : public Displayable
      { MEMALIGN(Volume)
        public:
          Volume (MR::Header&& header) :
              Displayable (header.name()),
              _header (std::move (header)),
              _transform (_header),
              //CONF option: ImageInterpolation
              //CONF default: true
              //CONF Define default interplation setting for image and image overlay.
              interpolation (File::Config::get_bool("ImageInterpolation", true) ? gl::LINEAR : gl::NEAREST),
              texture_mode_changed (true) { }

          virtual ~Volume();

          void set_interpolate (bool linear) { interpolation = linear ? gl::LINEAR : gl::NEAREST; }
          bool interpolate () const { return interpolation == gl::LINEAR; }

          void set_colourmap (size_t index) {
            if (ColourMap::maps[index].special || ColourMap::maps[colourmap].special) 
              if (index != colourmap) 
                texture_mode_changed = true;
            Displayable::colourmap = index;
          }

          void render (Displayable::Shader& shader_program, const Projection& projection, float depth) {
            start (shader_program, _scale_factor);
            projection.set (shader_program);
            _texture.bind();
            set_vertices_for_slice_render (projection, depth);
            draw_vertices ();
            stop (shader_program);
          }

          void bind () {
            if (!_texture) { // allocate:
              _texture.gen (gl::TEXTURE_3D);
              _texture.bind();
            }
            else 
              _texture.bind();
            _texture.set_interp (interpolation);
          }

          void allocate();

          float focus_rate () const {
            return 1.0e-3 * (std::pow ( 
                  _header.size(0) * _header.spacing(0) *
                  _header.size(1) * _header.spacing(1) *
                  _header.size(2) * _header.spacing(2),
                  float (1.0/3.0)));
          }

          float scale_factor () const { return _scale_factor; }
          const GL::Texture& texture () const { return _texture; }
          const MR::Header& header () const { return _header; }
          MR::Header& header () { return _header; }
          const MR::Transform& transform () const { return _transform; }

          void min_max_set() {
            update_levels();
            if (std::isnan (display_midpoint) || std::isnan (display_range))
              reset_windowing();
          }


          inline void upload_data (const std::array<ssize_t,3>& x, const std::array<ssize_t,3>& size, const void* data) {
            gl::TexSubImage3D (gl::TEXTURE_3D, 0,
                x[0], x[1], x[2],
                size[0], size[1], size[2],
                format, type, data);
          }

        protected:
          MR::Header _header;
          MR::Transform _transform;
          int interpolation;
          GL::Texture _texture;
          GL::VertexBuffer vertex_buffer;
          GL::VertexArrayObject vertex_array_object;
          GLenum type, format, internal_format;
          float _scale_factor;
          bool texture_mode_changed;

          Eigen::Vector3f pos[4], tex[4], z, im_z;
          Eigen::Vector3f vertices[8];


          inline Eigen::Vector3f div (const Eigen::Vector3f& a, const Eigen::Vector3f& b) {
            return Eigen::Vector3f (a[0]/b[0], a[1]/b[1], a[2]/b[2]);
          }

          void set_vertices_for_slice_render (const Projection& projection, float depth) 
          {
            vertices[0] = projection.screen_to_model (projection.x_position(), projection.y_position()+projection.height(), depth);
            vertices[2] = projection.screen_to_model (projection.x_position(), projection.y_position(), depth);
            vertices[4] = projection.screen_to_model (projection.x_position()+projection.width(), projection.y_position(), depth);
            vertices[6] = projection.screen_to_model (projection.x_position()+projection.width(), projection.y_position()+projection.height(), depth);

            const Eigen::Vector3f sizes (_header.size (0), _header.size (1), _header.size (2));
            vertices[1] = div ((_transform.scanner2voxel.cast<float>() * vertices[0]) + Eigen::Vector3f { 0.5, 0.5, 0.5 }, sizes);
            vertices[3] = div ((_transform.scanner2voxel.cast<float>() * vertices[2]) + Eigen::Vector3f { 0.5, 0.5, 0.5 }, sizes);
            vertices[5] = div ((_transform.scanner2voxel.cast<float>() * vertices[4]) + Eigen::Vector3f { 0.5, 0.5, 0.5 }, sizes);
            vertices[7] = div ((_transform.scanner2voxel.cast<float>() * vertices[6]) + Eigen::Vector3f { 0.5, 0.5, 0.5 }, sizes);
          }

          void draw_vertices ()
          {
            if (!vertex_buffer || !vertex_array_object) {
              assert (!vertex_buffer);
              assert (!vertex_array_object);

              vertex_buffer.gen();
              vertex_array_object.gen();

              vertex_buffer.bind (gl::ARRAY_BUFFER);
              vertex_array_object.bind();

              gl::EnableVertexAttribArray (0);
              gl::VertexAttribPointer (0, 3, gl::FLOAT, gl::FALSE_, 2*sizeof(Eigen::Vector3f), (void*)0);

              gl::EnableVertexAttribArray (1);
              gl::VertexAttribPointer (1, 3, gl::FLOAT, gl::FALSE_, 2*sizeof(Eigen::Vector3f), (void*)(sizeof(Eigen::Vector3f)));
            }
            else {
              vertex_buffer.bind (gl::ARRAY_BUFFER);
              vertex_array_object.bind();
            }

            gl::BufferData (gl::ARRAY_BUFFER, 8*sizeof(Eigen::Vector3f), &vertices[0][0], gl::STREAM_DRAW);
            gl::DrawArrays (gl::TRIANGLE_FAN, 0, 4);
          }

      };


    }
  }
}

#endif

