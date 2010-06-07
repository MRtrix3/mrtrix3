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

#include <gtkmm/stock.h>
#include <gtkmm/toggleaction.h>
#include <gtkmm/aboutdialog.h>
#include <gtkmm/menu_elems.h>
#include <gtkmm/radioaction.h>

#include "icon.h"
#include "dialog/file.h"
#include "mrview/window.h"
#include "mrview/image.h"
#include "mrview/colourmap.h"
#include "mrview/sidebar/main.h"
#include "mrview/dialog/error.h"
#include "mrview/dialog/progress.h"
#include "mrview/dialog/properties.h" 
#include "mrview/dialog/opengl.h" 

namespace MR {
  namespace Viewer {


    Window* Window::Main = NULL;

    Window::Window (std::vector<ArgBase>& argument)
    {
      using namespace Gtk::Menu_Helpers;

      add_events (Gdk::KEY_PRESS_MASK);
      Main = this;

      axes[0] = 3;
      axes[1] = 4;

      create_icon();
      set_icon (Icon);
      set_title ("MRView");
      set_default_size (300, 300);

      add (main_box);

      Gtk::RadioMenuItem::Group projection_group, colourmap_group;

      menubar.items().push_back (MenuElem ("_File"));
      menubar.items().back().set_submenu (file_menu);
      file_menu.items().push_back (StockMenuElem (Gtk::Stock::OPEN, sigc::mem_fun(*this, &Window::on_file_open)));
      file_menu.items().push_back (StockMenuElem (Gtk::Stock::SAVE, sigc::mem_fun(*this, &Window::on_file_save)));
      file_menu.items().push_back (StockMenuElem (Gtk::Stock::CLOSE, sigc::mem_fun(*this, &Window::on_file_close)));
      file_menu.items().push_back (SeparatorElem());
      file_menu.items().push_back (StockMenuElem (Gtk::Stock::PROPERTIES, sigc::mem_fun(*this, &Window::on_file_properties)));
      file_menu.items().push_back (SeparatorElem());
      file_menu.items().push_back (StockMenuElem (Gtk::Stock::QUIT, sigc::mem_fun(*this, &Window::on_quit)));

      file_menu.items()[1].set_sensitive (false);
      file_menu.items()[2].set_sensitive (false);
      file_menu.items()[4].set_sensitive (false);

      menubar.items().push_back (MenuElem ("_View"));
      menubar.items().back().set_submenu (view_menu);
      view_menu.items().push_back (CheckMenuElem ("Side_bar", Gtk::AccelKey ("F9"), sigc::mem_fun(*this, &Window::on_view_sidebar)));
      view_menu.items().push_back (CheckMenuElem ("_Interpolate", Gtk::AccelKey ("<control>I"), sigc::mem_fun (*this, &Window::on_view_interpolate)));
      view_menu.items().push_back (CheckMenuElem ("_Lock to image axes", sigc::mem_fun (*this, &Window::on_view_lock_to_axes)));
      view_menu.items().push_back (MenuElem ("Colour _map"));
      view_menu.items().back().set_submenu (colourmap_menu);
      view_menu.items().push_back (SeparatorElem());
      view_menu.items().push_back (RadioMenuElem (projection_group, "_Axial", Gtk::AccelKey ("A"),sigc::mem_fun (*this, &Window::on_view_axial)));
      view_menu.items().push_back (RadioMenuElem (projection_group, "_Sagittal", Gtk::AccelKey ("S"),sigc::mem_fun (*this, &Window::on_view_sagittal)));
      view_menu.items().push_back (RadioMenuElem (projection_group, "_Coronal", Gtk::AccelKey ("C"),sigc::mem_fun (*this, &Window::on_view_coronal)));
      view_menu.items().push_back (SeparatorElem());
      view_menu.items().push_back (CheckMenuElem ("Show F_ocus", Gtk::AccelKey ("<control>F"), sigc::mem_fun (*this, &Window::on_view_focus)));
      view_menu.items().push_back (MenuElem ("Reset _Windowing", Gtk::AccelKey ("<control>R"), sigc::mem_fun (*this, &Window::on_view_reset_windowing)));
      view_menu.items().push_back (MenuElem ("_Reset View", Gtk::AccelKey ("<control><shift>R"), sigc::mem_fun (*this, &Window::on_view_reset)));
      view_menu.items().push_back (CheckMenuElem ("_Full screen", Gtk::AccelKey ("F11"), sigc::mem_fun (*this, &Window::on_view_full_screen)));

      colourmap_menu.items().push_back (RadioMenuElem (colourmap_group, "_Gray", sigc::bind<int> (sigc::mem_fun (*this, &Window::on_colourmap), 0)));
      colourmap_menu.items().push_back (RadioMenuElem (colourmap_group, "_Hot", sigc::bind<int> (sigc::mem_fun (*this, &Window::on_colourmap), 1)));
      colourmap_menu.items().push_back (RadioMenuElem (colourmap_group, "_Cool", sigc::bind<int> (sigc::mem_fun (*this, &Window::on_colourmap), 2)));
      colourmap_menu.items().push_back (RadioMenuElem (colourmap_group, "_Jet", sigc::bind<int> (sigc::mem_fun (*this, &Window::on_colourmap), 3)));
      colourmap_menu.items().push_back (SeparatorElem());
      colourmap_menu.items().push_back (RadioMenuElem (colourmap_group, "_RGB", sigc::bind<int> (sigc::mem_fun (*this, &Window::on_colourmap), COLOURMAP_RGB)));
      colourmap_menu.items().push_back (RadioMenuElem (colourmap_group, "Comple_x", sigc::bind<int> (sigc::mem_fun (*this, &Window::on_colourmap), COLOURMAP_COMPLEX)));

      menubar.items().push_back (MenuElem ("_Image"));
      menubar.items().back().set_submenu (image_menu);
      image_menu.items().push_back (MenuElem ("_Next Image", Gtk::AccelKey ("<control>Page_Up"), sigc::mem_fun(*this, &Window::on_image_next)));
      image_menu.items().push_back (MenuElem ("_Previous Image", Gtk::AccelKey ("<control>Page_Down"), sigc::mem_fun(*this, &Window::on_image_previous)));
      image_menu.items().push_back (SeparatorElem ());
      image_menu.items().push_back (MenuElem ("No image loaded"));

      image_menu.items()[0].set_sensitive (false);
      image_menu.items()[1].set_sensitive (false);
      image_menu.items().back().set_sensitive (false);

      menubar.items().push_back (MenuElem ("_Help"));
      menubar.items().back().set_submenu (help_menu);
      help_menu.items().push_back (MenuElem ("_OpenGL Info", sigc::mem_fun(*this, &Window::on_help_OpenGL_info)));
      help_menu.items().push_back (StockMenuElem (Gtk::Stock::ABOUT, sigc::mem_fun(*this, &Window::on_help_about)));


      main_box.pack_start (menubar, Gtk::PACK_SHRINK);
      main_box.pack_start (paned);
      main_box.pack_start (statusbar, Gtk::PACK_SHRINK);

      paned.pack1 (display_area, true, false);
      paned.pack2 (sidebar, false, false);

      show_all();
      sidebar.hide();

      signal_key_press_event().connect (sigc::mem_fun(display_area, &DisplayArea::on_key_press), false);
      dynamic_cast<Gtk::CheckMenuItem&> (view_menu.items()[9]).set_active (true);

      realize();
      for (uint n = 0; n < argument.size(); n++) manage (argument[n].get_image());

      error = Viewer::ErrorDialog::error;
      info = Viewer::ErrorDialog::info;

      ProgressBar::init_func = ProgressDialog::init;
      ProgressBar::display_func = ProgressDialog::display;
      ProgressBar::done_func = ProgressDialog::done;

      if (images.size()) on_image_selected (images[0]);
      on_view_reset();
    }




