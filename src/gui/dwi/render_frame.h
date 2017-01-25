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

          typedef Renderer::mode_t mode_t;

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

          void set_mode (mode_t new_mode) {
            mode = new_mode;
            if (mode != mode_t::DIXEL && dirs)
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
            assert (mode == mode_t::SH);
            if (lmax != lmax_computed) 
              recompute_mesh = recompute_amplitudes = true;
            lmax_computed = lmax;
            update();
          }
          void set_LOD (int lod) {
            assert (mode == mode_t::SH || mode == mode_t::TENSOR);
            if (lod != lod_computed) 
              recompute_mesh = recompute_amplitudes = true;
            lod_computed = lod;
            update();
          }
          void set_dixels (const MR::DWI::Directions::Set& directions) {
            assert (mode == mode_t::DIXEL);
            if (dirs)
              delete dirs.release();
            dirs.reset (new MR::DWI::Directions::Set (directions));
            recompute_mesh = recompute_amplitudes = true;
            update();
          }
          void clear_dixels() {
            assert (mode == mode_t::DIXEL);
            if (dirs)
              delete dirs.release();
            recompute_mesh = recompute_amplitudes = true;
            update();
          }

          int  get_LOD () const { return lod_computed; }
          int  get_lmax () const { return lmax_computed; }
          float get_scale () const { return scale; }
          mode_t get_mode() const { return mode; }
          bool get_show_axes () const { return show_axes; }
          bool get_hide_neg_lobes () const { return hide_neg_values; }
          bool get_color_by_dir () const { return color_by_dir; }
          bool get_use_lighting () const { return use_lighting; }
          bool get_normalise () const { return normalise; }

          void screenshot (int oversampling, const std::string& image_name);

        protected:
          float view_angle, distance, line_width, scale;
          int lmax_computed, lod_computed;
          mode_t mode;
          bool recompute_mesh, recompute_amplitudes, show_axes, hide_neg_values, color_by_dir, use_lighting, normalise;
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


