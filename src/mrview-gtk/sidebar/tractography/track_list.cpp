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

#include <gtkmm/treerowreference.h>
#include <gtkmm/colorselection.h>
#include <gtkmm/stock.h>

#include "mrview/sidebar/tractography/track_list.h"
#include "mrview/sidebar/tractography.h"
#include "mrview/window.h"
#include "math/rng.h"
#include "dialog/file.h"


#ifdef WINDOWS
PFNGLBLENDEQUATIONPROC pglBlendEquation = NULL;
PFNGLBLENDCOLORPROC pglBlendColor = NULL;
#endif 

namespace MR {
  namespace Viewer {
    namespace SideBar {

      TrackListItem::Track::Point* TrackList::root  = NULL;




      TrackList::TrackList (const Tractography& sidebar) : 
        parent (sidebar) 
      {
        using namespace Gtk::Menu_Helpers;
#ifdef WINDOWS
        pglBlendEquation = (PFNGLBLENDEQUATIONPROC) wglGetProcAddress ("glBlendEquation");
        pglBlendColor = (PFNGLBLENDCOLORPROC) wglGetProcAddress ("glBlendColor");
#endif

        popup_menu.items().push_back (StockMenuElem(Gtk::Stock::OPEN, sigc::mem_fun(*this, &TrackList::on_open) ) );
        popup_menu.items().push_back (StockMenuElem(Gtk::Stock::CLOSE, sigc::mem_fun(*this, &TrackList::on_close) ) );
        popup_menu.items().push_back (SeparatorElem());
        popup_menu.items().push_back (MenuElem("Colour by _direction", sigc::mem_fun(*this, &TrackList::on_colour_by_direction)));
        popup_menu.items().push_back (MenuElem("_Randomise colour", sigc::mem_fun(*this, &TrackList::on_randomise_colour)));
        popup_menu.items().push_back (MenuElem("_Set colour", sigc::mem_fun(*this, &TrackList::on_set_colour)));
        popup_menu.items().push_back (SeparatorElem());
        popup_menu.items().push_back (StockMenuElem(Gtk::Stock::CLEAR, sigc::mem_fun(*this, &TrackList::on_clear) ) );
        popup_menu.accelerate (*this);

        model = Gtk::ListStore::create (columns);
        set_model (model);

        int tick_column_index = append_column_editable ("", columns.show) - 1;
        append_column ("", columns.pix);
        append_column ("file", columns.name);

        set_headers_visible (false);
        get_selection()->set_mode (Gtk::SELECTION_MULTIPLE);

        colour_by_dir_pixbuf = Gdk::Pixbuf::create_from_data (colour_by_dir_data, Gdk::COLORSPACE_RGB, true, 8, 16, 16, 16*4); 

        Gtk::CellRendererToggle* tick = dynamic_cast<Gtk::CellRendererToggle*> (get_column_cell_renderer (tick_column_index));
        tick->signal_toggled().connect (sigc::mem_fun (*this, &TrackList::on_tick));

        Glib::signal_timeout().connect (sigc::mem_fun (*this, &TrackList::on_refresh), 3000);

        set_tooltip_text ("right-click for more options");
      }






      TrackList::~TrackList()  { }





