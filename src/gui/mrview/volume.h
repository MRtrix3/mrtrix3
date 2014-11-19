/*
   Copyright 2009 Brain Research Institute, Melbourne, Australia

   Written by J-Donald Tournier, 13/11/09.

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

#ifndef __gui_mrview_volume_h__
#define __gui_mrview_volume_h__

#include "gui/opengl/gl.h"
#include "gui/mrview/displayable.h"
#include "math/versor.h"
#include "image/info.h"
#include "image/transform.h"


namespace MR
{
  namespace GUI
  {
    class Projection;

    namespace MRView
    {

      class Window;

      class Volume : public Displayable
      {
        public:
          template <class InfoType>
            Volume (const InfoType& info) :
              Displayable (info.name()),
              _info (info),
              _transform (_info),
              interpolation (gl::LINEAR),
              texture_mode_changed (true) { }
          template <class InfoType>
            Volume (Window& parent, const InfoType& info) :
              Displayable (parent, info.name()),
              _info (info),
              _transform (_info),
              interpolation (gl::LINEAR),
              texture_mode_changed (true) { }

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
                  _info.dim(0) * _info.vox(0) *
                  _info.dim(1) * _info.vox(1) *
                  _info.dim(2) * _info.vox(2),
                  float (1.0/3.0)));
          }

          float scale_factor () const { return _scale_factor; }
          const GL::Texture& texture () const { return _texture; }
          const MR::Image::ConstInfo& info () const { return _info; }
          const MR::Image::Transform& transform () const { return _transform; }

          void set_min_max (float min, float max) {
            value_min = min;
            value_max = max;
            update_levels();
            if (std::isnan (display_midpoint) || std::isnan (display_range))
              reset_windowing();
          }


            inline void upload_data (const std::vector<ssize_t>& x, const std::vector<ssize_t>& size, const void* data) {
              gl::TexSubImage3D (gl::TEXTURE_3D, 0,
                  x[0], x[1], x[2],
                  size[0], size[1], size[2],
                  format, type, data);
            }

        protected:
          MR::Image::ConstInfo _info;
          MR::Image::Transform _transform;
          int interpolation;
          GL::Texture _texture;
          GL::VertexBuffer vertex_buffer;
          GL::VertexArrayObject vertex_array_object;
          GLenum type, format, internal_format;
          float _scale_factor;
          bool texture_mode_changed;

          Point<> pos[4], tex[4], z, im_z;
          Point<> vertices[8];


          inline Point<> div (const Point<>& a, const Point<>& b) {
            return Point<> (a[0]/b[0], a[1]/b[1], a[2]/b[2]);
          }

          void set_vertices_for_slice_render (const Projection& projection, float depth) 
          {
            vertices[0] = projection.screen_to_model (projection.x_position(), projection.y_position()+projection.height(), depth);
            vertices[2] = projection.screen_to_model (projection.x_position(), projection.y_position(), depth);
            vertices[4] = projection.screen_to_model (projection.x_position()+projection.width(), projection.y_position(), depth);
            vertices[6] = projection.screen_to_model (projection.x_position()+projection.width(), projection.y_position()+projection.height(), depth);

            Point<> dim (_info.dim(0), _info.dim(1), _info.dim(2));
            vertices[1] = div (_transform.scanner2voxel (vertices[0]) + Point<> (0.5, 0.5, 0.5), dim);
            vertices[3] = div (_transform.scanner2voxel (vertices[2]) + Point<> (0.5, 0.5, 0.5), dim);
            vertices[5] = div (_transform.scanner2voxel (vertices[4]) + Point<> (0.5, 0.5, 0.5), dim);
            vertices[7] = div (_transform.scanner2voxel (vertices[6]) + Point<> (0.5, 0.5, 0.5), dim);
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
              gl::VertexAttribPointer (0, 3, gl::FLOAT, gl::FALSE_, 2*sizeof(Point<float>), (void*)0);

              gl::EnableVertexAttribArray (1);
              gl::VertexAttribPointer (1, 3, gl::FLOAT, gl::FALSE_, 2*sizeof(Point<float>), (void*)(sizeof(Point<float>)));
            }
            else {
              vertex_buffer.bind (gl::ARRAY_BUFFER);
              vertex_array_object.bind();
            }

            gl::BufferData (gl::ARRAY_BUFFER, 8*sizeof(Point<float>), &vertices[0][0], gl::STREAM_DRAW);
            gl::DrawArrays (gl::TRIANGLE_FAN, 0, 4);
          }

      };


    }
  }
}

#endif

