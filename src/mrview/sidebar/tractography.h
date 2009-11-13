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

#ifndef __mrview_sidebar_tractography_h__
#define __mrview_sidebar_tractography_h__

#include <gtkmm/checkbutton.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/scale.h>
#include <gtkmm/spinbutton.h>
#include <gtkmm/frame.h>
#include <gtkmm/paned.h>

#include "mrview/sidebar/base.h"
#include "mrview/sidebar/tractography/track_list.h"
#include "mrview/sidebar/tractography/roi_list.h"

namespace MR {
  namespace Viewer {
    namespace SideBar {

      class Tractography : public Base
      {
        public:
          Tractography ();
          ~Tractography ();

          void draw();
          bool show () const { return (show_tracks.get_active()); }

        protected:
          Gtk::VBox            crop_to_slice_vbox;
          Gtk::CheckButton     show_tracks, show_ROIs, crop_to_slice, depth_blend;
          Gtk::Frame           track_frame, roi_frame, crop_to_slice_frame, transparency_frame, line_thickness_frame;
          Gtk::HScale          transparency, line_thickness;
          Gtk::Adjustment      slice_thickness_adjustment;
          Gtk::SpinButton      slab_thickness;
          Gtk::ScrolledWindow  track_scrolled_window, roi_scrolled_window;
          Gtk::VPaned          paned;
          TrackList            track_list;
          ROIList              roi_list;

          void  on_change ();
          void  on_slab ();
          void  on_transparency ();
          void  on_crop_to_slice ();
          void  on_track_selection ();

          friend class TrackList;
      };

    }
  }
}

#endif


