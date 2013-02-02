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

#ifndef __gui_mrview_image_h__
#define __gui_mrview_image_h__

#include <QAction>

#include "gui/mrview/displayable.h"
#include "image/buffer.h"
#include "image/voxel.h"
#include "math/versor.h"
#include "image/interp/linear.h"


namespace MR
{
  class ProgressBar;

  namespace GUI
  {
    class Projection;

    namespace MRView
    {

      class Window;

      class Image : public Displayable
      {
        Q_OBJECT

        public:
          Image (const MR::Image::Header& image_header);
          Image (Window& parent, const MR::Image::Header& image_header);

          ~Image ();

          MR::Image::Header& header () {
            return buffer;
          }
          const MR::Image::Header& header () const {
            return buffer;
          }


          void update_texture2D (int plane, int slice);
          void update_texture3D ();

          void render2D (const Projection& projection, int plane, int slice);

          void render3D_pre (const Projection& transform, float depth);
          void render3D_slice (float offset);
          void render3D_post () {
            stop();
            if (use_lighting())
              glDisable (GL_LIGHTING);
          }

          void render3D (const Projection& transform, float depth) {
            render3D_pre (transform, depth);
            render3D_slice (0.0);
            render3D_post ();
          }


          void get_axes (int plane, int& x, int& y) {
            if (plane) {
              if (plane == 1) {
                x = 0;
                y = 2;
              }
              else {
                x = 0;
                y = 1;
              }
            }
            else {
              x = 1;
              y = 2;
            }
          }


          float scaling_rate () const {
            return 1e-3 * (value_max - value_min);
          }
          
          float focus_rate () const {
            return 1e-3 * (Math::pow ( 
                  interp.dim(0)*interp.vox(0) *
                  interp.dim(1)*interp.vox(1) *
                  interp.dim(2)*interp.vox(2),
                  float (1.0/3.0)));
          }

          typedef MR::Image::Buffer<cfloat> BufferType;
          typedef BufferType::voxel_type VoxelType;
          typedef MR::Image::Interp::Linear<VoxelType> InterpVoxelType;


        private:
          BufferType buffer;

        public:
          InterpVoxelType interp;

          VoxelType& voxel () { 
            return interp;
          }

        private:
          GLuint texture2D_ID[3], texture3D_ID;
          float windowing_scale_3D;
          GLenum type, format, internal_format;
          std::vector<ssize_t> position;
          GLuint vertex_buffer_ID, vertex_array_object_ID;

          Point<> pos[4], tex[4], z, im_z;


          bool volume_unchanged ();
          size_t guess_colourmap () const;

          void draw_vertices (const Point<float>* vertices);

          template <typename T> void copy_texture_3D (GLenum format);
          void copy_texture_3D_complex ();
          template <typename T> GLenum GLtype () const;
          template <typename T> float scale_factor_3D () const;

      };


    }
  }
}

#endif

