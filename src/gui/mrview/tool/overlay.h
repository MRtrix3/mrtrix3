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


#ifndef __gui_mrview_tool_overlay_h__
#define __gui_mrview_tool_overlay_h__

#include "gui/mrview/mode/base.h"
#include "gui/mrview/tool/base.h"
#include "gui/mrview/adjust_button.h"
#include "gui/mrview/colourmap_button.h"
#include "gui/mrview/spin_box.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {

        class Overlay : public Base, public ColourMapButtonObserver, public DisplayableVisitor
        { MEMALIGN(Overlay)
            Q_OBJECT

          public:

            Overlay (Dock* parent);

            void draw (const Projection& projection, bool is_3D, int axis, int slice) override;
            void draw_colourbars () override;
            int draw_tool_labels (int position, int start_line_num, const Projection&transform) const override;

            void selected_colourmap(size_t index, const ColourMapButton&) override;
            void selected_custom_colour(const QColor& colour, const ColourMapButton&) override;
            void toggle_show_colour_bar(bool visible, const ColourMapButton&) override;
            void toggle_invert_colourmap(bool, const ColourMapButton&) override;
            void reset_colourmap(const ColourMapButton&) override;

            size_t visible_number_colourbars () override;
            void render_image_colourbar(const Image& image) override;

            static void add_commandline_options (MR::App::OptionList& options);
            virtual bool process_commandline_option (const MR::App::ParsedOption& opt) override;

          private slots:
            void image_open_slot ();
            void image_close_slot ();
            void hide_all_slot ();
            void toggle_shown_slot (const QModelIndex&, const QModelIndex&);
            void selection_changed_slot (const QItemSelection &, const QItemSelection &);
            void right_click_menu_slot (const QPoint&);
            void volume_changed (int);
            void update_slot (int unused);
            void values_changed ();
            void upper_threshold_changed (int unused);
            void lower_threshold_changed (int unused);
            void upper_threshold_value_changed ();
            void lower_threshold_value_changed ();
            void opacity_changed (int unused);
            void interpolate_changed ();

          protected:
             class Item;
             class Model;
             class InterpolateCheckBox : public QCheckBox
             { NOMEMALIGN
               public:
                 InterpolateCheckBox(const QString& text, QWidget *parent = nullptr)
                   : QCheckBox(text, parent) {}
               protected:
                 // We don't want a click to cycle to a partially checked state
                 // So explicitly specify the allowed clickable states
                 void nextCheckState () override { checkState() == Qt::Unchecked ?
                         setCheckState(Qt::Checked) : setCheckState(Qt::Unchecked);
                 }
             };

             QPushButton* hide_all_button;
             Model* image_list_model;
             QListView* image_list_view;
             QLabel* volume_label;
             SpinBox* volume_selecter;
             ColourMapButton* colourmap_button;
             AdjustButton *min_value, *max_value, *lower_threshold, *upper_threshold;
             QCheckBox *lower_threshold_check_box, *upper_threshold_check_box;
             InterpolateCheckBox* interpolate_check_box;
             QSlider *opacity_slider;

             void update_selection ();
             void updateGL() { 
               window().get_current_mode()->update_overlays = true;
               window().updateGL();
             }
             
             void add_images (vector<std::unique_ptr<MR::Header>>& list);
             void dropEvent (QDropEvent* event) override;
        };

      }
    }
  }
}

#endif




