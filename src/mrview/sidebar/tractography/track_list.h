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

    15-12-2008 J-Donald Tournier <d.tournier@brain.org.au>
    * a few bug fixes + memory performance improvements for the depth blend option
    
*/

#ifndef __mrview_sidebar_tractography_track_list_h__
#define __mrview_sidebar_tractography_track_list_h__

#include <gtkmm/treeview.h>
#include <gtkmm/liststore.h>
#include <gtkmm/menu.h>

#include "mrview/sidebar/tractography/track_list_item.h"


namespace MR {
  namespace Viewer {
    namespace SideBar {
      class Tractography;

      class TrackList : public Gtk::TreeView
      {
        public:
          TrackList (const Tractography& sidebar);
          virtual ~TrackList();

          void load (const std::string& filename);
          void draw ();

        protected:
          class Columns : public Gtk::TreeModel::ColumnRecord {
            public:
              Columns() { add (show); add (pix); add (name); add (track); }

              Gtk::TreeModelColumn<bool> show;
              Gtk::TreeModelColumn<Glib::RefPtr<Gdk::Pixbuf> > pix;
              Gtk::TreeModelColumn<std::string> name;
              Gtk::TreeModelColumn<RefPtr<TrackListItem> >  track;
          };

          Columns columns;
          Glib::RefPtr<Gtk::ListStore> model;
          Glib::RefPtr<Gdk::Pixbuf>    colour_by_dir_pixbuf;
          Gtk::Menu popup_menu;
          const Tractography& parent;

          bool on_button_press_event(GdkEventButton *event);
          void on_open ();
          void on_close ();
          void on_clear ();
          void on_colour_by_direction ();
          void on_randomise_colour ();
          void on_set_colour ();
          void on_tick (const std::string& path);
          bool on_refresh ();

          std::vector<uint> vertices;
          Point previous_normal;
          float previous_Z;

          static const uint8_t colour_by_dir_data[16*16*4];
          static TrackListItem::Track::Point* root;
          static bool compare (uint a, uint b) { return (root[a] < root[b]); }

          friend class Tractography;
      };

    }
  }
}

#endif

