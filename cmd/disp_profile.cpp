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


    07-07-2008 J-Donald Tournier <d.tournier@brain.org.au>
    * remove use of Math::Matrix::transpose() 
      to flip row vector (transpose expects square matrices)

    17-10-2008 J-Donald Tournier <d.tournier@brain.org.au>
    * add option -response to display response function
  
    24-10-2008 J-Donald Tournier <d.tournier@brain.org.au>
    * add menu entry to normalise plot amplitude
    * set title based on row of SH coefficients currently displayed
    * use shift modifier to scroll through 10 rows at a time
    * set lmax based on SH values provided
    
*/

#include <gtk/gtkgl.h>
#include <gtkmm/main.h>
#include <gtkmm/window.h>
#include <gtkmm/box.h>
#include <gtkmm/menu_elems.h>
#include <gtkmm/menubar.h>
#include <gtkmm/menu.h>
#include <gtkmm/stock.h>

#include "app.h"
#include "icon.h"
#include "dwi/render_frame.h"

using namespace std; 
using namespace MR; 

SET_VERSION_DEFAULT;

DESCRIPTION = {
  "view spherical harmonics surface plots.",
  NULL
};

ARGUMENTS = {
  Argument ("coefs", "SH coefficients", "a text file containing the even spherical harmonics coefficients to display.").type_file (),
  Argument::End
};

OPTIONS = { 
  Option ("response", "display response function", "assume SH coefficients file only contains even, m=0 terms. Used to display the response function as produced by estimate_response"),

  Option::End 
};





class Window : public Gtk::Window 
{
  public:
    Window (const std::string& title, const Math::Matrix& coefs) : name (title), current (0), values (coefs)
    {
      using namespace Gtk::Menu_Helpers;

      create_icon();
      set_icon (Icon);
      set_title (name + " [ 0 ]");
      set_default_size (300, 300);
      add (main_box);

      menubar.items().push_back (MenuElem ("_Settings"));
      menubar.items().back().set_submenu (settings_menu);
      settings_menu.items().push_back (CheckMenuElem ("_Lighting", Gtk::AccelKey ("L"), sigc::mem_fun (*this, &Window::on_use_lighting)));
      settings_menu.items().push_back (CheckMenuElem ("Show _axes", Gtk::AccelKey ("A"), sigc::mem_fun (*this, &Window::on_show_axes)));
      settings_menu.items().push_back (CheckMenuElem ("_Hide negative lobes", Gtk::AccelKey ("H"), sigc::mem_fun (*this, &Window::on_hide_negative_lobes)));
      settings_menu.items().push_back (CheckMenuElem ("_Colour by direction", Gtk::AccelKey ("C"), sigc::mem_fun (*this, &Window::on_colour_by_direction)));
      settings_menu.items().push_back (CheckMenuElem ("_Normalise", Gtk::AccelKey ("N"), sigc::mem_fun (*this, &Window::on_normalise)));
      settings_menu.items().push_back (SeparatorElem());
      settings_menu.items().push_back (MenuElem ("Harmonic _order"));
      settings_menu.items().back().set_submenu (lmax_menu);
      settings_menu.items().push_back (MenuElem ("Level of _detail"));
      settings_menu.items().back().set_submenu (lod_menu);
      settings_menu.items().push_back (SeparatorElem());
      settings_menu.items().push_back (StockMenuElem (Gtk::Stock::QUIT, sigc::mem_fun(*this, &Window::on_quit)));

      Gtk::RadioMenuItem::Group lmax_group, lod_group;
      lmax_menu.items().push_back (RadioMenuElem (lmax_group, "2", sigc::bind<int> (sigc::mem_fun (*this, &Window::on_lmax), 2)));
      lmax_menu.items().push_back (RadioMenuElem (lmax_group, "4", sigc::bind<int> (sigc::mem_fun (*this, &Window::on_lmax), 4)));
      lmax_menu.items().push_back (RadioMenuElem (lmax_group, "6", sigc::bind<int> (sigc::mem_fun (*this, &Window::on_lmax), 6)));
      lmax_menu.items().push_back (RadioMenuElem (lmax_group, "8", sigc::bind<int> (sigc::mem_fun (*this, &Window::on_lmax), 8)));
      lmax_menu.items().push_back (RadioMenuElem (lmax_group, "10", sigc::bind<int> (sigc::mem_fun (*this, &Window::on_lmax), 10)));
      lmax_menu.items().push_back (RadioMenuElem (lmax_group, "12", sigc::bind<int> (sigc::mem_fun (*this, &Window::on_lmax), 12)));
      lmax_menu.items().push_back (RadioMenuElem (lmax_group, "14", sigc::bind<int> (sigc::mem_fun (*this, &Window::on_lmax), 14)));
      lmax_menu.items().push_back (RadioMenuElem (lmax_group, "16", sigc::bind<int> (sigc::mem_fun (*this, &Window::on_lmax), 16)));

      lod_menu.items().push_back (RadioMenuElem (lod_group, "3", sigc::bind<int> (sigc::mem_fun (*this, &Window::on_lod), 3)));
      lod_menu.items().push_back (RadioMenuElem (lod_group, "4", sigc::bind<int> (sigc::mem_fun (*this, &Window::on_lod), 4)));
      lod_menu.items().push_back (RadioMenuElem (lod_group, "5", sigc::bind<int> (sigc::mem_fun (*this, &Window::on_lod), 5)));
      lod_menu.items().push_back (RadioMenuElem (lod_group, "6", sigc::bind<int> (sigc::mem_fun (*this, &Window::on_lod), 6)));
      lod_menu.items().push_back (RadioMenuElem (lod_group, "7", sigc::bind<int> (sigc::mem_fun (*this, &Window::on_lod), 7)));

      signal_key_press_event().connect (sigc::mem_fun(*this, &Window::on_key_press), false);

      main_box.pack_start (menubar, Gtk::PACK_SHRINK);
      main_box.pack_start (render);

      show_all();
      realize();

      std::vector<float> val (values.columns());
      for (uint n = 0; n < values.columns(); n++)
        val[n] = values(0,n);

      render.set (val);

      dynamic_cast<Gtk::CheckMenuItem&> (settings_menu.items()[0]).set_active(); 
      dynamic_cast<Gtk::CheckMenuItem&> (settings_menu.items()[1]).set_active(); 
      dynamic_cast<Gtk::CheckMenuItem&> (settings_menu.items()[2]).set_active(); 
      dynamic_cast<Gtk::CheckMenuItem&> (settings_menu.items()[3]).set_active(); 
      dynamic_cast<Gtk::CheckMenuItem&> (settings_menu.items()[4]).set_active(); 

      dynamic_cast<Gtk::RadioMenuItem&> (lmax_menu.items()[DWI::SH::LforN(values.columns())/2-1]).set_active(); 
      dynamic_cast<Gtk::RadioMenuItem&> (lod_menu.items()[2]).set_active(); 
    }


