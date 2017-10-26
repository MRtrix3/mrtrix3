/* Copyright (c) 2008-2017 the MRtrix3 contributors.
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


#ifndef __gui_mrview_tool_tractography_h__
#define __gui_mrview_tool_tractography_h__

#include "gui/mrview/tool/base.h"
#include "gui/color_button.h"
#include "gui/projection.h"
#include "gui/mrview/adjust_button.h"
#include "gui/mrview/combo_box_error.h"
#include "gui/mrview/tool/tractography/track_scalar_file.h"

namespace MR
{
  namespace GUI
  {
    namespace GL {
      class Lighting;
    }

    class LightingDock;

    namespace MRView
    {
      namespace Tool
      {

        extern const char* tractogram_geometry_types[];

        class Tractography : public Base
        { MEMALIGN(Tractography)
            Q_OBJECT

          public:

            class Model;

            Tractography (Dock* parent);

            virtual ~Tractography ();

            void draw (const Projection& transform, bool is_3D, int axis, int slice) override;
            void draw_colourbars () override;
            size_t visible_number_colourbars () override;
            bool crop_to_slab () const { return (do_crop_to_slab && not_3D); }

            static void add_commandline_options (MR::App::OptionList& options);
            virtual bool process_commandline_option (const MR::App::ParsedOption& opt) override;

            QPushButton* hide_all_button;
            bool do_crop_to_slab;
            bool use_lighting;
            bool use_threshold_scalarfile;
            bool not_3D;
            float slab_thickness;
            float line_opacity;
            Model* tractogram_list_model;
            QListView* tractogram_list_view;

            GL::Lighting* lighting;

          private slots:
            void tractogram_open_slot ();
            void tractogram_close_slot ();
            void toggle_shown_slot (const QModelIndex&, const QModelIndex&);
            void hide_all_slot ();
            void on_slab_thickness_slot ();
            void on_crop_to_slab_slot (bool is_checked);
            void on_use_lighting_slot (bool is_checked);
            void on_lighting_settings ();
            void opacity_slot (int opacity);
            void line_thickness_slot (int thickness);
            void right_click_menu_slot (const QPoint& pos);
            void colour_track_by_direction_slot ();
            void colour_track_by_ends_slot ();
            void randomise_track_colour_slot ();
            void set_track_colour_slot ();
            void colour_by_scalar_file_slot ();
            void colour_mode_selection_slot (int);
            void colour_button_slot();
            void geom_type_selection_slot (int);
            void selection_changed_slot (const QItemSelection &, const QItemSelection &);

          protected:
            AdjustButton* slab_entry;
            QMenu* track_option_menu;

            ComboBoxWithErrorMsg *colour_combobox;
            QColorButton *colour_button;

            ComboBoxWithErrorMsg *geom_type_combobox;

            QLabel* thickness_label;
            QSlider* thickness_slider;

            TrackScalarFileOptions *scalar_file_options;
            LightingDock *lighting_dock;

            QGroupBox* slab_group_box;
            QGroupBox* lighting_group_box;
            QPushButton* lighting_button;

            QSlider* opacity_slider;

            void dropEvent (QDropEvent* event) override;
            void update_scalar_options();
            void add_tractogram (vector<std::string>& list);
            void select_last_added_tractogram();
            bool process_commandline_option_tsf_check_tracto_loaded ();
            bool process_commandline_option_tsf_option (const MR::App::ParsedOption&, uint, vector<default_type>& range);
            void update_geometry_type_gui();
        };
      }
    }
  }
}

#endif




