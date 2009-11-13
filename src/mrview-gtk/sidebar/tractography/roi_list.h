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

#ifndef __mrview_sidebar_tractography_roi_list_h__
#define __mrview_sidebar_tractography_roi_list_h__

#include <gtkmm/treeview.h>
#include <gtkmm/liststore.h>
#include <gtkmm/menu.h>

#include "mrview/sidebar/tractography/track_list_item.h"

#define NUM_SPHERE 10
#define NUM_CIRCLE 50
#define ROI_HANDLE_SIZE 3

namespace MR {
  namespace Viewer {
    namespace SideBar {
      class Tractography;
      class TrackList;

      class ROIList : public Gtk::TreeView
      {
        public:
          ROIList (const Tractography& sidebar);
          virtual ~ROIList();

          void set (std::vector<RefPtr<DWI::Tractography::ROI> >& rois);
          void draw ();

        protected:
          class Columns : public Gtk::TreeModel::ColumnRecord {
            public:
              Columns() { add (type); add (spec); add (roi); }

              Gtk::TreeModelColumn<std::string> type;
              Gtk::TreeModelColumn<std::string> spec;
              Gtk::TreeModelColumn<RefPtr<DWI::Tractography::ROI> > roi;
          };

          Columns columns;
          Glib::RefPtr<Gtk::ListStore> model;
          const Tractography& parent;

          //bool on_button_press_event(GdkEventButton *event);
          void on_selection ();

          float circle_vertices [NUM_CIRCLE*2];
          float sphere_vertices [2*NUM_SPHERE*NUM_SPHERE*3];
          void compile_circle ();
          void compile_sphere ();
          friend class Tractography;
      };

    }
  }
}

#endif