    Window::~Window() { }




    void Window::manage (RefPtr<MR::Image::Object> obj)
    {
      using namespace Gtk::Menu_Helpers;

      RefPtr<Image> ima (new Image (obj));
      images.push_back (ima);

      if (images.size() > 1) {
        image_menu.items()[0].set_sensitive (true);
        image_menu.items()[1].set_sensitive (true);
        Gtk::RadioButtonGroup group (dynamic_cast<Gtk::RadioMenuItem&>(image_menu.items().back()).get_group());
        image_menu.items().push_back (RadioMenuElem (group, ima->image->name(), sigc::bind<RefPtr<Image> > (sigc::mem_fun (*this, &Window::on_image_selected), ima)));
      }
      else {
        Gtk::RadioButtonGroup group;
        image_menu.items().remove (image_menu.items().back());
        image_menu.items().push_back (RadioMenuElem (group, ima->image->name(), sigc::bind<RefPtr<Image> > (sigc::mem_fun (*this, &Window::on_image_selected), ima)));
      }

      file_menu.items()[1].set_sensitive (true);
      file_menu.items()[2].set_sensitive (true);
      file_menu.items()[4].set_sensitive (true);
    }





    void Window::update_statusbar ()
    {
      statusbar.pop ();
      const Slice::Current S (pane());
      if (!S.image) return;
      if (!S.focus) return;

      MR::Image::Interp &I (*S.image->interp);
      MR::Image::Position &P (*S.image->interp);
      
      Point pix (I.R2P (S.focus));
      std::string pos;
      float value = NAN;
      if (S.orientation) {
        I.P (pix);
        value = I.value();
        pos = printf ("%.2f %.2f %.2f ", pix[0], pix[1], pix[2]);
        for (int n = 3; n < I.ndim(); n++) pos += str (I.dim(n)) + " ";
      }
      else {
        P.set (0, round (pix[0]));
        P.set (1, round (pix[1]));
        P.set (2, round (pix[2]));
        value = !P ? NAN : P.value();
        for (int n = 0; n < P.ndim(); n++) pos += str (P[n]) + " ";
      }

      statusbar.push (printf ("position: [ %.2f %.2f %.2f ] mm, voxel: [ %s], value: %.4g", 
            S.focus[0], S.focus[1], S.focus[2], pos.c_str(), value));
    }





