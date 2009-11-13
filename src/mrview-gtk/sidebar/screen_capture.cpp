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


    24-07-2008 J-Donald Tournier <d.tournier@brain.org.au>
    * added support for overlay of orientation plot on main window

    09-07-2008 J-Donald Tournier <d.tournier@brain.org.au>
    * set color using config file

    08-07-2008 J-Donald Tournier <d.tournier@brain.org.au>
    * add option to disable tri-linear interpolation

*/

#include "file/config.h"
#include "dialog/file.h"
#include "mrview/window.h"
#include "mrview/sidebar/screen_capture.h"

namespace MR {
  namespace Viewer {
    namespace SideBar {

      ScreenCapture::ScreenCapture () : 
        Base (100000000),
        snapshot_button ("grab"),
        destination_button ("set output..."),
        cancel_button ("cancel"),
        destination_prefix_frame ("prefix"),
        destination_folder_frame ("folder"),
        multislice_label (" slices: "),
        slice_separation_label (" spacing: "),
        oversample_label (" OS: "),
        destination_prefix_label ("screenshot"),
        destination_folder_label (Glib::path_get_basename (Dialog::File::get_cwd())),
        destination_number_label (" number: "),
        layout_table (3,2),
        multislice_adjustment (1, 1, 1000, 1, 10),
        slice_separation_adjustment (1, 0.01, 100.0, 0.01, 1.0),
        oversample_adjustment (1, 1, 10, 1, 1),
        destination_number_adjustment (0, 0, 9999, 1, 10),
        multislice (multislice_adjustment),
        slice_separation (slice_separation_adjustment, 0.01, 2),
        oversample (oversample_adjustment),
        destination_number (destination_number_adjustment),
        framebuffer (NULL),
        number_remaining (0),
        OS (1),
        OS_x (0),
        OS_y (0)
      { 
        prefix = Glib::build_filename (Dialog::File::get_cwd(), destination_prefix_label.get_text() + "-");

        layout_table.attach (multislice_label, 0, 1, 0, 1, Gtk::SHRINK | Gtk::FILL);
        layout_table.attach (multislice, 1, 2, 0, 1, Gtk::SHRINK | Gtk::FILL);
        layout_table.attach (slice_separation_label, 0, 1, 1, 2, Gtk::SHRINK | Gtk::FILL);
        layout_table.attach (slice_separation, 1, 2, 1, 2, Gtk::SHRINK | Gtk::FILL);
        layout_table.attach (oversample_label, 0, 1, 2, 3, Gtk::SHRINK | Gtk::FILL);
        layout_table.attach (oversample, 1, 2, 2, 3, Gtk::SHRINK | Gtk::FILL);

        destination_prefix_frame.add (destination_prefix_label);
        destination_folder_frame.add (destination_folder_label);

        destination_number_box.pack_start (destination_number_label, Gtk::PACK_SHRINK);
        destination_number_box.pack_start (destination_number);

        multislice_label.set_tooltip_text ("number of slices to capture");
        slice_separation_label.set_tooltip_text ("slice separation");
        oversample_label.set_tooltip_text ("oversampling factor");
        destination_number_label.set_tooltip_text ("start numbering output files from this number");
        destination_prefix_label.set_tooltip_text ("screenshots will be saved as \"prefix-<number>.png\"");
        destination_folder_label.set_tooltip_text (Dialog::File::get_cwd());
        destination_button.set_tooltip_text ("set destination prefix and folder");

        pack_start (layout_table, Gtk::PACK_SHRINK);
        pack_start (destination_prefix_frame, Gtk::PACK_SHRINK);
        pack_start (destination_folder_frame, Gtk::PACK_SHRINK);
        pack_start (destination_number_box, Gtk::PACK_SHRINK);
        pack_start (destination_button, Gtk::PACK_SHRINK);
        pack_start (snapshot_button, Gtk::PACK_SHRINK);
        pack_start (cancel_button, Gtk::PACK_SHRINK);

        show_all();
        snapshot_button.signal_clicked().connect (sigc::mem_fun (*this, &ScreenCapture::on_snapshot));
        destination_button.signal_clicked().connect (sigc::mem_fun (*this, &ScreenCapture::on_browse));
        cancel_button.signal_clicked().connect (sigc::mem_fun (*this, &ScreenCapture::on_cancel));
        cancel_button.set_sensitive (false);
        Window::Main->pane().activate (this);
      }

      

