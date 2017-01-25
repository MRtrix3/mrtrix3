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


#ifndef __gui_mrview_tool_connectome_selection_h__
#define __gui_mrview_tool_connectome_selection_h__

#include "mrtrix.h"

#include "gui/color_button.h"
#include "gui/mrview/adjust_button.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {



      class NodeSelectionSettings : public QObject
      {

          Q_OBJECT

        public:
          NodeSelectionSettings();

          bool  get_node_selected_visibility_override()    const { return node_selected_visibility_override; }
          float get_node_selected_colour_fade()            const { return node_selected_colour_fade; }
          const Eigen::Array3f& get_node_selected_colour() const { return node_selected_colour; }
          float get_node_selected_size_multiplier()        const { return node_selected_size_multiplier; }
          float get_node_selected_alpha_multiplier()       const { return node_selected_alpha_multiplier; }

          bool  get_edge_selected_visibility_override()    const { return edge_selected_visibility_override; }
          float get_edge_selected_colour_fade()            const { return edge_selected_colour_fade; }
          const Eigen::Array3f& get_edge_selected_colour() const { return edge_selected_colour; }
          float get_edge_selected_size_multiplier()        const { return edge_selected_size_multiplier; }
          float get_edge_selected_alpha_multiplier()       const { return edge_selected_alpha_multiplier; }

          float get_node_associated_colour_fade()            const { return node_associated_colour_fade; }
          const Eigen::Array3f& get_node_associated_colour() const { return node_associated_colour; }
          float get_node_associated_size_multiplier()        const { return node_associated_size_multiplier; }
          float get_node_associated_alpha_multiplier()       const { return node_associated_alpha_multiplier; }

          float get_edge_associated_colour_fade()            const { return edge_associated_colour_fade; }
          const Eigen::Array3f& get_edge_associated_colour() const { return edge_associated_colour; }
          float get_edge_associated_size_multiplier()        const { return edge_associated_size_multiplier; }
          float get_edge_associated_alpha_multiplier()       const { return edge_associated_alpha_multiplier; }

          bool  get_node_other_visibility_override()    const { return node_other_visibility_override; }
          float get_node_other_colour_fade()            const { return node_other_colour_fade; }
          const Eigen::Array3f& get_node_other_colour() const { return node_other_colour; }
          float get_node_other_size_multiplier()        const { return node_other_size_multiplier; }
          float get_node_other_alpha_multiplier()       const { return node_other_alpha_multiplier; }

          bool  get_edge_other_visibility_override()    const { return edge_other_visibility_override; }
          float get_edge_other_colour_fade()            const { return edge_other_colour_fade; }
          const Eigen::Array3f& get_edge_other_colour() const { return edge_other_colour; }
          float get_edge_other_size_multiplier()        const { return edge_other_size_multiplier; }
          float get_edge_other_alpha_multiplier()       const { return edge_other_alpha_multiplier; }


        signals:
          void dataChanged();


        private:
          bool node_selected_visibility_override;
          float node_selected_colour_fade;
          Eigen::Array3f node_selected_colour;
          float node_selected_size_multiplier;
          float node_selected_alpha_multiplier;

          bool edge_selected_visibility_override;
          float edge_selected_colour_fade;
          Eigen::Array3f edge_selected_colour;
          float edge_selected_size_multiplier;
          float edge_selected_alpha_multiplier;

          float node_associated_colour_fade;
          Eigen::Array3f node_associated_colour;
          float node_associated_size_multiplier;
          float node_associated_alpha_multiplier;

          float edge_associated_colour_fade;
          Eigen::Array3f edge_associated_colour;
          float edge_associated_size_multiplier;
          float edge_associated_alpha_multiplier;

          bool node_other_visibility_override;
          float node_other_colour_fade;
          Eigen::Array3f node_other_colour;
          float node_other_size_multiplier;
          float node_other_alpha_multiplier;

          bool edge_other_visibility_override;
          float edge_other_colour_fade;
          Eigen::Array3f edge_other_colour;
          float edge_other_size_multiplier;
          float edge_other_alpha_multiplier;

          friend class NodeSelectionSettingsFrame;

      };



      class NodeSelectionSettingsFrame : public QFrame
      {
          Q_OBJECT

        public:

          NodeSelectionSettingsFrame (QWidget*, NodeSelectionSettings&);
          ~NodeSelectionSettingsFrame () { }

        protected:
          NodeSelectionSettings& data;

        protected slots:
          void node_selected_visibility_slot();
          void node_selected_colour_fade_slot();
          void node_selected_colour_slot();
          void node_selected_size_slot();
          void node_selected_alpha_slot();

          void edge_selected_visibility_slot();
          void edge_selected_colour_fade_slot();
          void edge_selected_colour_slot();
          void edge_selected_size_slot();
          void edge_selected_alpha_slot();

          void node_associated_colour_fade_slot();
          void node_associated_colour_slot();
          void node_associated_size_slot();
          void node_associated_alpha_slot();

          void edge_associated_colour_fade_slot();
          void edge_associated_colour_slot();
          void edge_associated_size_slot();
          void edge_associated_alpha_slot();

          void node_other_visibility_slot();
          void node_other_colour_fade_slot();
          void node_other_colour_slot();
          void node_other_size_slot();
          void node_other_alpha_slot();

          void edge_other_visibility_slot();
          void edge_other_colour_fade_slot();
          void edge_other_colour_slot();
          void edge_other_size_slot();
          void edge_other_alpha_slot();

        private:
          QCheckBox *node_selected_visibility_checkbox;
          QSlider *node_selected_colour_slider;
          QColorButton *node_selected_colour_button;
          AdjustButton *node_selected_size_button;
          AdjustButton *node_selected_alpha_button;

          QCheckBox *edge_selected_visibility_checkbox;
          QSlider *edge_selected_colour_slider;
          QColorButton *edge_selected_colour_button;
          AdjustButton *edge_selected_size_button;
          AdjustButton *edge_selected_alpha_button;

          QSlider *node_associated_colour_slider;
          QColorButton *node_associated_colour_button;
          AdjustButton *node_associated_size_button;
          AdjustButton *node_associated_alpha_button;

          QSlider *edge_associated_colour_slider;
          QColorButton *edge_associated_colour_button;
          AdjustButton *edge_associated_size_button;
          AdjustButton *edge_associated_alpha_button;

          QCheckBox *node_other_visibility_checkbox;
          QSlider *node_other_colour_slider;
          QColorButton *node_other_colour_button;
          AdjustButton *node_other_size_button;
          AdjustButton *node_other_alpha_button;

          QCheckBox *edge_other_visibility_checkbox;
          QSlider *edge_other_colour_slider;
          QColorButton *edge_other_colour_button;
          AdjustButton *edge_other_size_button;
          AdjustButton *edge_other_alpha_button;

      };






      class NodeSelectionSettingsDialog : public QDialog
      {
        public:
          NodeSelectionSettingsDialog (QWidget* parent, const std::string& message, NodeSelectionSettings& settings);

          NodeSelectionSettingsFrame* frame;
      };




      }
    }
  }
}

#endif

