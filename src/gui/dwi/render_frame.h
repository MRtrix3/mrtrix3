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

#include "memory.h"
#include "types.h"
#include "dwi/directions/set.h"
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

      class RenderFrame : public GL::Area
      {
          Q_OBJECT

        public:
          RenderFrame (QWidget* parent);
          ~RenderFrame();

          GL::Lighting* lighting;

          void set (const Eigen::VectorXf& new_values) {
            values = new_values;
            recompute_amplitudes = true;
            update();
          }

          void set_rotation (const GL::mat4& rotation);

          void set_is_SH (bool yesno = true) {
            is_SH = yesno;
            if (is_SH && dirs)
              delete dirs.release();
            recompute_mesh = recompute_amplitudes = true;
            update();
          }
          void set_show_axes (bool yesno = true) {
            show_axes = yesno;
            update();
          }
          void set_hide_neg_values (bool yesno = true) {
            hide_neg_values = yesno;
            update();
          }
          void set_color_by_dir (bool yesno = true) {
            color_by_dir = yesno;
            update();
          }
          void set_use_lighting (bool yesno = true) {
            use_lighting = yesno;
            update();
          }
          void set_normalise (bool yesno = true) {
            normalise = yesno;
            update();
          }
          void set_lmax (int lmax) {
            assert (is_SH);
            if (lmax != lmax_computed) 
              recompute_mesh = recompute_amplitudes = true;
            lmax_computed = lmax;
            update();
          }
          void set_LOD (int lod) {
            assert (is_SH);
            if (lod != lod_computed) 
              recompute_mesh = recompute_amplitudes = true;
            lod_computed = lod;
            update();
          }
          void set_dixels (const MR::DWI::Directions::Set& directions) {
            if (dirs)
              delete dirs.release();
            dirs.reset (new MR::DWI::Directions::Set (directions));
            recompute_mesh = recompute_amplitudes = true;
            update();
          }
          void clear_dixels() {
            if (dirs)
              delete dirs.release();
            recompute_mesh = recompute_amplitudes = true;
            update();
          }

          int  get_LOD () const { return lod_computed; }
          int  get_lmax () const { return lmax_computed; }
          float get_scale () const { return scale; }
          bool get_is_SH() const { return is_SH; }
          bool get_show_axes () const { return show_axes; }
          bool get_hide_neg_lobes () const { return hide_neg_values; }
          bool get_color_by_dir () const { return color_by_dir; }
          bool get_use_lighting () const { return use_lighting; }
          bool get_normalise () const { return normalise; }

          void screenshot (int oversampling, const std::string& image_name);

        protected:
          float view_angle, distance, line_width, scale;
          int lmax_computed, lod_computed;
          bool is_SH, recompute_mesh, recompute_amplitudes, show_axes, hide_neg_values, color_by_dir, use_lighting, normalise;
          std::unique_ptr<MR::DWI::Directions::Set> dirs;

          QPoint last_pos;
          GL::Font font;
          Projection projection;
          Math::Versorf orientation;
          Eigen::Vector3f focus;

          std::string screenshot_name;
          std::unique_ptr<QImage> pix;
          std::unique_ptr<GLubyte[]> framebuffer;
          int OS, OS_x, OS_y;

          GL::VertexBuffer axes_VB;
          GL::VertexArrayObject axes_VAO;
          GL::Shader::Program axes_shader;

          Renderer renderer;
          Eigen::VectorXf values;

        protected:
          virtual void initializeGL () override;
          virtual void resizeGL (int w, int h) override;
          virtual void paintGL () override;
          void mouseDoubleClickEvent (QMouseEvent* event) override;
          void mousePressEvent (QMouseEvent* event) override;
          void mouseMoveEvent (QMouseEvent* event) override;
          void wheelEvent (QWheelEvent* event) override;

          void snapshot ();
      };


    }
  }
}

#endif