      ScreenCapture::~ScreenCapture () {  }





      void ScreenCapture::draw ()
      {
        if (number_remaining) {
          Pane& pane (Window::Main->pane());
          Slice::Current S (pane);
          if (!S.image) {
            number_remaining = 0;
            return;
          }

          snapshot();

          if (OS > 1) {
            OS_x++;
            if (OS_x >= OS) {
              OS_x = 0;
              OS_y++;
              if (OS_y >= OS) {
                OS_y = 0;
                S.focus += normal;
                number_remaining--;
              }
            }
            pane.mode->set_oversampling (OS, OS_x, OS_y);
          }
          else {
            S.focus += normal;
            number_remaining--;
          }

          if (!number_remaining) on_cancel(); 

          Window::Main->update();
        }
      }




      void ScreenCapture::on_snapshot ()
      {
        Pane& pane (Window::Main->pane());
        Slice::Current S (pane);
        if (!S.image) return;

        number_remaining = multislice.get_value_as_int();
        OS = oversample.get_value_as_int();
        OS_x = OS_y = 0;
        previous_focus = S.focus;

        width = pane.width();
        height = pane.height();
        framebuffer = new unsigned char [3*width*height];
        pix = Gdk::Pixbuf::create (Gdk::COLORSPACE_RGB, false, 8, OS*width, OS*height);

        if (number_remaining == 1 && OS == 1) {
          snapshot ();
          on_cancel();
          return;
        }

        if (number_remaining > 1) {
          const GLdouble* mv = pane.get_modelview();
          normal.set (mv[2], mv[6], mv[10]);
          normal *= slice_separation.get_value();
          S.focus -= (number_remaining/2.0) * normal;
        }

        if (OS > 1) pane.mode->set_oversampling (OS, OS_x, OS_y);

        cancel_button.set_sensitive (true);
        snapshot_button.set_sensitive (false);
        Window::Main->update();
      }





      void ScreenCapture::snapshot ()
      {
        Pane& pane (Window::Main->pane());

        pane.gl_start();
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, framebuffer);
        pane.gl_end();

        for (int i = 0; i < height; i++) 
          memcpy (pix->get_pixels() + 3*width*OS_x + (height*(OS-OS_y-1)+i)*pix->get_rowstride(), framebuffer+3*(height-i-1)*width, 3*width);

        if (OS_x == OS-1 && OS_y == OS-1) {
          pix->save (prefix + printf("%04d.png", destination_number.get_value_as_int()), "png");
          destination_number.set_value (destination_number.get_value_as_int()+1);
        }
      }






      void ScreenCapture::on_browse ()
      {
        Dialog::File dialog ("Set screenshot prefix", false, false);
        dialog.set_selection (destination_prefix_label.get_text());
        if (dialog.run() == Gtk::RESPONSE_OK) {
          std::vector<std::string> selection = dialog.get_selection();
          if (selection.size()) {
            prefix = selection[0] + "-";
            destination_prefix_label.set_text (Glib::path_get_basename (selection[0]));
            destination_folder_label.set_text (Glib::path_get_basename (dialog.get_cwd()));
            destination_folder_label.set_tooltip_text (dialog.get_cwd());
            destination_number.set_value (0);
          }
        }
      }





      void ScreenCapture::on_cancel ()
      {
        Pane& pane (Window::Main->pane());
        Slice::Current S (pane);
        pix.reset();
        if (framebuffer) delete [] framebuffer;
        framebuffer = NULL;
        number_remaining = 0;
        OS = 1; OS_x = OS_y = 0;
        S.focus = previous_focus;
        pane.mode->set_oversampling ();
        cancel_button.set_sensitive (false);
        snapshot_button.set_sensitive (true);
        Window::Main->update();
      }



    }
  }
}

