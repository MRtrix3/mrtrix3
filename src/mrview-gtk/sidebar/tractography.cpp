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

#include <gtkmm/treerowreference.h>

#include "mrview/sidebar/tractography.h"
#include "mrview/window.h"

namespace MR {
  namespace Viewer {
    namespace SideBar {

      Tractography::Tractography () : 
        Base (10),
        show_tracks ("show tracks"),
        show_ROIs ("show ROIs"),
        crop_to_slice ("crop to slab"),
        depth_blend ("depth blend"),
        track_frame ("tracks"), 
        roi_frame ("ROIs"), 
        crop_to_slice_frame ("slab thickness (mm)"), 
        transparency_frame ("opacity"), 
        line_thickness_frame ("line thickness"),
        transparency (0.0, 1.001, 0.001),
        line_thickness (1.0, 10.0, 1.0),
        slice_thickness_adjustment (5.0, 0.1, 1000.0, 0.1, 5.0),
        slab_thickness (slice_thickness_adjustment, 0.1, 1),
        track_list (*this),
        roi_list (*this)
      { 
        show_tracks.set_active (true);
        show_ROIs.set_active (true);
        crop_to_slice.set_active (true);

        transparency.set_draw_value (false);
        transparency.set_value (1.0);
        transparency.set_update_policy (Gtk::UPDATE_DELAYED);
        transparency.set_sensitive (false);

        line_thickness.set_draw_value (false);
        line_thickness.set_update_policy (Gtk::UPDATE_DELAYED);

        track_scrolled_window.add (track_list);
        track_scrolled_window.set_policy (Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
        track_scrolled_window.set_shadow_type (Gtk::SHADOW_IN);
        track_scrolled_window.set_border_width (3);
        track_frame.add (track_scrolled_window);

        roi_scrolled_window.add (roi_list);
        roi_scrolled_window.set_policy (Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
        roi_scrolled_window.set_shadow_type (Gtk::SHADOW_IN);
        roi_scrolled_window.set_border_width (3);
        roi_frame.add (roi_scrolled_window);

        paned.pack1 (track_frame, true, false);
        paned.pack2 (roi_frame, true, false);

        crop_to_slice_vbox.pack_start (crop_to_slice, Gtk::PACK_SHRINK);
        crop_to_slice_vbox.pack_start (slab_thickness, Gtk::PACK_SHRINK);

        crop_to_slice_frame.add (crop_to_slice_vbox);
        transparency_frame.add (transparency);
        line_thickness_frame.add (line_thickness);

        pack_start (show_tracks, Gtk::PACK_SHRINK);
        pack_start (show_ROIs, Gtk::PACK_SHRINK);
        pack_start (paned);
        pack_start (crop_to_slice_frame, Gtk::PACK_SHRINK);
        pack_start (transparency_frame, Gtk::PACK_SHRINK);
        pack_start (line_thickness_frame, Gtk::PACK_SHRINK);
        pack_start (depth_blend, Gtk::PACK_SHRINK);
        show_all();

        Window::Main->pane().activate (this);

        slab_thickness.signal_value_changed().connect (sigc::mem_fun (*this, &Tractography::on_slab));
        transparency.signal_value_changed().connect (sigc::mem_fun (*this, &Tractography::on_transparency));
        line_thickness.signal_value_changed().connect (sigc::mem_fun (*this, &Tractography::on_change));
        show_tracks.signal_toggled().connect (sigc::mem_fun (*this, &Tractography::on_change));
        show_ROIs.signal_toggled().connect (sigc::mem_fun (*this, &Tractography::on_change));
        crop_to_slice.signal_toggled().connect (sigc::mem_fun (*this, &Tractography::on_slab));
        depth_blend.signal_toggled().connect (sigc::mem_fun (*this, &Tractography::on_change));
        track_list.get_selection()->signal_changed().connect (sigc::mem_fun (*this, &Tractography::on_track_selection));
      }




      Tractography::~Tractography () { }


      void Tractography::draw () 
      {
        if (show()) {
          track_list.draw (); 
          if (show_ROIs.get_active()) roi_list.draw (); 
        }
      }

      void Tractography::on_change () { Window::Main->update (this); }
      void Tractography::on_slab () { track_list.vertices.clear(); Window::Main->update (this); }

      void Tractography::on_transparency () 
      {
        float alpha = transparency.get_value();
        bool update = false;
        std::list<Gtk::TreeModel::Path> paths = track_list.get_selection()->get_selected_rows();
        for (std::list<Gtk::TreeModel::Path>::iterator i = paths.begin(); i != paths.end(); ++i) {
          Gtk::TreeModel::iterator iter = track_list.model->get_iter (*i);
          RefPtr<TrackListItem> track = (*iter)[track_list.columns.track];
          if (track->alpha != alpha) { track->alpha = alpha; update = true; }
        }
        if (update) Window::Main->update (this);
      }


      void Tractography::on_track_selection () 
      {
        roi_list.model->clear();
        std::list<Gtk::TreeModel::Path> paths = track_list.get_selection()->get_selected_rows();


        if (paths.size()) {
          transparency.set_sensitive (true);
          Gtk::TreeModel::iterator iter = track_list.model->get_iter (paths.front());
          RefPtr<TrackListItem> track = (*iter)[track_list.columns.track];
          if (paths.size() == 1) roi_list.set (track->properties.roi);
          transparency.set_value (track->alpha);
        }
        else transparency.set_sensitive (false);

        if (show_ROIs.get_active()) Window::Main->update (this); 
      }

    }
  }
}