    void Window::on_file_open ()
    {
      Dialog::File dialog ("Open Images", true, true);

      if (dialog.run() == Gtk::RESPONSE_OK) {
        std::vector<RefPtr<MR::Image::Object> > selection = dialog.get_images();
        if (selection.size()) {
          int first = images.size();
          for (uint n = 0; n < selection.size(); n++) 
            manage (selection[n]);
          dynamic_cast<Gtk::RadioMenuItem&> (image_menu.items()[3+first]).set_active (true);
          on_image_selected (images[first]);
        }
      }
    }





    void Window::on_file_save () 
    { 
      const Slice::Current S (pane());
      if (!S.image) return;

      Dialog::File dialog ("Save Image", false, false);

      if (dialog.run() == Gtk::RESPONSE_OK) {
        std::vector<std::string> selection = dialog.get_selection();
        if (selection.size()) {
          try {
            MR::Image::Object obj;
            MR::Image::Header header (S.image->image->header());
            obj.create (selection[0], header);
            MR::Image::Position out (obj);
            MR::Image::Position in (*S.image->image);
            ProgressBar::init (out.voxel_count(), "saving image \"" + obj.name());
            do {
              out.value (in.value());
              in++;
              ProgressBar::inc();
            } while (out++);
            ProgressBar::done();
          }
          catch (...) { }
        }
      }
    }
    




    
    void Window::on_file_close ()
    {
      uint n;
      for (n = 0; n < images.size(); n++) {
        if (images[n] == image) {

          images.erase (images.begin() + n);
          image_menu.items().remove (image_menu.items()[3+n]);

          if (!images.size()) {
            image_menu.items().push_back (Gtk::Menu_Helpers::MenuElem ("No image loaded"));
            image_menu.items().back().set_sensitive (false);
            file_menu.items()[1].set_sensitive (false);
            file_menu.items()[2].set_sensitive (false);
            file_menu.items()[4].set_sensitive (false);
            image_menu.items()[0].set_sensitive (false);
            image_menu.items()[1].set_sensitive (false);
            RefPtr<Image> none (NULL);
            on_image_selected (none);
          }
          else {
            if (n >= images.size()) n--;
            if (images.size() < 2) {
              image_menu.items()[0].set_sensitive (false);
              image_menu.items()[1].set_sensitive (false);
            }
            dynamic_cast<Gtk::RadioMenuItem&> (image_menu.items()[3+n]).set_active (true);
          }

          return;
        }
      }
    }


    void Window::on_file_properties ()
    {
      const Slice::Current S (pane());
      if (!S.image) return;

      PropertiesDialog dialog (*S.image->image);
      dialog.run();
    }




    void Window::on_quit () { hide(); }

    void Window::on_view_sidebar () 
    { 
      if (sidebar_displayed()) {
        sidebar.show();
        if (sidebar.selector.get_active_row_number() < 0) sidebar.selector.set_active (0);
      }
      else sidebar.hide();
    }


    void Window::on_view_mode () { TEST; }

    void Window::on_colourmap (int mode) { Slice::Current S (pane()); if (!S.image) return; S.colourmap = mode; update(); }


    void Window::on_view_axial () 
    { 
      Slice::Current S (pane());
      if (!S.image) return;
      if (dynamic_cast<Gtk::RadioMenuItem&> (view_menu.items()[5]).get_active()) { S.projection = 2; update(); }
    }


    void Window::on_view_sagittal () 
    { 
      Slice::Current S (pane());
      if (!S.image) return;
      if (dynamic_cast<Gtk::RadioMenuItem&> (view_menu.items()[6]).get_active()) { S.projection = 0; update(); }
    }


    void Window::on_view_coronal () 
    { 
      Slice::Current S (pane());
      if (!S.image) return;
      if (dynamic_cast<Gtk::RadioMenuItem&> (view_menu.items()[7]).get_active()) { S.projection = 1; update(); }
    }


