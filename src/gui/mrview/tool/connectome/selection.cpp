/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 * 
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * For more details, see www.mrtrix.org
 * 
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
          node_selected_colour_fade (0.75f),
          node_selected_colour { 1.0f, 1.0f, 1.0f },
          node_selected_size_multiplier (1.0f),
          node_selected_alpha_multiplier (1.0f),
          edge_selected_visibility_override (false),
          edge_selected_colour_fade (0.5f),
          edge_selected_colour { 0.9f, 0.9f, 1.0f },
          edge_selected_size_multiplier (1.0f),
          edge_selected_alpha_multiplier (1.0f),
          node_associated_colour_fade (0.5f),
          node_associated_colour { 0.0f, 0.0f, 0.0f },
          node_associated_size_multiplier (1.0f),
          node_associated_alpha_multiplier (1.0f),
          edge_associated_colour_fade (0.5f),
          edge_associated_colour { 0.0f, 0.0f, 0.0f },
          edge_associated_size_multiplier (1.0f),
          edge_associated_alpha_multiplier (1.0f),
          node_other_visibility_override (false),
          node_other_colour_fade (0.75f),
          node_other_colour { 0.0f, 0.0f, 0.0f },
          node_other_size_multiplier (1.0f),
          node_other_alpha_multiplier (1.0f),
          edge_other_visibility_override (true),
          edge_other_colour_fade (0.75f),
          edge_other_colour { 0.0f, 0.0f, 0.0f },
          edge_other_size_multiplier (1.0f),
          edge_other_alpha_multiplier (1.0f)
      {
        // Load default settings from the config file

        //CONF option: ConnectomeNodeSelectedVisibilityOverride
        //CONF default: true
        //CONF Whether or not nodes are forced to be visible when selected.
        node_selected_visibility_override = File::Config::get_bool ("ConnectomeNodeSelectedVisibilityOverride", true);
        //CONF option: ConnectomeNodeSelectedColourFade
        //CONF default: 0.75
        //CONF The fraction of the colour of a selected node determined by the fixed selection highlight colour.
        node_selected_colour_fade = File::Config::get_float ("ConnectomeNodeSelectedColourFade", 0.75f);
        //CONF option: ConnectomeNodeSelectedColour
        //CONF default: 1.0,1.0,1.0
        //CONF The colour used to highlight those nodes currently selected.
        File::Config::get_RGB ("ConnectomeNodeSelectedColour", node_selected_colour.data(), 1.0f, 1.0f, 1.0f);
        //CONF option: ConnectomeNodeSelectedSizeMultiplier
        //CONF default: 1.0
        //CONF The multiplicative factor to apply to the size of selected nodes.
        node_selected_size_multiplier = File::Config::get_float ("ConnectomeNodeSelectedSizeMultiplier", 1.0f);
        //CONF option: ConnectomeNodeSelectedAlphaMultiplier
        //CONF default: 1.0
        //CONF The multiplicative factor to apply to the transparency of selected nodes.
        node_selected_alpha_multiplier = File::Config::get_float ("ConnectomeNodeSelectedAlphaMultiplier", 1.0f);

        //CONF option: ConnectomeEdgeSelectedVisibilityOverride
        //CONF default: false
        //CONF Whether or not to force visibility of edges connected to two selected nodes.
        edge_selected_visibility_override = File::Config::get_bool ("ConnectomeEdgeSelectedVisibilityOverride", false);
        //CONF option: ConnectomeEdgeSelectedColourFade
        //CONF default: 0.5
        //CONF The fraction of the colour of an edge connected to two selected nodes determined by the fixed selection highlight colour.
        edge_selected_colour_fade = File::Config::get_float ("ConnectomeEdgeSelectedColourFade", 0.5f);
        //CONF option: ConnectomeEdgeSelectedColour
        //CONF default: 0.9,0.9,1.0
        //CONF The colour used to highlight the edges connected to two currently selected nodes.
        File::Config::get_RGB ("ConnectomeEdgeSelectedColour", edge_selected_colour.data(), 0.9f, 0.9f, 1.0f);
        //CONF option: ConnectomeEdgeSelectedSizeMultiplier
        //CONF default: 1.0
        //CONF The multiplicative factor to apply to the size of edges connected to two selected nodes.
        edge_selected_size_multiplier = File::Config::get_float ("ConnectomeEdgeSelectedSizeMultiplier", 1.0f);
        //CONF option: ConnectomeEdgeSelectedAlphaMultiplier
        //CONF default: 1.0
        //CONF The multiplicative factor to apply to the transparency of edges connected to two selected nodes.
        edge_selected_alpha_multiplier = File::Config::get_float ("ConnectomeEdgeSelectedAlphaMultiplier", 1.0f);

        //////////////////////////////////////////////////////////////////////

        //CONF option: ConnectomeNodeAssociatedColourFade
        //CONF default: 0.5
        //CONF The fraction of the colour of an associated node determined by the fixed associated highlight colour.
        node_associated_colour_fade = File::Config::get_float ("ConnectomeNodeAssociatedColourFade", 0.5f);
        //CONF option: ConnectomeNodeAssociatedColour
        //CONF default: 0.0,0.0,0.0
        //CONF The colour mixed in to those nodes associated with any selected node.
        File::Config::get_RGB ("ConnectomeNodeAssociatedColour", node_associated_colour.data(), 0.0f, 0.0f, 0.0f);
        //CONF option: ConnectomeNodeAssociatedSizeMultiplier
        //CONF default: 1.0
        //CONF The multiplicative factor to apply to the size of nodes associated with a selected node.
        node_associated_size_multiplier = File::Config::get_float ("ConnectomeNodeAssociatedSizeMultiplier", 1.0f);
        //CONF option: ConnectomeNodeAssociatedAlphaMultiplier
        //CONF default: 1.0
        //CONF The multiplicative factor to apply to the transparency of nodes associated with a selected node.
        node_associated_alpha_multiplier = File::Config::get_float ("ConnectomeNodeAssociatedAlphaMultiplier", 1.0f);

        //CONF option: ConnectomeEdgeAssociatedColourFade
        //CONF default: 0.5
        //CONF The fraction of the colour of an edge connected to one selected node determined by the fixed colour.
        edge_associated_colour_fade = File::Config::get_float ("ConnectomeEdgeAssociatedColourFade", 0.5f);
        //CONF option: ConnectomeEdgeAssociatedColour
        //CONF default: 0.0,0.0,0.0
        //CONF The colour mixed in to edges connected to one currently selected node.
        File::Config::get_RGB ("ConnectomeEdgeAssociatedColour", edge_associated_colour.data(), 0.0f, 0.0f, 0.0f);
        //CONF option: ConnectomeEdgeAssociatedSizeMultiplier
        //CONF default: 1.0
        //CONF The multiplicative factor to apply to the size of edges connected to one selected node.
        edge_associated_size_multiplier = File::Config::get_float ("ConnectomeEdgeAssociatedSizeMultiplier", 1.0f);
        //CONF option: ConnectomeEdgeAssociatedAlphaMultiplier
        //CONF default: 1.0
        //CONF The multiplicative factor to apply to the transparency of edges connected to one selected node.
        edge_associated_alpha_multiplier = File::Config::get_float ("ConnectomeEdgeAssociatedAlphaMultiplier", 1.0f);

        //////////////////////////////////////////////////////////////////////

        //CONF option: ConnectomeNodeOtherVisibilityOverride
        //CONF default: false
        //CONF Whether or not nodes are forced to be invisible when not selected or associated with any selected node.
        node_other_visibility_override = File::Config::get_bool ("ConnectomeNodeOtherVisibilityOverride", false);
        //CONF option: ConnectomeNodeOtherColourFade
        //CONF default: 0.75
        //CONF The fraction of the colour of an unselected, non-associated node determined by the fixed not-selected highlight colour.
        node_other_colour_fade = File::Config::get_float ("ConnectomeNodeOtherColourFade", 0.75f);
        //CONF option: ConnectomeNodeOtherColour
        //CONF default: 0.0,0.0,0.0
        //CONF The colour mixed in to those nodes currently not selected nor associated with any selected node.
        File::Config::get_RGB ("ConnectomeNodeOtherColour", node_other_colour.data(), 0.0f, 0.0f, 0.0f);
        //CONF option: ConnectomeNodeOtherSizeMultiplier
        //CONF default: 1.0
        //CONF The multiplicative factor to apply to the size of nodes not currently selected nor associated with a selected node.
        node_other_size_multiplier = File::Config::get_float ("ConnectomeNodeOtherSizeMultiplier", 1.0f);
        //CONF option: ConnectomeNodeOtherAlphaMultiplier
        //CONF default: 1.0
        //CONF The multiplicative factor to apply to the transparency of nodes not currently selected nor associated with a selected node.
        node_other_alpha_multiplier = File::Config::get_float ("ConnectomeNodeOtherAlphaMultiplier", 1.0f);

        //CONF option: ConnectomeEdgeOtherVisibilityOverride
        //CONF default: true
        //CONF Whether or not to force invisibility of edges not connected to any selected node.
        edge_other_visibility_override = File::Config::get_bool ("ConnectomeEdgeOtherVisibilityOverride", true);
        //CONF option: ConnectomeEdgeOtherColourFade
        //CONF default: 0.75
        //CONF The fraction of the colour of an edge not connected to any selected node determined by the fixed colour.
        edge_other_colour_fade = File::Config::get_float ("ConnectomeEdgeOtherColourFade", 0.75f);
        //CONF option: ConnectomeEdgeOtherColour
        //CONF default: 0.0,0.0,0.0
        //CONF The colour mixed in to edges not connected to any currently selected node.
        File::Config::get_RGB ("ConnectomeEdgeOtherColour", edge_other_colour.data(), 0.0f, 0.0f, 0.0f);
        //CONF option: ConnectomeEdgeOtherSizeMultiplier
        //CONF default: 1.0
        //CONF The multiplicative factor to apply to the size of edges not connected to any selected node.
        edge_other_size_multiplier = File::Config::get_float ("ConnectomeEdgeOtherSizeMultiplier", 1.0f);
        //CONF option: ConnectomeEdgeOtherAlphaMultiplier
        //CONF default: 1.0
        //CONF The multiplicative factor to apply to the transparency of edges not connected to any selected node.
        edge_other_alpha_multiplier = File::Config::get_float ("ConnectomeEdgeOtherAlphaMultiplier", 1.0f);
      }




      NodeSelectionSettingsFrame::NodeSelectionSettingsFrame (QWidget* parent, NodeSelectionSettings& settings) :
          QFrame (parent),
          data (settings)
      {
        Base::GridLayout* main_box = new Base::GridLayout;
        setLayout (main_box);

        QGroupBox* group_box = new QGroupBox ("Selected nodes highlight");
        QVBoxLayout* vlayout = new QVBoxLayout;
        QFrame* frame = new QFrame (this);
        frame->setFrameShadow (QFrame::Sunken);
        frame->setFrameShape (QFrame::Panel);
        group_box->setLayout (vlayout);
        vlayout->addWidget (frame);
        Base::GridLayout* grid_layout = new Base::GridLayout;
        frame->setLayout (grid_layout);

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
        Eigen::Array3f c = settings.get_node_selected_colour();
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
        frame = new QFrame (this);
        frame->setFrameShadow (QFrame::Sunken);
        frame->setFrameShape (QFrame::Panel);
        vlayout = new QVBoxLayout;
        group_box->setLayout (vlayout);
        vlayout->addWidget (frame);
        grid_layout = new Base::GridLayout;
        frame->setLayout (grid_layout);

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

        //////////////////////////////////////////////////////////////////////

        group_box = new QGroupBox ("Associated nodes highlight");
        frame = new QFrame (this);
        frame->setFrameShadow (QFrame::Sunken);
        frame->setFrameShape (QFrame::Panel);
        vlayout = new QVBoxLayout;
        group_box->setLayout (vlayout);
        vlayout->addWidget (frame);
        grid_layout = new Base::GridLayout;
        frame->setLayout (grid_layout);

        label = new QLabel ("Colour: ");
        grid_layout->addWidget (label, 1, 0);
        hbox_layout = new Base::HBoxLayout;
        node_associated_colour_slider = new QSlider (Qt::Horizontal);
        node_associated_colour_slider->setRange (0, 100);
        node_associated_colour_slider->setSliderPosition (settings.get_node_associated_colour_fade() * 100.0f);
        connect (node_associated_colour_slider, SIGNAL (valueChanged (int)), this, SLOT (node_associated_colour_fade_slot()));
        hbox_layout->addWidget (node_associated_colour_slider);
        node_associated_colour_button = new QColorButton;
        c = settings.get_node_associated_colour();
        c *= 255.0f;
        node_associated_colour_button->setColor (QColor (c[0], c[1], c[2]));
        connect (node_associated_colour_button, SIGNAL (clicked()), this, SLOT (node_associated_colour_slot()));
        hbox_layout->addWidget (node_associated_colour_button);
        grid_layout->addLayout (hbox_layout, 1, 1);

        label = new QLabel ("Size: ");
        grid_layout->addWidget (label, 2, 0);
        node_associated_size_button = new AdjustButton (this, 0.01);
        node_associated_size_button->setMin (0.0f);
        node_associated_size_button->setValue (settings.get_node_associated_size_multiplier());
        connect (node_associated_size_button, SIGNAL (valueChanged()), this, SLOT (node_associated_size_slot()));
        grid_layout->addWidget (node_associated_size_button, 2, 1);

        label = new QLabel ("Transparency: ");
        grid_layout->addWidget (label, 3, 0);
        node_associated_alpha_button = new AdjustButton (this, 0.01);
        node_associated_alpha_button->setMin (0.0f);
        node_associated_alpha_button->setValue (settings.get_node_associated_alpha_multiplier());
        connect (node_associated_alpha_button, SIGNAL (valueChanged()), this, SLOT (node_associated_alpha_slot()));
        grid_layout->addWidget (node_associated_alpha_button, 3, 1);

        main_box->addWidget (group_box, 1, 0);

        group_box = new QGroupBox ("Associated edges highlight");
        frame = new QFrame (this);
        frame->setFrameShadow (QFrame::Sunken);
        frame->setFrameShape (QFrame::Panel);
        vlayout = new QVBoxLayout;
        group_box->setLayout (vlayout);
        vlayout->addWidget (frame);
        grid_layout = new Base::GridLayout;
        frame->setLayout (grid_layout);

        label = new QLabel ("Colour: ");
        grid_layout->addWidget (label, 1, 0);
        hbox_layout = new Base::HBoxLayout;
        edge_associated_colour_slider = new QSlider (Qt::Horizontal);
        edge_associated_colour_slider->setRange (0, 100);
        edge_associated_colour_slider->setSliderPosition (settings.get_edge_associated_colour_fade() * 100.0f);
        connect (edge_associated_colour_slider, SIGNAL (valueChanged (int)), this, SLOT (edge_associated_colour_fade_slot()));
        hbox_layout->addWidget (edge_associated_colour_slider);
        edge_associated_colour_button = new QColorButton;
        c = settings.get_edge_associated_colour();
        c *= 255.0f;
        edge_associated_colour_button->setColor (QColor (c[0], c[1], c[2]));
        connect (edge_associated_colour_button, SIGNAL (clicked()), this, SLOT (edge_associated_colour_slot()));
        hbox_layout->addWidget (edge_associated_colour_button);
        grid_layout->addLayout (hbox_layout, 1, 1);

        label = new QLabel ("Size: ");
        grid_layout->addWidget (label, 2, 0);
        edge_associated_size_button = new AdjustButton (this, 0.01);
        edge_associated_size_button->setMin (0.0f);
        edge_associated_size_button->setValue (settings.get_edge_associated_size_multiplier());
        connect (edge_associated_size_button, SIGNAL (valueChanged()), this, SLOT (edge_associated_size_slot()));
        grid_layout->addWidget (edge_associated_size_button, 2, 1);

        label = new QLabel ("Transparency: ");
        grid_layout->addWidget (label, 3, 0);
        edge_associated_alpha_button = new AdjustButton (this, 0.01);
        edge_associated_alpha_button->setMin (0.0f);
        edge_associated_alpha_button->setValue (settings.get_edge_associated_alpha_multiplier());
        connect (edge_associated_alpha_button, SIGNAL (valueChanged()), this, SLOT (edge_associated_alpha_slot()));
        grid_layout->addWidget (edge_associated_alpha_button, 3, 1);

        main_box->addWidget (group_box, 1, 1);

        //////////////////////////////////////////////////////////////////////

        group_box = new QGroupBox ("Other nodes");
        frame = new QFrame (this);
        frame->setFrameShadow (QFrame::Sunken);
        frame->setFrameShape (QFrame::Panel);
        vlayout = new QVBoxLayout;
        group_box->setLayout (vlayout);
        vlayout->addWidget (frame);
        grid_layout = new Base::GridLayout;
        frame->setLayout (grid_layout);

        label = new QLabel ("Visibility: ");
        grid_layout->addWidget (label, 0, 0);
        node_other_visibility_checkbox = new QCheckBox();
        node_other_visibility_checkbox->setTristate (false);
        node_other_visibility_checkbox->setChecked (settings.get_node_other_visibility_override());
        connect (node_other_visibility_checkbox, SIGNAL (stateChanged(int)), this, SLOT (node_other_visibility_slot()));
        grid_layout->addWidget (node_other_visibility_checkbox, 0, 1);

        label = new QLabel ("Colour: ");
        grid_layout->addWidget (label, 1, 0);
        hbox_layout = new Base::HBoxLayout;
        node_other_colour_slider = new QSlider (Qt::Horizontal);
        node_other_colour_slider->setRange (0, 100);
        node_other_colour_slider->setSliderPosition (settings.get_node_other_colour_fade() * 100.0f);
        connect (node_other_colour_slider, SIGNAL (valueChanged (int)), this, SLOT (node_other_colour_fade_slot()));
        hbox_layout->addWidget (node_other_colour_slider);
        node_other_colour_button = new QColorButton;
        c = settings.get_node_other_colour();
        c *= 255.0f;
        node_other_colour_button->setColor (QColor (c[0], c[1], c[2]));
        connect (node_other_colour_button, SIGNAL (clicked()), this, SLOT (node_other_colour_slot()));
        hbox_layout->addWidget (node_other_colour_button);
        grid_layout->addLayout (hbox_layout, 1, 1);

        label = new QLabel ("Size: ");
        grid_layout->addWidget (label, 2, 0);
        node_other_size_button = new AdjustButton (this, 0.01);
        node_other_size_button->setMin (0.0f);
        node_other_size_button->setValue (settings.get_node_other_size_multiplier());
        connect (node_other_size_button, SIGNAL (valueChanged()), this, SLOT (node_other_size_slot()));
        grid_layout->addWidget (node_other_size_button, 2, 1);

        label = new QLabel ("Transparency: ");
        grid_layout->addWidget (label, 3, 0);
        node_other_alpha_button = new AdjustButton (this, 0.01);
        node_other_alpha_button->setMin (0.0f);
        node_other_alpha_button->setValue (settings.get_node_other_alpha_multiplier());
        connect (node_other_alpha_button, SIGNAL (valueChanged()), this, SLOT (node_other_alpha_slot()));
        grid_layout->addWidget (node_other_alpha_button, 3, 1);

        node_other_visibility_slot();
        main_box->addWidget (group_box, 2, 0);

        group_box = new QGroupBox ("Other edges");
        frame = new QFrame (this);
        frame->setFrameShadow (QFrame::Sunken);
        frame->setFrameShape (QFrame::Panel);
        vlayout = new QVBoxLayout;
        group_box->setLayout (vlayout);
        vlayout->addWidget (frame);
        grid_layout = new Base::GridLayout;
        frame->setLayout (grid_layout);

        label = new QLabel ("Visibility: ");
        grid_layout->addWidget (label, 0, 0);
        edge_other_visibility_checkbox = new QCheckBox();
        edge_other_visibility_checkbox->setTristate (false);
        edge_other_visibility_checkbox->setChecked (settings.get_edge_other_visibility_override());
        connect (edge_other_visibility_checkbox, SIGNAL (stateChanged(int)), this, SLOT (edge_other_visibility_slot()));
        grid_layout->addWidget (edge_other_visibility_checkbox, 0, 1);

        label = new QLabel ("Colour: ");
        grid_layout->addWidget (label, 1, 0);
        hbox_layout = new Base::HBoxLayout;
        edge_other_colour_slider = new QSlider (Qt::Horizontal);
        edge_other_colour_slider->setRange (0, 100);
        edge_other_colour_slider->setSliderPosition (settings.get_edge_other_colour_fade() * 100.0f);
        connect (edge_other_colour_slider, SIGNAL (valueChanged (int)), this, SLOT (edge_other_colour_fade_slot()));
        hbox_layout->addWidget (edge_other_colour_slider);
        edge_other_colour_button = new QColorButton;
        c = settings.get_edge_other_colour();
        c *= 255.0f;
        edge_other_colour_button->setColor (QColor (c[0], c[1], c[2]));
        connect (edge_other_colour_button, SIGNAL (clicked()), this, SLOT (edge_other_colour_slot()));
        hbox_layout->addWidget (edge_other_colour_button);
        grid_layout->addLayout (hbox_layout, 1, 1);

        label = new QLabel ("Size: ");
        grid_layout->addWidget (label, 2, 0);
        edge_other_size_button = new AdjustButton (this, 0.01);
        edge_other_size_button->setMin (0.0f);
        edge_other_size_button->setValue (settings.get_edge_other_size_multiplier());
        connect (edge_other_size_button, SIGNAL (valueChanged()), this, SLOT (edge_other_size_slot()));
        grid_layout->addWidget (edge_other_size_button, 2, 1);

        label = new QLabel ("Transparency: ");
        grid_layout->addWidget (label, 3, 0);
        edge_other_alpha_button = new AdjustButton (this, 0.01);
        edge_other_alpha_button->setMin (0.0f);
        edge_other_alpha_button->setValue (settings.get_edge_other_alpha_multiplier());
        connect (edge_other_alpha_button, SIGNAL (valueChanged()), this, SLOT (edge_other_alpha_slot()));
        grid_layout->addWidget (edge_other_alpha_button, 3, 1);

        edge_other_visibility_slot();
        main_box->addWidget (group_box, 2, 1);

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
        data.node_selected_colour = { c.red() / 255.0f, c.green() / 255.0f, c.blue() / 255.0f };
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
        data.edge_selected_colour = { c.red() / 255.0f, c.green() / 255.0f, c.blue() / 255.0f };
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

      ////////////////////////////////////////////////////////////////////////

      void NodeSelectionSettingsFrame::node_associated_colour_fade_slot()
      {
        data.node_associated_colour_fade = node_associated_colour_slider->value() / 100.0f;
        emit data.dataChanged();
      }
      void NodeSelectionSettingsFrame::node_associated_colour_slot()
      {
        const QColor c = node_associated_colour_button->color();
        data.node_associated_colour = { c.red() / 255.0f, c.green() / 255.0f, c.blue() / 255.0f };
        emit data.dataChanged();
      }
      void NodeSelectionSettingsFrame::node_associated_size_slot()
      {
        data.node_associated_size_multiplier = node_associated_size_button->value();
        emit data.dataChanged();
      }
      void NodeSelectionSettingsFrame::node_associated_alpha_slot()
      {
        data.node_associated_alpha_multiplier = node_associated_alpha_button->value();
        emit data.dataChanged();
      }

      void NodeSelectionSettingsFrame::edge_associated_colour_fade_slot()
      {
        data.edge_associated_colour_fade = edge_associated_colour_slider->value() / 100.0f;
        emit data.dataChanged();
      }
      void NodeSelectionSettingsFrame::edge_associated_colour_slot()
      {
        const QColor c = edge_associated_colour_button->color();
        data.edge_associated_colour = { c.red() / 255.0f, c.green() / 255.0f, c.blue() / 255.0f };
        emit data.dataChanged();
      }
      void NodeSelectionSettingsFrame::edge_associated_size_slot()
      {
        data.edge_associated_size_multiplier = edge_associated_size_button->value();
        emit data.dataChanged();
      }
      void NodeSelectionSettingsFrame::edge_associated_alpha_slot()
      {
        data.edge_associated_alpha_multiplier = node_associated_alpha_button->value();
        emit data.dataChanged();
      }

      ////////////////////////////////////////////////////////////////////////

      void NodeSelectionSettingsFrame::node_other_visibility_slot()
      {
        data.node_other_visibility_override = node_other_visibility_checkbox->isChecked();
        node_other_colour_slider->setEnabled (!data.node_other_visibility_override);
        node_other_colour_button->setEnabled (!data.node_other_visibility_override);
        node_other_size_button  ->setEnabled (!data.node_other_visibility_override);
        node_other_alpha_button ->setEnabled (!data.node_other_visibility_override);
        emit data.dataChanged();
      }
      void NodeSelectionSettingsFrame::node_other_colour_fade_slot()
      {
        data.node_other_colour_fade = node_other_colour_slider->value() / 100.0f;
        emit data.dataChanged();
      }
      void NodeSelectionSettingsFrame::node_other_colour_slot()
      {
        const QColor c = node_other_colour_button->color();
        data.node_other_colour = { c.red() / 255.0f, c.green() / 255.0f, c.blue() / 255.0f };
        emit data.dataChanged();
      }
      void NodeSelectionSettingsFrame::node_other_size_slot()
      {
        data.node_other_size_multiplier = node_other_size_button->value();
        emit data.dataChanged();
      }
      void NodeSelectionSettingsFrame::node_other_alpha_slot()
      {
        data.node_other_alpha_multiplier = node_other_alpha_button->value();
        emit data.dataChanged();
      }

      void NodeSelectionSettingsFrame::edge_other_visibility_slot()
      {
        data.edge_other_visibility_override = edge_other_visibility_checkbox->isChecked();
        edge_other_colour_slider->setEnabled (!data.edge_other_visibility_override);
        edge_other_colour_button->setEnabled (!data.edge_other_visibility_override);
        edge_other_size_button  ->setEnabled (!data.edge_other_visibility_override);
        edge_other_alpha_button ->setEnabled (!data.edge_other_visibility_override);
        emit data.dataChanged();
      }
      void NodeSelectionSettingsFrame::edge_other_colour_fade_slot()
      {
        data.edge_other_colour_fade = edge_other_colour_slider->value() / 100.0f;
        emit data.dataChanged();
      }
      void NodeSelectionSettingsFrame::edge_other_colour_slot()
      {
        const QColor c = edge_other_colour_button->color();
        data.edge_other_colour = { c.red() / 255.0f, c.green() / 255.0f, c.blue() / 255.0f };
        emit data.dataChanged();
      }
      void NodeSelectionSettingsFrame::edge_other_size_slot()
      {
        data.edge_other_size_multiplier = edge_other_size_button->value();
        emit data.dataChanged();
      }
      void NodeSelectionSettingsFrame::edge_other_alpha_slot()
      {
        data.edge_other_alpha_multiplier = node_other_alpha_button->value();
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

