/*
    Copyright 2013 Brain Research Institute, Melbourne, Australia

    Written by David Raffelt, 01/02/13.

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

#ifndef __gui_mrtrix_tools_tractography_scalar_file_options_h__
#define __gui_mrtrix_tools_tractography_scalar_file_options_h__

#include "gui/mrview/tool/base.h"
#include "gui/mrview/tool/tractography/tractogram.h"
#include "gui/dialog/file.h"


namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {

          class ScalarFileOptions : public Base
          {
            Q_OBJECT

            public:
              ScalarFileOptions (Window& main_window, Dock* parent);

              void set_tractogram (Tractogram* selected_tractogram);

            public slots:
              bool open_track_scalar_file_slot ();

            private slots:
              void show_colour_bar_slot();
              void select_colourmap_slot ();
              void toggle_threshold_slot ();
              void on_set_scaling_slot ();
              void on_set_threshold_slot ();
              void invert_colourmap_slot ();
              void scalarfile_by_direction_slot ();

            protected:
              Tractogram *tractogram;
              QVBoxLayout *main_box;
              QAction *show_colour_bar;
              QAction *invert_colourmap_action;
              QAction *scalarfile_by_direction;
              QMenu *colourmap_menu;
              QAction **colourmap_actions;
              QActionGroup *colourmap_group;
              QToolButton *colourmap_button;
              QPushButton *button;
              AdjustButton *max_entry, *min_entry;
              AdjustButton *lessthan, *greaterthan;
              QGroupBox *threshold_box;

          };

      }
    }
  }
}

#endif

