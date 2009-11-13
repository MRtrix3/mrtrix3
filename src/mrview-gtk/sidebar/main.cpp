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


    08-07-2008 J-Donald Tournier <d.tournier@brain.org.au>
    * rename "diffusion profile" to "orientation plot"

*/

#include "mrview/sidebar/main.h"
#include "mrview/sidebar/base.h"
#include "mrview/sidebar/tractography.h"
#include "mrview/sidebar/roi_analysis.h"
#include "mrview/sidebar/orientation_plot.h"
#include "mrview/sidebar/screen_capture.h"

namespace MR {
  namespace Viewer {
    namespace SideBar {

      const char* Main::names[] = { 
        "tractography",
        "ROI analysis",
        "orientation plot",
        "screen capture"
      }; 




      Main::Main () 
      {
        set_size_request (128, -1);
        set_shadow_type (Gtk::SHADOW_IN);

        selector_list = Gtk::ListStore::create (entry);
        selector.set_model (selector_list);
        selector.pack_start (entry.name);

        Gtk::TreeModel::Row row = *(selector_list->append());
        row[entry.ID] = 0; 
        row[entry.name] = names[0];
        list[0] = NULL;

        row = *(selector_list->append());
        row[entry.ID] = 1;
        row[entry.name] = names[1];
        list[1] = NULL;

        row = *(selector_list->append());
        row[entry.ID] = 2;
        row[entry.name] = names[2];
        list[2] = NULL;

        row = *(selector_list->append());
        row[entry.ID] = 3;
        row[entry.name] = names[3];
        list[3] = NULL;

        box.pack_start (selector, Gtk::PACK_SHRINK);
        selector.signal_changed().connect (sigc::mem_fun (*this, &Main::on_selector));

        add (box);
        show_all();
      }







      void Main::on_selector ()
      {
        Gtk::TreeModel::iterator iter = selector.get_active();
        if (iter) {
          Gtk::TreeModel::Row row = *iter;
          if (row) {   
            int id = row[entry.ID];
            if (!list[id]) init (id);

            for (uint n = 1; n < box.children().size(); n++)
              box.children()[n].get_widget()->hide();

            list[id]->show();
          }   
        }

      }






      void Main::init (uint index)
      {
        assert (index < NUM_SIDEBAR);
        assert (list[index] == NULL);
        switch (index) {
          case 0:  list[0] = manage (new Tractography); break;
          case 1:  list[1] = manage (new ROIAnalysis); break;
          case 2:  list[2] = manage (new OrientationPlot); break;
          case 3:  list[3] = manage (new ScreenCapture); break;
          default: list[index] = manage (new Base (-1)); break;
        }
        box.pack_start (*list[index]);
      }

    }
  }
}


