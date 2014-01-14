/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 27/06/08.

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


    24-10-2008 J-Donald Tournier <d.tournier@brain.org.au>
    * add functions to normalise plot amplitude

*/

#ifndef __gui_dwi_render_frame_h__
#define __gui_dwi_render_frame_h__

#include "ptr.h"
#include "math/versor.h"
#include "gui/opengl/lighting.h"
#include "gui/dwi/renderer.h"
#include "gui/projection.h"
#include "gui/opengl/gl.h"
#include "gui/opengl/font.h"
#include "gui/opengl/transformation.h"

#define MAX_LOD 8

namespace MR
{
  namespace GUI
  {
    namespace DWI
    {

      class RenderFrame : public GL::Widget
      {
          Q_OBJECT

        public:
          RenderFrame (QWidget* parent);

          GL::Lighting* lighting;

          void set (const Math::Vector<float>& new_values) {
            values = new_values;
            recompute_amplitudes = true;
            updateGL();
          }

          void set_rotation (const GL::mat4& rotation);

          void set_show_axes (bool yesno = true) {
            show_axes = yesno;
            updateGL();
          }
          void set_hide_neg_lobes (bool yesno = true) {
            hide_neg_lobes = yesno;
            updateGL();
          }
          void set_color_by_dir (bool yesno = true) {
            color_by_dir = yesno;
            updateGL();
          }
          void set_use_lighting (bool yesno = true) {
            use_lighting = yesno;
            updateGL();
          }
          void set_normalise (bool yesno = true) {
            normalise = yesno;
            updateGL();
          }
          void set_lmax (int lmax) {
            if (lmax != lmax_computed) 
              recompute_mesh = recompute_amplitudes = true;
            lmax_computed = lmax;
            updateGL();
          }
          void set_LOD (int lod) {
            if (lod != lod_computed) 
              recompute_mesh = recompute_amplitudes = true;
            lod_computed = lod;
            updateGL();
          }

          int  get_LOD () const { return lod_computed; }
          int  get_lmax () const { return lmax_computed; }
          float get_scale () const { return scale; }
          bool get_show_axes () const { return show_axes; }
          bool get_hide_neg_lobes () const { return hide_neg_lobes; }
          bool get_color_by_dir () const { return color_by_dir; }
          bool get_use_lighting () const { return use_lighting; }
          bool get_normalise () const { return normalise; }

          void screenshot (int oversampling, const std::string& image_name);

        protected:
          float view_angle, distance, line_width, scale;
          int lmax_computed, lod_computed;
          bool  recompute_mesh, recompute_amplitudes, show_axes, hide_neg_lobes, color_by_dir, use_lighting, normalise;

          QPoint last_pos;
          GL::Font font;
          Projection projection;
          Math::Versor<float> orientation;
          Point<> focus;

          std::string screenshot_name;
          Ptr<QImage> pix;
          GLubyte* framebuffer;
          int OS, OS_x, OS_y;

          GL::VertexBuffer axes_VB;
          GL::VertexArrayObject axes_VAO;
          GL::Shader::Program axes_shader;

          Renderer renderer;
          Math::Vector<float> values;

          virtual void initializeGL ();
          virtual void resizeGL (int w, int h);
          virtual void paintGL ();
          void mouseDoubleClickEvent (QMouseEvent* event);
          void mousePressEvent (QMouseEvent* event);
          void mouseMoveEvent (QMouseEvent* event);
          void wheelEvent (QWheelEvent* event);

          void snapshot ();
      };


    }
  }
}

#endif


