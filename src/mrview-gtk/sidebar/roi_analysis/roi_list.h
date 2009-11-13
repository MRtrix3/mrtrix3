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

#ifndef __mrview_sidebar_roi_analysis_roi_list_h__
#define __mrview_sidebar_roi_analysis_roi_list_h__

#include <gtkmm/treeview.h>
#include <gtkmm/liststore.h>
#include <gtkmm/menu.h>
//#include <gtkmm/tooltip.h>

#include "mrview/slice.h"

namespace MR {
  namespace Viewer {
    namespace SideBar {

      class ROIAnalysis;

      class DP_ROIList : public Gtk::TreeView
      {
        public:
          DP_ROIList (const ROIAnalysis& sidebar);
          virtual ~DP_ROIList();

          void draw (int transparency);
          bool on_button_press (GdkEventButton* event);
          bool on_motion (GdkEventMotion* event) { if (editing) { process (event->x, event->y); return (true); } return (false); };
          bool on_button_release (GdkEventButton* event) { if (editing) { editing = false; return (true); } return (false); }

        protected:
          const ROIAnalysis& parent;
          bool  set, editing;
          Gtk::TreeModel::Row row;

          class ROI {
            public:
              ROI (RefPtr<MR::Image::Object> image, uint32_t C) : mask (new Image (image)), render (false), colour (C) { mask->image->set_read_only (false);}
              RefPtr<Image> mask;
              Slice::Renderer render;
              uint32_t colour;
          };

          class Columns : public Gtk::TreeModel::ColumnRecord {
            public:
              Columns() { add (show); add (pix); add (name); add (roi); }

              Gtk::TreeModelColumn<bool> show;
              Gtk::TreeModelColumn<Glib::RefPtr<Gdk::Pixbuf> > pix;
              Gtk::TreeModelColumn<std::string> name;
              Gtk::TreeModelColumn<RefPtr<ROI> > roi;
          };

          Columns columns;
          Glib::RefPtr<Gtk::ListStore> model;
          Gtk::Menu popup_menu;

          bool on_button_press_event(GdkEventButton *event);
          void on_open ();
          void on_new ();
          void on_close ();
          void on_set_colour ();
          void on_clear ();
          void on_tick (const std::string& path);

          void process (double x, double y);
          Point position (double x, double y);

          void load (RefPtr<MR::Image::Object> image);
      };

    }
  }
}

#endif

