/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by Robert E. Smith, 2015.

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

#ifndef __gui_mrview_tool_connectome_selection_h__
#define __gui_mrview_tool_connectome_selection_h__

#include "mrtrix.h"
#include "point.h"

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

          bool  get_node_selected_visibility_override()  const { return node_selected_visibility_override; }
          float get_node_selected_colour_fade()          const { return node_selected_colour_fade; }
          const Point<float>& get_node_selected_colour() const { return node_selected_colour; }
          float get_node_selected_size_multiplier()      const { return node_selected_size_multiplier; }
          float get_node_selected_alpha_multiplier()     const { return node_selected_alpha_multiplier; }

          bool  get_edge_selected_visibility_override()  const { return edge_selected_visibility_override; }
          float get_edge_selected_colour_fade()          const { return edge_selected_colour_fade; }
          const Point<float>& get_edge_selected_colour() const { return edge_selected_colour; }
          float get_edge_selected_size_multiplier()      const { return edge_selected_size_multiplier; }
          float get_edge_selected_alpha_multiplier()     const { return edge_selected_alpha_multiplier; }

          bool  get_node_not_selected_visibility_override()  const { return node_not_selected_visibility_override; }
          float get_node_not_selected_colour_fade()          const { return node_not_selected_colour_fade; }
          const Point<float>& get_node_not_selected_colour() const { return node_not_selected_colour; }
          float get_node_not_selected_size_multiplier()      const { return node_not_selected_size_multiplier; }
          float get_node_not_selected_alpha_multiplier()     const { return node_not_selected_alpha_multiplier; }

          bool  get_edge_not_selected_visibility_override()  const { return edge_not_selected_visibility_override; }
          float get_edge_not_selected_colour_fade()          const { return edge_not_selected_colour_fade; }
          const Point<float>& get_edge_not_selected_colour() const { return edge_not_selected_colour; }
          float get_edge_not_selected_size_multiplier()      const { return edge_not_selected_size_multiplier; }
          float get_edge_not_selected_alpha_multiplier()     const { return edge_not_selected_alpha_multiplier; }


        signals:
          void dataChanged();


        private:
          bool node_selected_visibility_override;
          float node_selected_colour_fade;
          Point<float> node_selected_colour;
          float node_selected_size_multiplier;
          float node_selected_alpha_multiplier;

          bool edge_selected_visibility_override;
          float edge_selected_colour_fade;
          Point<float> edge_selected_colour;
          float edge_selected_size_multiplier;
          float edge_selected_alpha_multiplier;

          bool node_not_selected_visibility_override;
          float node_not_selected_colour_fade;
          Point<float> node_not_selected_colour;
          float node_not_selected_size_multiplier;
          float node_not_selected_alpha_multiplier;

          bool edge_not_selected_visibility_override;
          float edge_not_selected_colour_fade;
          Point<float> edge_not_selected_colour;
          float edge_not_selected_size_multiplier;
          float edge_not_selected_alpha_multiplier;

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

          void node_not_selected_visibility_slot();
          void node_not_selected_colour_fade_slot();
          void node_not_selected_colour_slot();
          void node_not_selected_size_slot();
          void node_not_selected_alpha_slot();

          void edge_not_selected_visibility_slot();
          void edge_not_selected_colour_fade_slot();
          void edge_not_selected_colour_slot();
          void edge_not_selected_size_slot();
          void edge_not_selected_alpha_slot();

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

          QCheckBox *node_not_selected_visibility_checkbox;
          QSlider *node_not_selected_colour_slider;
          QColorButton *node_not_selected_colour_button;
          AdjustButton *node_not_selected_size_button;
          AdjustButton *node_not_selected_alpha_button;

          QCheckBox *edge_not_selected_visibility_checkbox;
          QSlider *edge_not_selected_colour_slider;
          QColorButton *edge_not_selected_colour_button;
          AdjustButton *edge_not_selected_size_button;
          AdjustButton *edge_not_selected_alpha_button;

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

