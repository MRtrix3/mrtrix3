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

#ifndef __gui_dwi_render_window_h__
#define __gui_dwi_render_window_h__

#include "gui/dwi/render_frame.h"
#include "gui/opengl/gl.h"

namespace MR
{
  namespace GUI
  {
    class LightingDock;

    namespace DWI
    {

      class Window : public QMainWindow
      {
          Q_OBJECT

          class RenderFrame : public DWI::RenderFrame
          {
            public:
              using DWI::RenderFrame::RenderFrame;
              void set_colour (const QColor& c) {
                renderer.set_colour (c);
                update();
              }
              QColor get_colour() const {
                return renderer.get_colour();
              }
          };

        public:
          Window (bool is_response_coefs);
          ~Window();
          void set_values (const std::string& filename);

        protected slots:
          void open_slot ();
          void close_slot ();
          void use_lighting_slot (bool is_checked);
          void show_axes_slot (bool is_checked);
          void hide_negative_lobes_slot (bool is_checked);
          void colour_by_direction_slot (bool is_checked);
          void reset_scale_slot ();
          void reset_view_slot ();
          void response_slot (bool is_checked);
          void previous_slot ();
          void next_slot ();
          void previous_10_slot ();
          void next_10_slot ();
          void lmax_slot ();
          void lod_slot ();
          void screenshot_slot ();
          void lmax_inc_slot ();
          void lmax_dec_slot ();
          void manual_colour_slot ();
          void advanced_lighting_slot ();

        protected:
          RenderFrame* render_frame;
          QDialog* lighting_dialog;
          QActionGroup* lod_group, *lmax_group, *screenshot_OS_group;
          QAction *colour_by_direction_action, *response_action;

          std::string name;
          int current;
          Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic> values;
          bool is_response;

          void set_values (int row);
      };


    }
  }
}


#endif