    void Window::on_view_interpolate () 
    {
      Slice::Current S (pane());
      if (!S.image) return;
      S.interpolate = dynamic_cast<Gtk::CheckMenuItem&> (view_menu.items()[1]).get_active(); 
      update();
    }


    void Window::on_view_lock_to_axes () 
    { 
      Slice::Current S (pane());
      if (!S.image) return;

      if (dynamic_cast<Gtk::CheckMenuItem&> (view_menu.items()[2]).get_active()) S.orientation.invalidate();
      else { 
        const Math::Matrix &M (S.image->image->I2R());
        float matrix[] = { 
          -M(0,0), M(0,1), -M(0,2),
          -M(1,0), M(1,1), -M(1,2),
          -M(2,0), M(2,1), -M(2,2)
        };
        S.orientation.from_matrix (matrix);
      }

      update();
    }





    void Window::on_view_focus () { update(); }





    void Window::on_view_reset_windowing () 
    { 
      Slice::Current S (pane());
      if (!S.image) return;
      S.scaling.reset(); 
      update();
    }



    void Window::on_view_reset () 
    { 
      Slice::Current S (pane());
      if (!S.image) return;
      if (S.orientation) { 
        const Math::Matrix &M (S.image->image->I2R());
        float matrix[] = { 
          -M(0,0), M(0,1), -M(0,2),
          -M(1,0), M(1,1), -M(1,2),
          -M(2,0), M(2,1), -M(2,2)
        };
        S.orientation.from_matrix (matrix);
      }
      S.focus.invalidate();
      pane().FOV = NAN;
      const MR::Image::Object& ima (*S.image->image);
      S.projection = minindex (ima.dim(0)*ima.vox(0), ima.dim(1)*ima.vox(1), ima.dim(2)*ima.vox(2));
      update_projection();
      update();
    }




    void Window::on_view_full_screen () 
    { 
      if (dynamic_cast<Gtk::CheckMenuItem&> (view_menu.items().back()).get_active()) fullscreen();
      else unfullscreen();
    }





    void Window::on_image_selected (RefPtr<Image>& R) 
    { 
      if (image == R) return;

      image = R;
      Slice::Current current (pane());
      current.image = image;

      const Slice::Current S (pane());
      if (S.image) {
        set_title (image->image->name());
        dynamic_cast<Gtk::CheckMenuItem&> (view_menu.items()[1]).set_active (S.interpolate);
        dynamic_cast<Gtk::CheckMenuItem&> (view_menu.items()[2]).set_active (!S.orientation);
      }
      else set_title ("MRView");
      update();
    }



    void Window::on_image_next () 
    { 
      if (images.size() < 2) return;

      uint n;
      for (n = 0; n < images.size(); n++) 
        if (images[n] == image) break;

      n = (n+1) % images.size();
      dynamic_cast<Gtk::CheckMenuItem&> (image_menu.items()[n+3]).set_active (true);
    }




    void Window::on_image_previous ()
    { 
      if (images.size() < 2) return;

      uint n;
      for (n = 0; n < images.size(); n++) 
        if (images[n] == image) break;

      n = (n+images.size()-1) % images.size();
      dynamic_cast<Gtk::CheckMenuItem&> (image_menu.items()[n+3]).set_active (true);
    }






    void Window::on_help_OpenGL_info () 
    {
      OpenGLInfo dialog;
      dialog.run();
    }




    void Window::on_help_about () 
    { 
      Gtk::AboutDialog dialog;

      dialog.set_name ("MRView");
      dialog.set_icon (Icon);
      std::vector<std::string> slist;
      slist.push_back (App::author);
      dialog.set_authors (slist);
      dialog.set_comments ("The MRtrix image viewer");
      dialog.set_version (MR::printf ("%d.%d.%d", mrtrix_major_version, mrtrix_minor_version, mrtrix_micro_version));

      dialog.run();
    }
    



    void Window::set_pane (int num)
    {
      /*
      splitter->set_segment (panes[num]);
      */
    }


    void Window::update_projection ()
    {
      const Slice::Current S (pane());
      if (!S.image) return;
      switch (S.projection) {
        case 0: dynamic_cast<Gtk::CheckMenuItem&> (view_menu.items()[6]).set_active(true); return;
        case 1: dynamic_cast<Gtk::CheckMenuItem&> (view_menu.items()[7]).set_active(true); return;
        case 2: dynamic_cast<Gtk::CheckMenuItem&> (view_menu.items()[5]).set_active(true); return;
        default: return;
      }
    }






    bool Window::show_focus () const { return (dynamic_cast<Gtk::CheckMenuItem&> (view_menu.items()[9]).get_active()); }


  }
}

