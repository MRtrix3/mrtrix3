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

*/

#ifndef __mrview_sidebar_orientation_plot_h__
#define __mrview_sidebar_orientation_plot_h__

#include <gtkmm/paned.h>
#include <gtkmm/frame.h>
#include <gtkmm/checkbutton.h>
#include <gtkmm/spinbutton.h>
#include <gtkmm/button.h>
#include <gtkmm/label.h>
#include <gtkmm/table.h>

#include "ptr.h"
#include "image/interp.h"
#include "mrview/window.h"
#include "mrview/sidebar/base.h"
#include "dwi/render_frame.h"

namespace MR {
  namespace Viewer {
    namespace SideBar {

      class OrientationPlot : public Base
      {
        public:
          OrientationPlot ();
          ~OrientationPlot ();

          void draw ();

        protected:
          Gtk::VPaned       paned;
          Gtk::VBox         settings, source_box;
          Gtk::Frame        frame, settings_frame, source_frame;
          Gtk::Button       source_button;
          Gtk::Label        lmax_label, lod_label;
          Gtk::Table        lmax_lod_table;
          Gtk::CheckButton  align_with_viewer, interpolate, show_axes, colour_by_direction, use_lighting, hide_neg_lobes, show_overlay;
          Gtk::Adjustment   lmax_adjustment, lod_adjustment;
          Gtk::SpinButton   lmax, lod;
          DWI::RenderFrame  render;
          RefPtr<MR::Image::Object> image_object;
          RefPtr<MR::Image::Interp> interp;
          Point focus;
          float azimuth, elevation;
          GLfloat rotation[16];

          int    overlay_slice, overlay_bounds[2][2], overlay_pos[2];
          DWI::Renderer overlay_render;
          Pane* overlay_pane;

          sigc::connection       idle_connection;
          bool   on_idle ();

          void  refresh_overlay () { if (show_overlay.get_active()) Window::Main->update (this); }

          void   set_values ();
          void   get_values (std::vector<float>& values, const Point& position);
          void   set_projection ();

          void   on_align_with_viewer ();
          void   on_show_axes ();
          void   on_colour_by_direction ();
          void   on_use_lighting ();
          void   on_hide_negative_lobes ();
          void   on_show_overlay ();
          void   on_lod ();
          void   on_lmax ();

          void   on_source_browse (); 
      };

    }
  }
}

#endif