      void TrackList::draw ()
      {
        Gtk::TreeModel::Children tracks = model->children();
        if (tracks.size() == 0) return;

        Pane& pane (Window::Main->pane());

        float thickness = parent.crop_to_slice.get_active() ? parent.slab_thickness.get_value() : INFINITY; 
        bool  depth_blend = parent.depth_blend.get_active();

        glDisable (GL_LINE_SMOOTH);
        glEnable (GL_DEPTH_TEST);

        const GLdouble* mv = pane.get_modelview();

        if (depth_blend) {
          Point& normal (TrackListItem::Track::Point::normal);
          normal.set (mv[2], mv[6], mv[10]);
          const Slice::Current S (pane);
          float Z = normal.dot (S.focus);

          if (vertices.empty() || Z != previous_Z || normal != previous_normal) {

            if (finite (thickness) || vertices.empty()) {
              uint count = 0;
              for (Gtk::TreeModel::Children::iterator iter = tracks.begin(); iter != tracks.end(); ++iter) {
                bool show = (*iter)[columns.show];
                if (show) {
                  RefPtr<TrackListItem> tck = (*iter)[columns.track];
                  count += tck->count();
                }
              }
              vertices.clear();
              vertices.reserve (count);

              for (Gtk::TreeModel::Children::iterator iter = tracks.begin(); iter != tracks.end(); ++iter) {
                bool show = (*iter)[columns.show];
                if (show) {
                  RefPtr<TrackListItem> tck = (*iter)[columns.track];
                  tck->add (vertices, Z-0.5*thickness, Z+0.5*thickness);
                }
              }
            }

            std::sort (vertices.begin(), vertices.end(), compare);

            previous_Z = Z;
            previous_normal = normal;
          }

          for (Gtk::TreeModel::Children::iterator iter = tracks.begin(); iter != tracks.end(); ++iter) {
            bool show = (*iter)[columns.show];
            if (show) {
              RefPtr<TrackListItem> tck = (*iter)[columns.track];
              tck->update_RGBA();
            }
          }


          glEnable (GL_BLEND);
          glDepthMask (GL_FALSE);
          glBlendEquation (GL_FUNC_ADD);
          glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
          glEnable (GL_POINT_SMOOTH);
          glPointSize (parent.line_thickness.get_value() * pane.mode->get_oversampling());
          glEnableClientState (GL_VERTEX_ARRAY);
          glEnableClientState (GL_COLOR_ARRAY);

          glVertexPointer (3, GL_FLOAT, sizeof(TrackListItem::Track::Point), root[0].pos);
          glColorPointer (4, GL_UNSIGNED_BYTE, sizeof(TrackListItem::Track::Point), root[0].C);
          glDrawElements (GL_POINTS, vertices.size(), GL_UNSIGNED_INT, &vertices[0]);

          glDisableClientState (GL_VERTEX_ARRAY);
          glDisableClientState (GL_COLOR_ARRAY);
          glDisable (GL_POINT_SMOOTH);
        }
        else {
          if (pane.mode->type() == 0 && finite (thickness)) {
            const Slice::Current S (pane);

            GLdouble n[4];
            n[0] = -mv[2];
            n[1] = -mv[6];
            n[2] = -mv[10];
            n[3] = 0.5*thickness - n[0]*S.focus[0] - n[1]*S.focus[1] - n[2]*S.focus[2];
            glClipPlane (GL_CLIP_PLANE0, n);
            glEnable (GL_CLIP_PLANE0);

            n[0] = -n[0];
            n[1] = -n[1];
            n[2] = -n[2];
            n[3] = thickness - n[3];
            glClipPlane (GL_CLIP_PLANE1, n);
            glEnable (GL_CLIP_PLANE1);
          }


          glLineWidth (parent.line_thickness.get_value());
          glShadeModel (GL_SMOOTH);
          glEnableClientState (GL_VERTEX_ARRAY);
          glEnableClientState (GL_COLOR_ARRAY);

          glDisable (GL_BLEND);
          glDepthMask (GL_TRUE);
          for (Gtk::TreeModel::Children::iterator iter = tracks.begin(); iter != tracks.end(); ++iter) {
            bool show = (*iter)[columns.show];
            if (show) {
              RefPtr<TrackListItem> tck = (*iter)[columns.track];
              if (tck->alpha == 1.0) tck->draw();
            }
          }

          glEnable (GL_BLEND);
          glDepthMask (GL_FALSE);
          glBlendEquation (GL_FUNC_ADD);
          glBlendFunc (GL_CONSTANT_ALPHA, GL_ONE);

          for (Gtk::TreeModel::Children::iterator iter = tracks.begin(); iter != tracks.end(); ++iter) {
            bool show = (*iter)[columns.show];
            if (show) {
              RefPtr<TrackListItem> tck = (*iter)[columns.track];
              if (tck->alpha < 1.0) tck->draw();
            }
          }

          glDisable (GL_BLEND);
          glDepthMask (GL_TRUE);

          glDisableClientState (GL_VERTEX_ARRAY);
          glDisableClientState (GL_COLOR_ARRAY);
          glDisable (GL_CLIP_PLANE0);
          glDisable (GL_CLIP_PLANE1);
        }
      }






      void TrackList::load (const std::string& filename)
      {
        RefPtr<TrackListItem> track (new TrackListItem);
        try { track->load (filename); }
        catch (...) { return; }


        Gtk::TreeModel::Row row = *(model->append());
        row[columns.show] = true;
        row[columns.pix] = colour_by_dir_pixbuf;
        row[columns.name] = Glib::path_get_basename (filename);
        row[columns.track] = track;

        vertices.clear();
        get_selection()->select (row);

        Window::Main->update (&parent);
      }







