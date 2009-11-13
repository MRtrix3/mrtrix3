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

#ifndef __mrview_window_h__
#define __mrview_window_h__

#include <gtkmm/window.h>
#include <gtkmm/box.h>
#include <gtkmm/menubar.h>
#include <gtkmm/statusbar.h>
#include <gtkmm/paned.h>

#include "ptr.h"
#include "args.h"
#include "mrview/sidebar/main.h"
#include "mrview/display_area.h"
#include "mrview/slice.h"

namespace Gtk {
  namespace Menu_Helpers {
    class MenuList;
  }
}

namespace MR {

  namespace Image { class Object; }
  namespace SideBar { class Base; }

  namespace Viewer {

    class Pane;
    class Image;

    class Window : public Gtk::Window
    {
      public:
        Window (std::vector<ArgBase>& argument);
        virtual ~Window ();

        std::vector< RefPtr<Image> >   images;
        RefPtr<Image>        image;
        int                  manage (Image& ima);

        void                 update () { display_area.update(); }
        void                 update (const SideBar::Base* sidebar) { display_area.update (sidebar); }
        void                 update_statusbar ();
        void                 update_projection ();
        void                 set_pane (int num);
        Pane&                pane () { return (display_area.current()); }

        bool                 show_focus () const;
        bool                 sidebar_displayed () const { return (dynamic_cast<const Gtk::CheckMenuItem&> (view_menu.items()[0]).get_active()); }

        uint                axes[2];

        Slice::Info          slice;
        static Window*       Main;


      protected:
        friend class Pane;
        Gtk::VBox            main_box;
        Gtk::MenuBar         menubar;
        Gtk::Menu            file_menu, view_menu, image_menu, help_menu, colourmap_menu;
        Gtk::HPaned          paned;
        Gtk::Statusbar       statusbar;
        DisplayArea          display_area;
        SideBar::Main        sidebar;

        void                 on_file_open ();
        void                 on_file_save ();
        void                 on_file_close ();
        void                 on_file_properties ();
        void                 on_quit ();

        void                 on_view_sidebar ();
        void                 on_view_mode ();
        void                 on_view_interpolate ();
        void                 on_view_lock_to_axes ();
        void                 on_colourmap (int mode);
        void                 on_view_axial ();
        void                 on_view_sagittal ();
        void                 on_view_coronal ();
        void                 on_view_focus ();
        void                 on_view_reset_windowing ();
        void                 on_view_reset ();
        void                 on_view_full_screen ();

        void                 on_image_selected (RefPtr<Image>& R);
        void                 on_image_next ();
        void                 on_image_previous ();

        void                 on_help_about ();
        void                 on_help_OpenGL_info ();

        void                 manage (RefPtr<MR::Image::Object> obj);
    };


  }
}

#endif

