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

          class TrackScalarFile : public Base, public DisplayableVisitor
          {
            Q_OBJECT

            public:
              TrackScalarFile (Dock* parent);
              virtual ~TrackScalarFile () {}

              void set_tractogram (Tractogram* selected_tractogram);

              void render_tractogram_colourbar(const Tool::Tractogram&) override;

            public slots:
              bool open_track_scalar_file_slot ();

            private slots:
              void show_colour_bar_slot();
              void select_colourmap_slot ();
              void on_set_scaling_slot ();
              void threshold_lower_changed (int unused);
              void threshold_upper_changed (int unused);
              void threshold_lower_value_changed ();
              void threshold_upper_value_changed ();
              void invert_colourmap_slot ();
              void scalarfile_by_direction_slot ();
              void reset_intensity_slot ();

            protected:
              void clear_tool_display ();
              void update_tool_display ();

              Tractogram *tractogram;
              VBoxLayout *main_box;
              QAction *show_colour_bar;
              QAction *invert_scale;
              QAction *scalarfile_by_direction;
              QMenu *colourmap_menu;
              QAction **colourmap_actions;
              QActionGroup *colourmap_group;
              QToolButton *colourmap_button;
              QPushButton *file_button;
              AdjustButton *max_entry, *min_entry;
              AdjustButton *threshold_lower, *threshold_upper;
              QCheckBox *threshold_upper_box, *threshold_lower_box;

          };

      }
    }
  }
}

#endif