      bool TrackList::on_button_press_event (GdkEventButton* event)
      {
        if ((event->type == GDK_BUTTON_PRESS) && (event->button == 3)) {
          Gtk::TreeModel::Path path;
          Gtk::TreeViewColumn* col;
          int x, y;
          bool is_row = get_path_at_pos ((int) event->x, (int) event->y, path, col, x, y);
          if (is_row) {
            if (!get_selection()->is_selected (path)) 
              TreeView::on_button_press_event(event);
          }
          else get_selection()->unselect_all();

          popup_menu.items()[1].set_sensitive (is_row);
          popup_menu.items()[3].set_sensitive (is_row);
          popup_menu.items()[4].set_sensitive (is_row);
          popup_menu.popup (event->button, event->time);
          return (true);
        }
        return (TreeView::on_button_press_event(event));
      }







      void TrackList::on_open () 
      {
        Dialog::File dialog ("Open Tracks", true, false);

        if (dialog.run() == Gtk::RESPONSE_OK) {
          std::vector<std::string> selection = dialog.get_selection();
          if (selection.size()) 
            for (uint n = 0; n < selection.size(); n++) 
              load (selection[n]);
        }
      }







      void TrackList::on_close ()
      {
        std::list<Gtk::TreeModel::Path> paths = get_selection()->get_selected_rows();
        std::list<Gtk::TreeModel::RowReference> rows;

        for (std::list<Gtk::TreeModel::Path>::iterator row = paths.begin(); row != paths.end(); row++) 
          rows.push_back (Gtk::TreeModel::RowReference (model, *row));

        for (std::list<Gtk::TreeModel::RowReference>::iterator row = rows.begin(); row != rows.end(); row++) 
          model->erase (model->get_iter (row->get_path()));

        vertices.clear();
        Window::Main->update (&parent);
      }





      void TrackList::on_clear () 
      {
        model->clear(); 
        vertices.clear();
        Window::Main->update (&parent);
      }





      void TrackList::on_colour_by_direction () 
      {
        bool update = false;
        std::list<Gtk::TreeModel::Path> paths = get_selection()->get_selected_rows();
        for (std::list<Gtk::TreeModel::Path>::iterator i = paths.begin(); i != paths.end(); ++i) {
          Gtk::TreeModel::iterator iter = model->get_iter (*i);
          RefPtr<TrackListItem> track = (*iter)[columns.track];
          if (!track->colour_by_dir) { 
            (*iter)[columns.pix] = colour_by_dir_pixbuf;
            track->colour_by_dir = true;
            track->update_RGBA(); 
            update = true; 
          }
        }
        if (update) Window::Main->update (&parent);
      }






      void TrackList::on_randomise_colour () 
      {
        Math::RNG rng;
        std::list<Gtk::TreeModel::Path> paths = get_selection()->get_selected_rows();
        for (std::list<Gtk::TreeModel::Path>::iterator i = paths.begin(); i != paths.end(); ++i) {
          Gtk::TreeModel::iterator iter = model->get_iter (*i);
          RefPtr<TrackListItem> track = (*iter)[columns.track];
          track->colour_by_dir = false; 
          do {
            track->colour[0] = (int) (256.0*rng.uniform());
            track->colour[1] = (int) (256.0*rng.uniform());
            track->colour[2] = (int) (256.0*rng.uniform());
          } while (track->colour[0] < 128 && track->colour[1] < 128 && track->colour[2] < 128);
          Glib::RefPtr<Gdk::Pixbuf> pix = Gdk::Pixbuf::create (Gdk::COLORSPACE_RGB, false, 8, 16, 16);
          pix->fill (track->colour[0] << 24 | track->colour[1] << 16 | track->colour[2] << 8 | 255);
          (*iter)[columns.pix] = pix;
          track->update_RGBA (true); 
        }
        Window::Main->update (&parent);
      }





      void TrackList::on_set_colour () 
      {
        Gtk::ColorSelectionDialog dialog ("Choose colour for ROI");
        if (dialog.run() == Gtk::RESPONSE_OK) {
          Gdk::Color colour (dialog.get_colorsel()->get_current_color());
          GLubyte C[] = { colour.get_red() >> 8, colour.get_green() >> 8, colour.get_blue() >> 8 };
          std::list<Gtk::TreeModel::Path> paths = get_selection()->get_selected_rows();
          for (std::list<Gtk::TreeModel::Path>::iterator i = paths.begin(); i != paths.end(); ++i) {
            Gtk::TreeModel::iterator iter = model->get_iter (*i);
            RefPtr<TrackListItem> track = (*iter)[columns.track];
            track->colour_by_dir = false; 
            track->colour[0] = C[0];
            track->colour[1] = C[1];
            track->colour[2] = C[2];
            Glib::RefPtr<Gdk::Pixbuf> pix = Gdk::Pixbuf::create (Gdk::COLORSPACE_RGB, false, 8, 16, 16);
            pix->fill (track->colour[0] << 24 | track->colour[1] << 16 | track->colour[2] << 8 | 255);
            (*iter)[columns.pix] = pix;
            track->update_RGBA(); 
          }
          Window::Main->update (&parent);
        }
      }