  protected:
      DWI::RenderFrame render;
      Gtk::VBox     main_box;
      Gtk::MenuBar  menubar;
      Gtk::Menu     settings_menu, lod_menu, lmax_menu;

      std::string name;
      int current;
      const Math::Matrix& values;

      void   on_quit() { hide(); }
      void   on_use_lighting () { render.set_use_lighting (dynamic_cast<Gtk::CheckMenuItem&> (settings_menu.items()[0]).get_active()); }
      void   on_show_axes () { render.set_show_axes (dynamic_cast<Gtk::CheckMenuItem&> (settings_menu.items()[1]).get_active()); }
      void   on_hide_negative_lobes () { render.set_hide_neg_lobes (dynamic_cast<Gtk::CheckMenuItem&> (settings_menu.items()[2]).get_active()); }
      void   on_colour_by_direction () { render.set_color_by_dir (dynamic_cast<Gtk::CheckMenuItem&> (settings_menu.items()[3]).get_active()); }
      void   on_normalise () { render.set_normalise (dynamic_cast<Gtk::CheckMenuItem&> (settings_menu.items()[4]).get_active()); }
      void   on_lod (int lod) { render.set_LOD (lod); }
      void   on_lmax (int lmax) { render.set_lmax (lmax); }

      bool on_key_press (GdkEventKey* event)
      {
        if (event->is_modifier) return (false);
        if (event->keyval == GDK_Left) {
          int c = current - ((event->state & MODIFIERS) == GDK_SHIFT_MASK ? 10 : 1);
          if (c < 0) { if (current == 0) return (true); current = 0; } else current = c;
          std::vector<float> val (values.columns());
          for (uint n = 0; n < values.columns(); n++)
            val[n] = values (current,n);
          render.set (val);
          set_title (name + " [ " + str(current) + " ]");
          return (true);
        }
        if (event->keyval == GDK_Right) {
          int c = current + ((event->state & MODIFIERS) == GDK_SHIFT_MASK ? 10 : 1);
          if (c >= int (values.rows())) { if (current == (int) values.rows()-1) return (true); current = values.rows()-1; } else current = c;
          std::vector<float> val (values.columns());
          for (uint n = 0; n < values.columns(); n++)
            val[n] = values (current,n);
          render.set (val);
          set_title (name + " [ " + str(current) + " ]");
          return (true);
        }
        return (false);
      }
};





class MyApp : public MR::App { 
  public: 
    MyApp (int argc, char** argv) : App (argc, argv, __command_description, __command_arguments, __command_options, 
        __command_version, __command_author, __command_copyright) { } 

    void execute () { 
      Math::Matrix values;
      values.load (argument[0].get_string());
      if (values.columns() == 1) {
        Math::Matrix tmp (values.columns(), values.rows());
        for (uint r = 0; r < values.rows(); r++)
          for (uint c = 0; c < values.columns(); c++)
            tmp(c,r) = values(r,c);
        values = tmp;
      }

      std::vector<OptBase> opt = get_options (0); // response
      if (opt.size()) {
        Math::Matrix R (values);
        values.allocate (R.rows(), DWI::SH::NforL (2*(R.columns()-1)));
        values.zero();
        for (uint n = 0; n < R.rows(); n++) 
          for (uint i = 0; i < R.columns(); i++) 
            values(n, DWI::SH::index(2*i,0)) = R(n,i);
      }


      Window window (Glib::path_get_basename (argument[0].get_string()), values);
      Gtk::Main::run (window);
    }

    void init () { parse_arguments(); }
}; 




int main (int argc, char* argv[]) 
{ 
  try { 
    MyApp app (argc, argv);  

    Gtk::Main kit (&argc, &argv);
    gtk_gl_init (&argc, &argv);
    app.init();

    app.execute();
  }
  catch (Exception) { return (1); } 
  catch (int ret) { return (ret); } 
  return (0); 
}


