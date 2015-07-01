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

#include "gui/mrview/tool/connectome/selection.h"

#include "gui/mrview/tool/base.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {


      NodeSelectionSettings::NodeSelectionSettings() :
          node_selected_visibility_override (true),
          node_selected_colour_fade (0.5f),
          node_selected_colour (1.0f, 0.9f, 0.9f),
          node_selected_size_multiplier (1.0f),
          node_selected_alpha_multiplier (1.0f),
          edge_selected_visibility_override (false),
          edge_selected_colour_fade (0.5f),
          edge_selected_colour (0.9f, 0.9f, 1.0f),
          edge_selected_size_multiplier (1.0f),
          edge_selected_alpha_multiplier (1.0f),
          node_not_selected_visibility_override (false),
          node_not_selected_colour_fade (0.75f),
          node_not_selected_colour (0.0f, 0.0f, 0.0f),
          node_not_selected_size_multiplier (1.0f),
          node_not_selected_alpha_multiplier (1.0f),
          edge_not_selected_visibility_override (false),
          edge_not_selected_colour_fade (0.75f),
          edge_not_selected_colour (0.0f, 0.0f, 0.0f),
          edge_not_selected_size_multiplier (1.0f),
          edge_not_selected_alpha_multiplier (1.0f)
      {
        // Load default settings from the config file

        //CONF option: ConnectomeNodeSelectedVisibilityOverride
        //CONF default: true
        //CONF Whether or not nodes are forced to be visible when selected
        node_selected_visibility_override = File::Config::get_bool ("ConnectomeNodeSelectedVisibilityOverride", true);
        //CONF option: ConnectomeNodeSelectedColourFade
        //CONF default: 0.5
        //CONF The fraction of the colour of a selected node determined by the fixed selection highlight colour
        node_selected_colour_fade = File::Config::get_float ("ConnectomeNodeSelectedColourFade", 0.5f);
        //CONF option: ConnectomeNodeSelectedColour
        //CONF default: 1.0,0.9,0.9
        //CONF The colour used to highlight those nodes currently selected
        File::Config::get_RGB ("ConnectomeNodeSelectedColour", node_selected_colour, 1.0f, 0.9f, 0.9f);
        //CONF option: ConnectomeNodeSelectedSizeMultiplier
        //CONF default: 1.0
        //CONF The multiplicative factor to apply to the size of selected nodes
        node_selected_size_multiplier = File::Config::get_float ("ConnectomeNodeSelectedSizeMultiplier", 1.0f);
        //CONF option: ConnectomeNodeSelectedAlphaMultiplier
        //CONF default: 1.0
        //CONF The multiplicative factor to apply to the transparency of selected nodes
        node_selected_alpha_multiplier = File::Config::get_float ("ConnectomeNodeSelectedAlphaMultiplier", 1.0f);

        //CONF option: ConnectomeEdgeSelectedVisibilityOverride
        //CONF default: false
        //CONF Whether or not to force visibility of edges connected to selected nodes
        edge_selected_visibility_override = File::Config::get_bool ("ConnectomeEdgeSelectedVisibilityOverride", false);
        //CONF option: ConnectomeEdgeSelectedColourFade
        //CONF default: 0.5
        //CONF The fraction of the colour of an edge associated with selected nodes determined by the fixed selection highlight colour
        edge_selected_colour_fade = File::Config::get_float ("ConnectomeEdgeSelectedColourFade", 0.5f);
        //CONF option: ConnectomeEdgeSelectedColour
        //CONF default: 0.9,0.9,1.0
        //CONF The colour used to highlight the edges associated with currently selected nodes
        File::Config::get_RGB ("ConnectomeEdgeSelectedColour", edge_selected_colour, 0.9f, 0.9f, 1.0f);
        //CONF option: ConnectomeEdgeSelectedSizeMultiplier
        //CONF default: 1.0
        //CONF The multiplicative factor to apply to the size of edges associated with selected nodes
        edge_selected_size_multiplier = File::Config::get_float ("ConnectomeEdgeSelectedSizeMultiplier", 1.0f);
        //CONF option: ConnectomeEdgeSelectedAlphaMultiplier
        //CONF default: 1.0
        //CONF The multiplicative factor to apply to the transparency of edges associated with selected nodes
        edge_selected_alpha_multiplier = File::Config::get_float ("ConnectomeEdgeSelectedAlphaMultiplier", 1.0f);

        //CONF option: ConnectomeNodeNotSelectedVisibilityOverride
        //CONF default: true
        //CONF Whether or not nodes are forced to be invisible when not selected
        node_not selected_visibility_override = File::Config::get_bool ("ConnectomeNodeNotSelectedVisibilityOverride", true);
        //CONF option: ConnectomeNodeNotSelectedColourFade
        //CONF default: 0.75
        //CONF The fraction of the colour of an unselected node determined by the fixed not-selected highlight colour
        node_not_selected_colour_fade = File::Config::get_float ("ConnectomeNodeSelectedColourFade", 0.75f);
        //CONF option: ConnectomeNodeNotSelectedColour
        //CONF default: 0.0,0.0,0.0
        //CONF The colour mixed in to those nodes currently not selected
        File::Config::get_RGB ("ConnectomeNodeNotSelectedColour", node_not_selected_colour, 0.0f, 0.0f, 0.0f);
        //CONF option: ConnectomeNodeNotSelectedSizeMultiplier
        //CONF default: 1.0
        //CONF The multiplicative factor to apply to the size of nodes not currently selected
        node_not_selected_size_multiplier = File::Config::get_float ("ConnectomeNodeNotSelectedSizeMultiplier", 1.0f);
        //CONF option: ConnectomeNodeNotSelectedAlphaMultiplier
        //CONF default: 1.0
        //CONF The multiplicative factor to apply to the transparency of nodes not currently selected
        node_not_selected_alpha_multiplier = File::Config::get_float ("ConnectomeNodeNotSelectedAlphaMultiplier", 1.0f);

        //CONF option: ConnectomeEdgeNotSelectedVisibilityOverride
        //CONF default: false
        //CONF Whether or not to force invisibility of edges not connected to selected nodes
        edge_not_selected_visibility_override = File::Config::get_bool ("ConnectomeEdgeNotSelectedVisibilityOverride", false);
        //CONF option: ConnectomeEdgeNotSelectedColourFade
        //CONF default: 0.75
        //CONF The fraction of the colour of an edge not associated with selected nodes determined by the fixed colour
        edge_not_selected_colour_fade = File::Config::get_float ("ConnectomeEdgeNotSelectedColourFade", 0.75f);
        //CONF option: ConnectomeEdgeNotSelectedColour
        //CONF default: 0.0,0.0,0.0
        //CONF The colour mixed in to edges not associated with currently selected nodes
        File::Config::get_RGB ("ConnectomeEdgeNotSelectedColour", edge_not_selected_colour, 0.0f, 0.0f, 0.0f);
        //CONF option: ConnectomeEdgeNotSelectedSizeMultiplier
        //CONF default: 1.0
        //CONF The multiplicative factor to apply to the size of edges not associated with selected nodes
        edge_not_selected_size_multiplier = File::Config::get_float ("ConnectomeEdgeNotSelectedSizeMultiplier", 1.0f);
        //CONF option: ConnectomeEdgeNotSelectedAlphaMultiplier
        //CONF default: 1.0
        //CONF The multiplicative factor to apply to the transparency of edges not associated with selected nodes
        edge_not_selected_alpha_multiplier = File::Config::get_float ("ConnectomeEdgeNotSelectedAlphaMultiplier", 1.0f);
      }




      NodeSelectionSettingsFrame::NodeSelectionSettingsFrame (QWidget* parent, NodeSelectionSettings& settings) :
          QFrame (parent),
          data (settings)
      {
        Base::GridLayout* main_box = new Base::GridLayout;
        setLayout (main_box);

        QGroupBox* group_box = new QGroupBox ("Selected nodes highlight");
        Base::GridLayout* grid_layout = new Base::GridLayout;
        group_box->setLayout (grid_layout);

        QLabel *label = new QLabel ("Visibility: ");
        grid_layout->addWidget (label, 0, 0);
        node_selected_visibility_checkbox = new QCheckBox();
        node_selected_visibility_checkbox->setTristate (false);
        node_selected_visibility_checkbox->setChecked (settings.get_node_selected_visibility_override());
        connect (node_selected_visibility_checkbox, SIGNAL (stateChanged(int)), this, SLOT (node_selected_visibility_slot()));
        grid_layout->addWidget (node_selected_visibility_checkbox, 0, 1);

        label = new QLabel ("Colour: ");
        grid_layout->addWidget (label, 1, 0);
        Base::HBoxLayout* hbox_layout = new Base::HBoxLayout;
        node_selected_colour_slider = new QSlider (Qt::Horizontal);
        node_selected_colour_slider->setRange (0, 100);
        node_selected_colour_slider->setSliderPosition (settings.get_node_selected_colour_fade() * 100.0f);
        connect (node_selected_colour_slider, SIGNAL (valueChanged (int)), this, SLOT (node_selected_colour_fade_slot()));
        hbox_layout->addWidget (node_selected_colour_slider);
        node_selected_colour_button = new QColorButton;
        Point<float> c = settings.get_node_selected_colour();
        c *= 255.0f;
        node_selected_colour_button->setColor (QColor (c[0], c[1], c[2]));
        connect (node_selected_colour_button, SIGNAL (clicked()), this, SLOT (node_selected_colour_slot()));
        hbox_layout->addWidget (node_selected_colour_button);
        grid_layout->addLayout (hbox_layout, 1, 1);

        label = new QLabel ("Size: ");
        grid_layout->addWidget (label, 2, 0);
        node_selected_size_button = new AdjustButton (this, 0.01);
        node_selected_size_button->setMin (0.0f);
        node_selected_size_button->setValue (settings.get_node_selected_size_multiplier());
        connect (node_selected_size_button, SIGNAL (valueChanged()), this, SLOT (node_selected_size_slot()));
        grid_layout->addWidget (node_selected_size_button, 2, 1);

        label = new QLabel ("Transparency: ");
        grid_layout->addWidget (label, 3, 0);
        node_selected_alpha_button = new AdjustButton (this, 0.01);
        node_selected_alpha_button->setMin (0.0f);
        node_selected_alpha_button->setValue (settings.get_node_selected_alpha_multiplier());
        connect (node_selected_alpha_button, SIGNAL (valueChanged()), this, SLOT (node_selected_alpha_slot()));
        grid_layout->addWidget (node_selected_alpha_button, 3, 1);

        main_box->addWidget (group_box, 0, 0);

        group_box = new QGroupBox ("Selected edges highlight");
        grid_layout = new Base::GridLayout;
        group_box->setLayout (grid_layout);

        label = new QLabel ("Visibility: ");
        grid_layout->addWidget (label, 0, 0);
        edge_selected_visibility_checkbox = new QCheckBox();
        edge_selected_visibility_checkbox->setTristate (false);
        edge_selected_visibility_checkbox->setChecked (settings.get_edge_selected_visibility_override());
        connect (edge_selected_visibility_checkbox, SIGNAL (stateChanged(int)), this, SLOT (edge_selected_visibility_slot()));
        grid_layout->addWidget (edge_selected_visibility_checkbox, 0, 1);

        label = new QLabel ("Colour: ");
        grid_layout->addWidget (label, 1, 0);
        hbox_layout = new Base::HBoxLayout;
        edge_selected_colour_slider = new QSlider (Qt::Horizontal);
        edge_selected_colour_slider->setRange (0, 100);
        edge_selected_colour_slider->setSliderPosition (settings.get_edge_selected_colour_fade() * 100.0f);
        connect (edge_selected_colour_slider, SIGNAL (valueChanged (int)), this, SLOT (edge_selected_colour_fade_slot()));
        hbox_layout->addWidget (edge_selected_colour_slider);
        edge_selected_colour_button = new QColorButton;
        c = settings.get_edge_selected_colour();
        c *= 255.0f;
        edge_selected_colour_button->setColor (QColor (c[0], c[1], c[2]));
        connect (edge_selected_colour_button, SIGNAL (clicked()), this, SLOT (edge_selected_colour_slot()));
        hbox_layout->addWidget (edge_selected_colour_button);
        grid_layout->addLayout (hbox_layout, 1, 1);

        label = new QLabel ("Size: ");
        grid_layout->addWidget (label, 2, 0);
        edge_selected_size_button = new AdjustButton (this, 0.01);
        edge_selected_size_button->setMin (0.0f);
        edge_selected_size_button->setValue (settings.get_edge_selected_size_multiplier());
        connect (edge_selected_size_button, SIGNAL (valueChanged()), this, SLOT (edge_selected_size_slot()));
        grid_layout->addWidget (edge_selected_size_button, 2, 1);

        label = new QLabel ("Transparency: ");
        grid_layout->addWidget (label, 3, 0);
        edge_selected_alpha_button = new AdjustButton (this, 0.01);
        edge_selected_alpha_button->setMin (0.0f);
        edge_selected_alpha_button->setValue (settings.get_edge_selected_alpha_multiplier());
        connect (edge_selected_alpha_button, SIGNAL (valueChanged()), this, SLOT (edge_selected_alpha_slot()));
        grid_layout->addWidget (edge_selected_alpha_button, 3, 1);

        main_box->addWidget (group_box, 0, 1);

        group_box = new QGroupBox ("Other nodes");
        grid_layout = new Base::GridLayout;
        group_box->setLayout (grid_layout);

        label = new QLabel ("Visibility: ");
        grid_layout->addWidget (label, 0, 0);
        node_not_selected_visibility_checkbox = new QCheckBox();
        node_not_selected_visibility_checkbox->setTristate (false);
        node_not_selected_visibility_checkbox->setChecked (settings.get_node_not_selected_visibility_override());
        connect (node_not_selected_visibility_checkbox, SIGNAL (stateChanged(int)), this, SLOT (node_not_selected_visibility_slot()));
        grid_layout->addWidget (node_not_selected_visibility_checkbox, 0, 1);

        label = new QLabel ("Colour: ");
        grid_layout->addWidget (label, 1, 0);
        hbox_layout = new Base::HBoxLayout;
        node_not_selected_colour_slider = new QSlider (Qt::Horizontal);
        node_not_selected_colour_slider->setRange (0, 100);
        node_not_selected_colour_slider->setSliderPosition (settings.get_node_not_selected_colour_fade() * 100.0f);
        connect (node_not_selected_colour_slider, SIGNAL (valueChanged (int)), this, SLOT (node_not_selected_colour_fade_slot()));
        hbox_layout->addWidget (node_not_selected_colour_slider);
        node_not_selected_colour_button = new QColorButton;
        c = settings.get_node_not_selected_colour();
        c *= 255.0f;
        node_not_selected_colour_button->setColor (QColor (c[0], c[1], c[2]));
        connect (node_not_selected_colour_button, SIGNAL (clicked()), this, SLOT (node_not_selected_colour_slot()));
        hbox_layout->addWidget (node_not_selected_colour_button);
        grid_layout->addLayout (hbox_layout, 1, 1);

        label = new QLabel ("Size: ");
        grid_layout->addWidget (label, 2, 0);
        node_not_selected_size_button = new AdjustButton (this, 0.01);
        node_not_selected_size_button->setMin (0.0f);
        node_not_selected_size_button->setValue (settings.get_node_not_selected_size_multiplier());
        connect (node_not_selected_size_button, SIGNAL (valueChanged()), this, SLOT (node_not_selected_size_slot()));
        grid_layout->addWidget (node_not_selected_size_button, 2, 1);

        label = new QLabel ("Transparency: ");
        grid_layout->addWidget (label, 3, 0);
        node_not_selected_alpha_button = new AdjustButton (this, 0.01);
        node_not_selected_alpha_button->setMin (0.0f);
        node_not_selected_alpha_button->setValue (settings.get_node_not_selected_alpha_multiplier());
        connect (node_not_selected_alpha_button, SIGNAL (valueChanged()), this, SLOT (node_not_selected_alpha_slot()));
        grid_layout->addWidget (node_not_selected_alpha_button, 3, 1);

        main_box->addWidget (group_box, 1, 0);

        group_box = new QGroupBox ("Other edges");
        grid_layout = new Base::GridLayout;
        group_box->setLayout (grid_layout);

        label = new QLabel ("Visibility: ");
        grid_layout->addWidget (label, 0, 0);
        edge_not_selected_visibility_checkbox = new QCheckBox();
        edge_not_selected_visibility_checkbox->setTristate (false);
        edge_not_selected_visibility_checkbox->setChecked (settings.get_edge_not_selected_visibility_override());
        connect (edge_not_selected_visibility_checkbox, SIGNAL (stateChanged(int)), this, SLOT (edge_not_selected_visibility_slot()));
        grid_layout->addWidget (edge_not_selected_visibility_checkbox, 0, 1);

        label = new QLabel ("Colour: ");
        grid_layout->addWidget (label, 1, 0);
        hbox_layout = new Base::HBoxLayout;
        edge_not_selected_colour_slider = new QSlider (Qt::Horizontal);
        edge_not_selected_colour_slider->setRange (0, 100);
        edge_not_selected_colour_slider->setSliderPosition (settings.get_edge_not_selected_colour_fade() * 100.0f);
        connect (edge_not_selected_colour_slider, SIGNAL (valueChanged (int)), this, SLOT (edge_not_selected_colour_fade_slot()));
        hbox_layout->addWidget (edge_not_selected_colour_slider);
        edge_not_selected_colour_button = new QColorButton;
        c = settings.get_edge_not_selected_colour();
        c *= 255.0f;
        edge_not_selected_colour_button->setColor (QColor (c[0], c[1], c[2]));
        connect (edge_not_selected_colour_button, SIGNAL (clicked()), this, SLOT (edge_not_selected_colour_slot()));
        hbox_layout->addWidget (edge_not_selected_colour_button);
        grid_layout->addLayout (hbox_layout, 1, 1);

        label = new QLabel ("Size: ");
        grid_layout->addWidget (label, 2, 0);
        edge_not_selected_size_button = new AdjustButton (this, 0.01);
        edge_not_selected_size_button->setMin (0.0f);
        edge_not_selected_size_button->setValue (settings.get_edge_not_selected_size_multiplier());
        connect (edge_not_selected_size_button, SIGNAL (valueChanged()), this, SLOT (edge_not_selected_size_slot()));
        grid_layout->addWidget (edge_not_selected_size_button, 2, 1);

        label = new QLabel ("Transparency: ");
        grid_layout->addWidget (label, 3, 0);
        edge_not_selected_alpha_button = new AdjustButton (this, 0.01);
        edge_not_selected_alpha_button->setMin (0.0f);
        edge_not_selected_alpha_button->setValue (settings.get_edge_not_selected_alpha_multiplier());
        connect (edge_not_selected_alpha_button, SIGNAL (valueChanged()), this, SLOT (edge_not_selected_alpha_slot()));
        grid_layout->addWidget (edge_not_selected_alpha_button, 3, 1);

        main_box->addWidget (group_box, 1, 1);

      }




      void NodeSelectionSettingsFrame::node_selected_visibility_slot()
      {
        data.node_selected_visibility_override = node_selected_visibility_checkbox->isChecked();
        emit data.dataChanged();
      }
      void NodeSelectionSettingsFrame::node_selected_colour_fade_slot()
      {
        data.node_selected_colour_fade = node_selected_colour_slider->value() / 100.0f;
        emit data.dataChanged();
      }
      void NodeSelectionSettingsFrame::node_selected_colour_slot()
      {
        const QColor c = node_selected_colour_button->color();
        data.node_selected_colour = Point<float> (c.red() / 255.0f, c.green() / 255.0f, c.blue() / 255.0f);
        emit data.dataChanged();
      }
      void NodeSelectionSettingsFrame::node_selected_size_slot()
      {
        data.node_selected_size_multiplier = node_selected_size_button->value();
        emit data.dataChanged();
      }
      void NodeSelectionSettingsFrame::node_selected_alpha_slot()
      {
        data.node_selected_alpha_multiplier = node_selected_alpha_button->value();
        emit data.dataChanged();
      }


      void NodeSelectionSettingsFrame::edge_selected_visibility_slot()
      {
        data.edge_selected_visibility_override = edge_selected_visibility_checkbox->isChecked();
        emit data.dataChanged();
      }
      void NodeSelectionSettingsFrame::edge_selected_colour_fade_slot()
      {
        data.edge_selected_colour_fade = edge_selected_colour_slider->value() / 100.0f;
        emit data.dataChanged();
      }
      void NodeSelectionSettingsFrame::edge_selected_colour_slot()
      {
        const QColor c = edge_selected_colour_button->color();
        data.edge_selected_colour = Point<float> (c.red() / 255.0f, c.green() / 255.0f, c.blue() / 255.0f);
        emit data.dataChanged();
      }
      void NodeSelectionSettingsFrame::edge_selected_size_slot()
      {
        data.edge_selected_size_multiplier = edge_selected_size_button->value();
        emit data.dataChanged();
      }
      void NodeSelectionSettingsFrame::edge_selected_alpha_slot()
      {
        data.edge_selected_alpha_multiplier = node_selected_alpha_button->value();
        emit data.dataChanged();
      }

      void NodeSelectionSettingsFrame::node_not_selected_visibility_slot()
      {
        data.node_not_selected_visibility_override = node_not_selected_visibility_checkbox->isChecked();
        emit data.dataChanged();
      }
      void NodeSelectionSettingsFrame::node_not_selected_colour_fade_slot()
      {
        data.node_not_selected_colour_fade = node_not_selected_colour_slider->value() / 100.0f;
        emit data.dataChanged();
      }
      void NodeSelectionSettingsFrame::node_not_selected_colour_slot()
      {
        const QColor c = node_not_selected_colour_button->color();
        data.node_not_selected_colour = Point<float> (c.red() / 255.0f, c.green() / 255.0f, c.blue() / 255.0f);
        emit data.dataChanged();
      }
      void NodeSelectionSettingsFrame::node_not_selected_size_slot()
      {
        data.node_not_selected_size_multiplier = node_not_selected_size_button->value();
        emit data.dataChanged();
      }
      void NodeSelectionSettingsFrame::node_not_selected_alpha_slot()
      {
        data.node_not_selected_alpha_multiplier = node_not_selected_alpha_button->value();
        emit data.dataChanged();
      }

      void NodeSelectionSettingsFrame::edge_not_selected_visibility_slot()
      {
        data.edge_not_selected_visibility_override = edge_not_selected_visibility_checkbox->isChecked();
        emit data.dataChanged();
      }
      void NodeSelectionSettingsFrame::edge_not_selected_colour_fade_slot()
      {
        data.edge_not_selected_colour_fade = edge_not_selected_colour_slider->value() / 100.0f;
        emit data.dataChanged();
      }
      void NodeSelectionSettingsFrame::edge_not_selected_colour_slot()
      {
        const QColor c = edge_not_selected_colour_button->color();
        data.edge_not_selected_colour = Point<float> (c.red() / 255.0f, c.green() / 255.0f, c.blue() / 255.0f);
        emit data.dataChanged();
      }
      void NodeSelectionSettingsFrame::edge_not_selected_size_slot()
      {
        data.edge_not_selected_size_multiplier = edge_not_selected_size_button->value();
        emit data.dataChanged();
      }
      void NodeSelectionSettingsFrame::edge_not_selected_alpha_slot()
      {
        data.edge_not_selected_alpha_multiplier = node_not_selected_alpha_button->value();
        emit data.dataChanged();
      }







      NodeSelectionSettingsDialog::NodeSelectionSettingsDialog (QWidget*, const std::string& message, NodeSelectionSettings& settings) :
          frame (new NodeSelectionSettingsFrame (this, settings))
      {
        setWindowTitle (QString (message.c_str()));
        setModal (false);
        setSizeGripEnabled (true);

        QPushButton* close_button = new QPushButton (style()->standardIcon (QStyle::SP_DialogCloseButton), tr ("&Close"));
        connect (close_button, SIGNAL (clicked()), this, SLOT (close()));

        QHBoxLayout* buttons_layout = new QHBoxLayout;
        buttons_layout->addStretch (1);
        buttons_layout->addWidget (close_button);

        QVBoxLayout* main_layout = new QVBoxLayout;
        main_layout->addWidget (frame);
        main_layout->addStretch (1);
        main_layout->addSpacing (12);
        main_layout->addLayout (buttons_layout);
        setLayout (main_layout);
      }




      }
    }
  }
}