      void TrackList::on_tick (const std::string& path) 
      {
        vertices.clear();
        Window::Main->update (&parent);
      }
      


      bool TrackList::on_refresh ()
      {
        if (!parent.show()) return (true);
        Gtk::TreeModel::Children tracks = model->children();
        if (tracks.size() == 0) return (true);
        for (Gtk::TreeModel::Children::iterator iter = tracks.begin(); iter != tracks.end(); ++iter) {
          bool show = (*iter)[columns.show];
          if (show) {
            try { 
              RefPtr<TrackListItem> tck = (*iter)[columns.track];
              if (tck->refresh()) {
                vertices.clear();
                g_signal_emit_by_name (get_selection()->gobj(), "changed"); 
                Window::Main->update (&parent);
              }
            }
            catch (...) { }
          }
        }
        return (true);
      }






      const uint8_t TrackList::colour_by_dir_data[16*16*4] = 
      {
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0,
        105, 226, 51, 96,
        76, 231, 71, 191,
        44, 235, 83, 255,
        12, 235, 95, 255,
        20, 235, 93, 255,
        51, 234, 82, 239,
        83, 230, 66, 175,
        110, 225, 45, 64,
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0,
        163, 191, 38, 32,
        137, 201, 71, 207,
        108, 203, 108, 255,
        76, 203, 133, 255,
        44, 203, 147, 255,
        12, 203, 153, 255,
        20, 203, 152, 255,
        52, 203, 144, 255,
        84, 203, 128, 255,
        116, 203, 99, 255,
        143, 199, 64, 159,
        167, 191, 21, 16,
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0,
        191, 163, 38, 32,
        171, 171, 77, 239,
        139, 171, 126, 255,
        108, 171, 154, 255,
        76, 171, 172, 255,
        44, 171, 183, 255,
        12, 171, 188, 255,
        20, 171, 187, 255,
        52, 171, 181, 255,
        84, 171, 169, 255,
        116, 171, 148, 255,
        147, 171, 116, 255,
        177, 169, 65, 207,
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0,
        201, 137, 71, 207,
        171, 139, 126, 255,
        139, 139, 161, 255,
        108, 139, 184, 255,
        76, 139, 199, 255,
        44, 139, 208, 255,
        12, 139, 213, 255,
        20, 139, 212, 255,
        52, 139, 207, 255,
        84, 139, 196, 255,
        116, 139, 179, 255,
        147, 139, 153, 255,
        179, 139, 114, 255,
        205, 136, 60, 143,
        0, 0, 0, 0,
        226, 105, 51, 96,
        203, 108, 108, 255,
        171, 108, 154, 255,
        139, 108, 184, 255,
        108, 108, 204, 255,
        76, 108, 218, 255,
        44, 108, 227, 255,
        12, 108, 231, 255,
        20, 108, 230, 255,
        52, 108, 225, 255,
        84, 108, 215, 255,
        116, 108, 200, 255,
        147, 108, 177, 255,
        179, 108, 145, 255,
        211, 108, 90, 255,
        231, 100, 40, 32,
        231, 76, 71, 191,
        203, 76, 133, 255,
        171, 76, 172, 255,
        139, 76, 199, 255,
        108, 76, 218, 255,
        76, 76, 231, 255,
        44, 76, 239, 255,
        12, 76, 243, 255,
        20, 76, 242, 255,
        52, 76, 238, 255,
        84, 76, 228, 255,
        116, 76, 214, 255,
        147, 76, 193, 255,
        179, 76, 164, 255,
        211, 76, 119, 255,
        235, 76, 59, 128,
        235, 44, 83, 255,
        203, 44, 147, 255,
        171, 44, 183, 255,
        139, 44, 208, 255,
        108, 44, 227, 255,
        76, 44, 239, 255,
        44, 44, 247, 255,
        12, 44, 251, 255,
        20, 44, 250, 255,
        52, 44, 245, 255,
        84, 44, 236, 255,
        116, 44, 223, 255,
        147, 44, 203, 255,
        179, 44, 175, 255,
        211, 44, 135, 255,
        239, 44, 73, 191,
        235, 12, 95, 255,
        203, 12, 153, 255,
        171, 12, 188, 255,
        139, 12, 213, 255,
        108, 12, 231, 255,
        76, 12, 243, 255,
        44, 12, 251, 255,
        12, 12, 254, 255,
        20, 12, 254, 255,
        52, 12, 249, 255,
        84, 12, 240, 255,
        116, 12, 227, 255,
        147, 12, 207, 255,
        179, 12, 180, 255,
        211, 12, 141, 255,
        240, 11, 79, 207,
        235, 20, 93, 255,
        203, 20, 152, 255,
        171, 20, 187, 255,
        139, 20, 212, 255,
        108, 20, 230, 255,
        76, 20, 242, 255,
        44, 20, 250, 255,
        12, 20, 254, 255,
        20, 20, 253, 255,
        52, 20, 249, 255,
        84, 20, 240, 255,
        116, 20, 226, 255,
        147, 20, 207, 255,
        179, 20, 180, 255,
        211, 20, 140, 255,
        239, 20, 84, 191,
        234, 51, 82, 239,
        203, 52, 144, 255,
        171, 52, 181, 255,
        139, 52, 207, 255,
        108, 52, 225, 255,
        76, 52, 238, 255,
        44, 52, 245, 255,
        12, 52, 249, 255,
        20, 52, 249, 255,
        52, 52, 244, 255,
        84, 52, 235, 255,
        116, 52, 221, 255,
        147, 52, 201, 255,
        179, 52, 173, 255,
        211, 52, 132, 255,
        238, 51, 71, 175,
        230, 83, 66, 175,
        203, 84, 128, 255,
        171, 84, 169, 255,
        139, 84, 196, 255,
        108, 84, 215, 255,
        76, 84, 228, 255,
        44, 84, 236, 255,
        12, 84, 240, 255,
        20, 84, 240, 255,
        52, 84, 235, 255,
        84, 84, 225, 255,
        116, 84, 211, 255,
        147, 84, 190, 255,
        179, 84, 160, 255,
        211, 84, 114, 255,
        235, 82, 53, 112,
        225, 110, 45, 64,
        203, 116, 99, 255,
        171, 116, 148, 255,
        139, 116, 179, 255,
        108, 116, 200, 255,
        76, 116, 214, 255,
        44, 116, 223, 255,
        12, 116, 227, 255,
        20, 116, 226, 255,
        52, 116, 221, 255,
        84, 116, 211, 255,
        116, 116, 195, 255,
        147, 116, 172, 255,
        179, 116, 138, 255,
        210, 115, 83, 239,
        231, 104, 30, 16,
        0, 0, 0, 0,
        199, 143, 64, 159,
        171, 147, 116, 255,
        139, 147, 153, 255,
        108, 147, 177, 255,
        76, 147, 193, 255,
        44, 147, 203, 255,
        12, 147, 207, 255,
        20, 147, 207, 255,
        52, 147, 201, 255,
        84, 147, 190, 255,
        116, 147, 172, 255,
        147, 147, 146, 255,
        179, 147, 103, 255,
        205, 141, 54, 96,
        0, 0, 0, 0,
        0, 0, 0, 0,
        191, 167, 21, 16,
        169, 177, 65, 207,
        139, 179, 114, 255,
        108, 179, 145, 255,
        76, 179, 164, 255,
        44, 179, 175, 255,
        12, 179, 180, 255,
        20, 179, 180, 255,
        52, 179, 173, 255,
        84, 179, 160, 255,
        116, 179, 138, 255,
        147, 179, 103, 255,
        175, 175, 53, 159,
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0,
        136, 205, 60, 143,
        108, 211, 90, 255,
        76, 211, 119, 255,
        44, 211, 135, 255,
        12, 211, 141, 255,
        20, 211, 140, 255,
        52, 211, 132, 255,
        84, 211, 114, 255,
        115, 210, 83, 239,
        141, 205, 54, 96,
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0,
        100, 231, 40, 32,
        76, 235, 59, 128,
        44, 239, 73, 191,
        11, 240, 79, 207,
        20, 239, 84, 191,
        51, 238, 71, 175,
        82, 235, 53, 112,
        104, 231, 30, 16,
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0
      };

    }
  }
}


