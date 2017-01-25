/* Copyright (c) 2008-2017 the MRtrix3 contributors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
 */


#ifndef __gui_mrtrix_tools_tractography_scalar_file_options_h__
#define __gui_mrtrix_tools_tractography_scalar_file_options_h__

#include "gui/mrview/adjust_button.h"
#include "gui/mrview/displayable.h"
#include "gui/mrview/tool/base.h"


namespace MR
{
  namespace GUI
  {
    namespace MRView
    {

      namespace Tool
      {

        class Tractogram;

          class TrackScalarFileOptions : public QGroupBox, public DisplayableVisitor
          {
            Q_OBJECT

            public:
              TrackScalarFileOptions (QWidget*);
              virtual ~TrackScalarFileOptions () {}

              void set_tractogram (Tractogram* selected_tractogram);

              void render_tractogram_colourbar (const Tool::Tractogram&) override;

              void update_UI();

            public slots:
              bool open_intensity_track_scalar_file_slot ();

            private slots:
              void show_colour_bar_slot();
              void select_colourmap_slot ();
              void on_set_scaling_slot ();
              bool threshold_scalar_file_slot (int);
              void threshold_lower_changed (int unused);
              void threshold_upper_changed (int unused);
              void threshold_lower_value_changed ();
              void threshold_upper_value_changed ();
              void invert_colourmap_slot ();
              void reset_intensity_slot ();

            protected:
              Tractogram *tractogram;
              Tool::Base::VBoxLayout *main_box;
              QGroupBox *colour_groupbox;
              QAction *show_colour_bar;
              QAction *invert_scale;
              QMenu *colourmap_menu;
              QAction **colourmap_actions;
              QActionGroup *colourmap_group;
              QToolButton *colourmap_button;
              QPushButton *intensity_file_button;
              AdjustButton *max_entry, *min_entry;
              QComboBox *threshold_file_combobox;
              AdjustButton *threshold_lower, *threshold_upper;
              QCheckBox *threshold_upper_box, *threshold_lower_box;

            private:
              // Required since this no longer derives from Tool::Base
              Window& window () const { return *Window::main; }

          };

      }
    }
  }
}

#endif

