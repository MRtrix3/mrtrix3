/*
   Copyright 2014 Brain Research Institute, Melbourne, Australia

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

#include "gui/mrview/tool/connectome/connectome.h"

#include "thread_queue.h"

#include "file/path.h"
#include "gui/dialog/file.h"
#include "gui/mrview/colourmap.h"
#include "image/adapter/subset.h"
#include "image/buffer.h"
#include "image/header.h"
#include "image/info.h"
#include "image/loop.h"
#include "image/nav.h"
#include "image/transform.h"

#include "math/math.h"
#include "math/rng.h"
#include "math/versor.h"

#include "mesh/mesh.h"
#include "mesh/vox2mesh.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {






        Connectome::Connectome (Window& main_window, Dock* parent) :
            Base (main_window, parent),
            mat2vec (0),
            is_3D (true),
            lighting (this),
            lighting_dialog (nullptr),
            node_visibility (node_visibility_t::ALL),
            node_geometry (node_geometry_t::SPHERE),
            node_colour (node_colour_t::FIXED),
            node_size (node_size_t::FIXED),
            node_alpha (node_alpha_t::FIXED),
            have_meshes (false),
            have_smooth_meshes (false),
            node_fixed_colour (0.5f, 0.5f, 0.5f),
            node_colourmap_index (1),
            node_colourmap_invert (false),
            node_fixed_alpha (1.0f),
            node_size_scale_factor (1.0f),
            voxel_volume (0.0f),
            edge_visibility (edge_visibility_t::NONE),
            edge_geometry (edge_geometry_t::LINE),
            edge_colour (edge_colour_t::FIXED),
            edge_size (edge_size_t::FIXED),
            edge_alpha (edge_alpha_t::FIXED),
            have_exemplars (false),
            edge_fixed_colour (0.5f, 0.5f, 0.5f),
            edge_colourmap_index (1),
            edge_colourmap_invert (false),
            edge_fixed_alpha (1.0f),
            edge_size_scale_factor (1.0f),
            line_thickness_range_aliased {0, 0},
            line_thickness_range_smooth {0, 0},
            node_colourmap_observer (*this),
            edge_colourmap_observer (*this)
        {
          VBoxLayout* main_box = new VBoxLayout (this);

          QGroupBox* group_box = new QGroupBox ("Basic setup");
          main_box->addWidget (group_box);
          VBoxLayout* vlayout = new VBoxLayout;
          group_box->setLayout (vlayout);

          HBoxLayout* hlayout = new HBoxLayout;
          hlayout->setContentsMargins (0, 0, 0, 0);
          hlayout->setSpacing (0);
          hlayout->addWidget (new QLabel ("Node image: "));
          image_button = new QPushButton (this);
          image_button->setToolTip (tr ("Open primary parcellation image\n"
                                        "This should be a 3D image containing an integer value for each\n"
                                        "voxel, indicating the node to which that voxel is assigned."));
          connect (image_button, SIGNAL (clicked()), this, SLOT (image_open_slot ()));
          hlayout->addWidget (image_button, 1);
          hide_all_button = new QPushButton (this);
          hide_all_button->setToolTip (tr ("Hide all connectome visualisation"));
          hide_all_button->setIcon (QIcon (":/hide.svg"));
          hide_all_button->setCheckable (true);
          connect (hide_all_button, SIGNAL (clicked()), this, SLOT (hide_all_slot ()));
          hlayout->addWidget (hide_all_button, 1);
          vlayout->addLayout (hlayout);

          hlayout = new HBoxLayout;
          hlayout->setContentsMargins (0, 0, 0, 0);
          hlayout->setSpacing (0);
          hlayout->addWidget (new QLabel ("LUT: "));
          lut_combobox = new QComboBox (this);
          lut_combobox->setToolTip (tr ("Open lookup table file (must select appropriate format)\n"
                                        "If the primary parcellation image has come from an atlas that\n"
                                        "provides a look-up table, select that file here so that MRview \n"
                                        "can access the node names and colours."));
          for (size_t index = 0; MR::DWI::Tractography::Connectomics::lut_format_strings[index]; ++index)
            lut_combobox->insertItem (index, MR::DWI::Tractography::Connectomics::lut_format_strings[index]);
          connect (lut_combobox, SIGNAL (activated(int)), this, SLOT (lut_open_slot (int)));
          hlayout->addWidget (lut_combobox, 1);
          vlayout->addLayout (hlayout);

          hlayout = new HBoxLayout;
          hlayout->setContentsMargins (0, 0, 0, 0);
          hlayout->setSpacing (0);
          hlayout->addWidget (new QLabel ("Config: "));
          config_button = new QPushButton (this);
          config_button->setToolTip (tr ("Open connectome config file\n"
                                         "If the primary parcellation image has been modified using the\n"
                                         "labelconfig command, the node indices in the image will no longer\n"
                                         "correspond with the indices in the lookup table provided with the\n"
                                         "atlas. Therefore, use this button to provide the config file that\n"
                                         "was utilised by labelconfig so that MRview can cross-reference. "));
          config_button->setText ("(none)");
          connect (config_button, SIGNAL (clicked()), this, SLOT (config_open_slot ()));
          hlayout->addWidget (config_button, 1);
          vlayout->addLayout (hlayout);

          group_box = new QGroupBox ("General display options");
          main_box->addWidget (group_box);
          GridLayout* gridlayout = new GridLayout();
          group_box->setLayout (gridlayout);

          lighting_checkbox = new QCheckBox ("Lighting");
          lighting_checkbox->setTristate (false);
          lighting_checkbox->setChecked (true);
          lighting_checkbox->setToolTip (tr ("Toggle whether lighting should be applied to compatible elements"));
          connect (lighting_checkbox, SIGNAL (stateChanged (int)), this, SLOT (lighting_change_slot (int)));
          gridlayout->addWidget (lighting_checkbox, 0, 0);
          lighting_settings_button = new QPushButton ("Settings...");
          lighting_settings_button->setToolTip (tr ("Advanced lighting configuration"));
          connect (lighting_settings_button, SIGNAL (clicked()), this, SLOT (lighting_settings_slot()));
          gridlayout->addWidget (lighting_settings_button, 0, 1);
          connect (&lighting, SIGNAL (changed()), SLOT (lighting_parameter_slot()));

          dimensionality_combobox = new QComboBox (this);
          dimensionality_combobox->addItem ("2D");
          dimensionality_combobox->addItem ("3D");
          dimensionality_combobox->setCurrentIndex (1);
          connect (dimensionality_combobox, SIGNAL (activated(int)), this, SLOT (dimensionality_slot (int)));
          gridlayout->addWidget (new QLabel ("Dimensionality: "), 1, 0);
          gridlayout->addWidget (dimensionality_combobox, 1, 1);

          group_box = new QGroupBox ("Node visualisation");
          main_box->addWidget (group_box);
          gridlayout = new GridLayout();
          group_box->setLayout (gridlayout);

          QLabel* label = new QLabel ("Visibility: ");
          gridlayout->addWidget (label, 0, 0, 1, 2);
          node_visibility_combobox = new QComboBox (this);
          node_visibility_combobox->setToolTip (tr ("Set which nodes are visible"));
          node_visibility_combobox->addItem ("All");
          node_visibility_combobox->addItem ("None");
          node_visibility_combobox->addItem ("Vector file");
          node_visibility_combobox->addItem ("Degree >= 1");
          connect (node_visibility_combobox, SIGNAL (activated(int)), this, SLOT (node_visibility_selection_slot (int)));
          gridlayout->addWidget (node_visibility_combobox, 0, 2);
          QIcon warning_icon (":/warn.svg");
          node_visibility_warning_icon = new QLabel();
          node_visibility_warning_icon->setPixmap (warning_icon.pixmap (node_visibility_combobox->height()));
          node_visibility_warning_icon->setToolTip ("Changes to node visualisation will have no apparent effect if node visibility is set to \'none\'");
          node_visibility_warning_icon->setVisible (false);
          gridlayout->addWidget (node_visibility_warning_icon, 0, 3);

          hlayout = new HBoxLayout;
          hlayout->setContentsMargins (0, 0, 0, 0);
          hlayout->setSpacing (0);
          node_visibility_threshold_label = new QLabel ("Threshold: ");
          hlayout->addWidget (node_visibility_threshold_label);
          node_visibility_threshold_button = new AdjustButton (this);
          node_visibility_threshold_button->setValue (0.0f);
          node_visibility_threshold_button->setMin (0.0f);
          node_visibility_threshold_button->setMax (0.0f);
          connect (node_visibility_threshold_button, SIGNAL (valueChanged()), this, SLOT (node_visibility_parameter_slot()));
          hlayout->addWidget (node_visibility_threshold_button);
          node_visibility_threshold_invert_checkbox = new QCheckBox ("Invert");
          node_visibility_threshold_invert_checkbox->setTristate (false);
          connect (node_visibility_threshold_invert_checkbox, SIGNAL (stateChanged(int)), this, SLOT (node_visibility_parameter_slot()));
          hlayout->addWidget (node_visibility_threshold_invert_checkbox);
          node_visibility_threshold_label->setVisible (false);
          node_visibility_threshold_button->setVisible (false);
          node_visibility_threshold_invert_checkbox->setVisible (false);
          gridlayout->addLayout (hlayout, 1, 1, 1, 4);

          label = new QLabel ("Geometry: ");
          gridlayout->addWidget (label, 2, 0, 1, 2);
          node_geometry_combobox = new QComboBox (this);
          node_geometry_combobox->setToolTip (tr ("The 3D geometrical shape used to draw each node"));
          node_geometry_combobox->addItem ("Sphere");
          node_geometry_combobox->addItem ("Cube");
          node_geometry_combobox->addItem ("Overlay");
          node_geometry_combobox->addItem ("Mesh");
          node_geometry_combobox->addItem ("Smooth mesh");
          connect (node_geometry_combobox, SIGNAL (activated(int)), this, SLOT (node_geometry_selection_slot (int)));
          gridlayout->addWidget (node_geometry_combobox, 2, 2);
          hlayout = new HBoxLayout;
          hlayout->setContentsMargins (0, 0, 0, 0);
          hlayout->setSpacing (0);
          node_geometry_sphere_lod_label = new QLabel ("LOD: ");
          hlayout->addWidget (node_geometry_sphere_lod_label, 1);
          node_geometry_sphere_lod_spinbox = new QSpinBox (this);
          node_geometry_sphere_lod_spinbox->setMinimum (1);
          node_geometry_sphere_lod_spinbox->setMaximum (7);
          node_geometry_sphere_lod_spinbox->setSingleStep (1);
          node_geometry_sphere_lod_spinbox->setValue (4);
          connect (node_geometry_sphere_lod_spinbox, SIGNAL (valueChanged(int)), this, SLOT(sphere_lod_slot(int)));
          hlayout->addWidget (node_geometry_sphere_lod_spinbox, 1);
          node_geometry_overlay_interp_checkbox = new QCheckBox ("Interp");
          node_geometry_overlay_interp_checkbox->setTristate (false);
          node_geometry_overlay_interp_checkbox->setVisible (false);
          connect (node_geometry_overlay_interp_checkbox, SIGNAL (stateChanged(int)), this, SLOT(overlay_interp_slot(int)));
          hlayout->addWidget (node_geometry_overlay_interp_checkbox, 1);
          gridlayout->addLayout (hlayout, 2, 3, 1, 2);

          label = new QLabel ("Colour: ");
          gridlayout->addWidget (label, 3, 0, 1, 2);
          node_colour_combobox = new QComboBox (this);
          node_colour_combobox->setToolTip (tr ("Set how the colour of each node is determined"));
          node_colour_combobox->addItem ("Fixed");
          node_colour_combobox->addItem ("Random");
          node_colour_combobox->addItem ("LUT");
          node_colour_combobox->addItem ("Vector file");
          connect (node_colour_combobox, SIGNAL (activated(int)), this, SLOT (node_colour_selection_slot (int)));
          gridlayout->addWidget (node_colour_combobox, 3, 2);
          hlayout = new HBoxLayout;
          hlayout->setContentsMargins (0, 0, 0, 0);
          hlayout->setSpacing (0);
          node_colour_fixedcolour_button = new QColorButton;
          connect (node_colour_fixedcolour_button, SIGNAL (clicked()), this, SLOT (node_colour_change_slot()));
          hlayout->addWidget (node_colour_fixedcolour_button, 1);
          node_colour_colourmap_button = new ColourMapButton (this, node_colourmap_observer, false, false, true);
          node_colour_colourmap_button->setVisible (false);
          hlayout->addWidget (node_colour_colourmap_button, 1);
          gridlayout->addLayout (hlayout, 3, 3, 1, 2);

          hlayout = new HBoxLayout;
          hlayout->setContentsMargins (0, 0, 0, 0);
          hlayout->setSpacing (0);
          node_colour_range_label = new QLabel ("Range: ");
          hlayout->addWidget (node_colour_range_label);
          node_colour_lower_button = new AdjustButton (this);
          node_colour_lower_button->setValue (0.0f);
          node_colour_lower_button->setMin (-std::numeric_limits<float>::max());
          node_colour_lower_button->setMax (std::numeric_limits<float>::max());
          connect (node_colour_lower_button, SIGNAL (valueChanged()), this, SLOT (node_colour_parameter_slot()));
          hlayout->addWidget (node_colour_lower_button);
          node_colour_upper_button = new AdjustButton (this);
          node_colour_upper_button->setValue (0.0f);
          node_colour_upper_button->setMin (-std::numeric_limits<float>::max());
          node_colour_upper_button->setMax (std::numeric_limits<float>::max());
          connect (node_colour_upper_button, SIGNAL (valueChanged()), this, SLOT (node_colour_parameter_slot()));
          hlayout->addWidget (node_colour_upper_button);
          node_colour_range_label->setVisible (false);
          node_colour_lower_button->setVisible (false);
          node_colour_upper_button->setVisible (false);
          gridlayout->addLayout (hlayout, 4, 1, 1, 4);

          label = new QLabel ("Size scaling: ");
          gridlayout->addWidget (label, 5, 0, 1, 2);
          node_size_combobox = new QComboBox (this);
          node_size_combobox->setToolTip (tr ("Scale the size of each node"));
          node_size_combobox->addItem ("Fixed");
          node_size_combobox->addItem ("Node volume");
          node_size_combobox->addItem ("Vector file");
          connect (node_size_combobox, SIGNAL (activated(int)), this, SLOT (node_size_selection_slot (int)));
          gridlayout->addWidget (node_size_combobox, 5, 2);
          hlayout = new HBoxLayout;
          hlayout->setContentsMargins (0, 0, 0, 0);
          hlayout->setSpacing (0);
          node_size_button = new AdjustButton (this, 0.01);
          node_size_button->setValue (node_size_scale_factor);
          node_size_button->setMin (0.0f);
          connect (node_size_button, SIGNAL (valueChanged()), this, SLOT (node_size_value_slot()));
          hlayout->addWidget (node_size_button, 1);
          gridlayout->addLayout (hlayout, 5, 3, 1, 2);

          hlayout = new HBoxLayout;
          hlayout->setContentsMargins (0, 0, 0, 0);
          hlayout->setSpacing (0);
          node_size_range_label = new QLabel ("Range: ");
          hlayout->addWidget (node_size_range_label);
          node_size_lower_button = new AdjustButton (this);
          node_size_lower_button->setValue (0.0f);
          node_size_lower_button->setMin (-std::numeric_limits<float>::max());
          node_size_lower_button->setMax (std::numeric_limits<float>::max());
          connect (node_size_lower_button, SIGNAL (valueChanged()), this, SLOT (node_size_parameter_slot()));
          hlayout->addWidget (node_size_lower_button);
          node_size_upper_button = new AdjustButton (this);
          node_size_upper_button->setValue (0.0f);
          node_size_upper_button->setMin (-std::numeric_limits<float>::max());
          node_size_upper_button->setMax (std::numeric_limits<float>::max());
          connect (node_size_upper_button, SIGNAL (valueChanged()), this, SLOT (node_size_parameter_slot()));
          hlayout->addWidget (node_size_upper_button);
          node_size_invert_checkbox = new QCheckBox ("Invert");
          node_size_invert_checkbox->setTristate (false);
          connect (node_size_invert_checkbox, SIGNAL (stateChanged(int)), this, SLOT (node_size_parameter_slot()));
          hlayout->addWidget (node_size_invert_checkbox);
          node_size_range_label->setVisible (false);
          node_size_lower_button->setVisible (false);
          node_size_upper_button->setVisible (false);
          node_size_invert_checkbox->setVisible (false);
          gridlayout->addLayout (hlayout, 6, 1, 1, 4);

          label = new QLabel ("Transparency: ");
          gridlayout->addWidget (label, 7, 0, 1, 2);
          node_alpha_combobox = new QComboBox (this);
          node_alpha_combobox->setToolTip (tr ("Set how node transparency is determined"));
          node_alpha_combobox->addItem ("Fixed");
          node_alpha_combobox->addItem ("LUT");
          node_alpha_combobox->addItem ("Vector file");
          connect (node_alpha_combobox, SIGNAL (activated(int)), this, SLOT (node_alpha_selection_slot (int)));
          gridlayout->addWidget (node_alpha_combobox, 7, 2);
          hlayout = new HBoxLayout;
          hlayout->setContentsMargins (0, 0, 0, 0);
          hlayout->setSpacing (0);
          node_alpha_slider = new QSlider (Qt::Horizontal);
          node_alpha_slider->setRange (0,1000);
          node_alpha_slider->setSliderPosition (1000);
          connect (node_alpha_slider, SIGNAL (valueChanged (int)), this, SLOT (node_alpha_value_slot (int)));
          hlayout->addWidget (node_alpha_slider, 1);
          gridlayout->addLayout (hlayout, 7, 3, 1, 2);

          hlayout = new HBoxLayout;
          hlayout->setContentsMargins (0, 0, 0, 0);
          hlayout->setSpacing (0);
          node_alpha_range_label = new QLabel ("Range: ");
          hlayout->addWidget (node_alpha_range_label);
          node_alpha_lower_button = new AdjustButton (this);
          node_alpha_lower_button->setValue (0.0f);
          node_alpha_lower_button->setMin (-std::numeric_limits<float>::max());
          node_alpha_lower_button->setMax (std::numeric_limits<float>::max());
          connect (node_alpha_lower_button, SIGNAL (valueChanged()), this, SLOT (node_alpha_parameter_slot()));
          hlayout->addWidget (node_alpha_lower_button);
          node_alpha_upper_button = new AdjustButton (this);
          node_alpha_upper_button->setValue (0.0f);
          node_alpha_upper_button->setMin (-std::numeric_limits<float>::max());
          node_alpha_upper_button->setMax (std::numeric_limits<float>::max());
          connect (node_alpha_upper_button, SIGNAL (valueChanged()), this, SLOT (node_alpha_parameter_slot()));
          hlayout->addWidget (node_alpha_upper_button);
          node_alpha_invert_checkbox = new QCheckBox ("Invert");
          node_alpha_invert_checkbox->setTristate (false);
          connect (node_alpha_invert_checkbox, SIGNAL (stateChanged(int)), this, SLOT (node_alpha_parameter_slot()));
          hlayout->addWidget (node_alpha_invert_checkbox);
          node_alpha_range_label->setVisible (false);
          node_alpha_lower_button->setVisible (false);
          node_alpha_upper_button->setVisible (false);
          node_alpha_invert_checkbox->setVisible (false);
          gridlayout->addLayout (hlayout, 8, 1, 1, 4);

          group_box = new QGroupBox ("Edge visualisation");
          main_box->addWidget (group_box);
          gridlayout = new GridLayout();
          group_box->setLayout (gridlayout);

          label = new QLabel ("Visibility: ");
          gridlayout->addWidget (label, 0, 0, 1, 2);
          edge_visibility_combobox = new QComboBox (this);
          edge_visibility_combobox->setToolTip (tr ("Set which edges are visible"));
          edge_visibility_combobox->addItem ("All");
          edge_visibility_combobox->addItem ("None");
          edge_visibility_combobox->addItem ("By nodes");
          edge_visibility_combobox->addItem ("Matrix file");
          edge_visibility_combobox->setCurrentIndex (1);
          connect (edge_visibility_combobox, SIGNAL (activated(int)), this, SLOT (edge_visibility_selection_slot (int)));
          gridlayout->addWidget (edge_visibility_combobox, 0, 2);
          edge_visibility_warning_icon = new QLabel();
          edge_visibility_warning_icon->setPixmap (warning_icon.pixmap (edge_visibility_combobox->height()));
          edge_visibility_warning_icon->setToolTip ("Changes to edge visualisation will have no apparent effect if edge visibility is set to \'none\'");
          edge_visibility_warning_icon->setVisible (false);
          gridlayout->addWidget (edge_visibility_warning_icon, 0, 3);

          hlayout = new HBoxLayout;
          hlayout->setContentsMargins (0, 0, 0, 0);
          hlayout->setSpacing (0);
          edge_visibility_threshold_label = new QLabel ("Threshold: ");
          hlayout->addWidget (edge_visibility_threshold_label);
          edge_visibility_threshold_button = new AdjustButton (this);
          edge_visibility_threshold_button->setValue (0.0f);
          edge_visibility_threshold_button->setMin (0.0f);
          edge_visibility_threshold_button->setMax (0.0f);
          connect (edge_visibility_threshold_button, SIGNAL (valueChanged()), this, SLOT (edge_visibility_parameter_slot()));
          hlayout->addWidget (edge_visibility_threshold_button);
          edge_visibility_threshold_invert_checkbox = new QCheckBox ("Invert");
          edge_visibility_threshold_invert_checkbox->setTristate (false);
          connect (edge_visibility_threshold_invert_checkbox, SIGNAL (stateChanged(int)), this, SLOT (edge_visibility_parameter_slot()));
          hlayout->addWidget (edge_visibility_threshold_invert_checkbox);
          edge_visibility_threshold_label->setVisible (false);
          edge_visibility_threshold_button->setVisible (false);
          edge_visibility_threshold_invert_checkbox->setVisible (false);
          gridlayout->addLayout (hlayout, 1, 1, 1, 4);

          label = new QLabel ("Geometry: ");
          gridlayout->addWidget (label, 2, 0, 1, 2);
          edge_geometry_combobox = new QComboBox (this);
          edge_geometry_combobox->setToolTip (tr ("The geometry used to draw each edge"));
          edge_geometry_combobox->addItem ("Line");
          edge_geometry_combobox->addItem ("Cylinder");
          edge_geometry_combobox->addItem ("Streamline");
          edge_geometry_combobox->addItem ("Streamtube");
          connect (edge_geometry_combobox, SIGNAL (activated(int)), this, SLOT (edge_geometry_selection_slot (int)));
          gridlayout->addWidget (edge_geometry_combobox, 2, 2);
          hlayout = new HBoxLayout;
          hlayout->setContentsMargins (0, 0, 0, 0);
          hlayout->setSpacing (0);
          edge_geometry_cylinder_lod_label = new QLabel ("LOD: ");
          edge_geometry_cylinder_lod_label->setVisible (false);
          hlayout->addWidget (edge_geometry_cylinder_lod_label, 1);
          edge_geometry_cylinder_lod_spinbox = new QSpinBox (this);
          edge_geometry_cylinder_lod_spinbox->setMinimum (1);
          edge_geometry_cylinder_lod_spinbox->setMaximum (7);
          edge_geometry_cylinder_lod_spinbox->setSingleStep (1);
          edge_geometry_cylinder_lod_spinbox->setValue (4);
          edge_geometry_cylinder_lod_spinbox->setVisible (false);
          connect (edge_geometry_cylinder_lod_spinbox, SIGNAL (valueChanged(int)), this, SLOT(cylinder_lod_slot(int)));
          hlayout->addWidget (edge_geometry_cylinder_lod_spinbox, 1);
          edge_geometry_line_smooth_checkbox = new QCheckBox ("Smooth");
          edge_geometry_line_smooth_checkbox->setTristate (false);
          connect (edge_geometry_line_smooth_checkbox, SIGNAL (stateChanged(int)), this, SLOT(edge_size_value_slot()));
          hlayout->addWidget (edge_geometry_line_smooth_checkbox, 1);
          gridlayout->addLayout (hlayout, 2, 3, 1, 2);

          label = new QLabel ("Colour: ");
          gridlayout->addWidget (label, 3, 0, 1, 2);
          edge_colour_combobox = new QComboBox (this);
          edge_colour_combobox->setToolTip (tr ("Set how the colour of each edge is determined"));
          edge_colour_combobox->addItem ("Fixed");
          edge_colour_combobox->addItem ("By direction");
          edge_colour_combobox->addItem ("Matrix file");
          connect (edge_colour_combobox, SIGNAL (activated(int)), this, SLOT (edge_colour_selection_slot (int)));
          gridlayout->addWidget (edge_colour_combobox, 3, 2);
          hlayout = new HBoxLayout;
          hlayout->setContentsMargins (0, 0, 0, 0);
          hlayout->setSpacing (0);
          edge_colour_fixedcolour_button = new QColorButton;
          connect (edge_colour_fixedcolour_button, SIGNAL (clicked()), this, SLOT (edge_colour_change_slot()));
          hlayout->addWidget (edge_colour_fixedcolour_button, 1);
          edge_colour_colourmap_button = new ColourMapButton (this, edge_colourmap_observer, false, false, true);
          edge_colour_colourmap_button->setVisible (false);
          hlayout->addWidget (edge_colour_colourmap_button, 1);
          gridlayout->addLayout (hlayout, 3, 3, 1, 2);

          hlayout = new HBoxLayout;
          hlayout->setContentsMargins (0, 0, 0, 0);
          hlayout->setSpacing (0);
          edge_colour_range_label = new QLabel ("Range: ");
          hlayout->addWidget (edge_colour_range_label);
          edge_colour_lower_button = new AdjustButton (this);
          edge_colour_lower_button->setValue (0.0f);
          edge_colour_lower_button->setMin (-std::numeric_limits<float>::max());
          edge_colour_lower_button->setMax (std::numeric_limits<float>::max());
          connect (edge_colour_lower_button, SIGNAL (valueChanged()), this, SLOT (edge_colour_parameter_slot()));
          hlayout->addWidget (edge_colour_lower_button);
          edge_colour_upper_button = new AdjustButton (this);
          edge_colour_upper_button->setValue (0.0f);
          edge_colour_upper_button->setMin (-std::numeric_limits<float>::max());
          edge_colour_upper_button->setMax (std::numeric_limits<float>::max());
          connect (edge_colour_upper_button, SIGNAL (valueChanged()), this, SLOT (edge_colour_parameter_slot()));
          hlayout->addWidget (edge_colour_upper_button);
          edge_colour_range_label->setVisible (false);
          edge_colour_lower_button->setVisible (false);
          edge_colour_upper_button->setVisible (false);
          gridlayout->addLayout (hlayout, 4, 1, 1, 4);

          label = new QLabel ("Size scaling: ");
          gridlayout->addWidget (label, 5, 0, 1, 2);
          edge_size_combobox = new QComboBox (this);
          edge_size_combobox->setToolTip (tr ("Scale the width of each edge"));
          edge_size_combobox->addItem ("Fixed");
          edge_size_combobox->addItem ("Matrix file");
          connect (edge_size_combobox, SIGNAL (activated(int)), this, SLOT (edge_size_selection_slot (int)));
          gridlayout->addWidget (edge_size_combobox, 5, 2);
          hlayout = new HBoxLayout;
          hlayout->setContentsMargins (0, 0, 0, 0);
          hlayout->setSpacing (0);
          edge_size_button = new AdjustButton (this, 0.01);
          edge_size_button->setValue (edge_size_scale_factor);
          edge_size_button->setMin (0.0f);
          connect (edge_size_button, SIGNAL (valueChanged()), this, SLOT (edge_size_value_slot()));
          hlayout->addWidget (edge_size_button, 1);
          gridlayout->addLayout (hlayout, 5, 3, 1, 2);

          hlayout = new HBoxLayout;
          hlayout->setContentsMargins (0, 0, 0, 0);
          hlayout->setSpacing (0);
          edge_size_range_label = new QLabel ("Range: ");
          hlayout->addWidget (edge_size_range_label);
          edge_size_lower_button = new AdjustButton (this);
          edge_size_lower_button->setValue (0.0f);
          edge_size_lower_button->setMin (-std::numeric_limits<float>::max());
          edge_size_lower_button->setMax (std::numeric_limits<float>::max());
          connect (edge_size_lower_button, SIGNAL (valueChanged()), this, SLOT (edge_size_parameter_slot()));
          hlayout->addWidget (edge_size_lower_button);
          edge_size_upper_button = new AdjustButton (this);
          edge_size_upper_button->setValue (0.0f);
          edge_size_upper_button->setMin (-std::numeric_limits<float>::max());
          edge_size_upper_button->setMax (std::numeric_limits<float>::max());
          connect (edge_size_upper_button, SIGNAL (valueChanged()), this, SLOT (edge_size_parameter_slot()));
          hlayout->addWidget (edge_size_upper_button);
          edge_size_invert_checkbox = new QCheckBox ("Invert");
          edge_size_invert_checkbox->setTristate (false);
          connect (edge_size_invert_checkbox, SIGNAL (stateChanged(int)), this, SLOT (edge_size_parameter_slot()));
          hlayout->addWidget (edge_size_invert_checkbox);
          edge_size_range_label->setVisible (false);
          edge_size_lower_button->setVisible (false);
          edge_size_upper_button->setVisible (false);
          edge_size_invert_checkbox->setVisible (false);
          gridlayout->addLayout (hlayout, 6, 1, 1, 4);

          label = new QLabel ("Transparency: ");
          gridlayout->addWidget (label, 7, 0, 1, 2);
          edge_alpha_combobox = new QComboBox (this);
          edge_alpha_combobox->setToolTip (tr ("Set how node transparency is determined"));
          edge_alpha_combobox->addItem ("Fixed");
          edge_alpha_combobox->addItem ("Matrix file");
          connect (edge_alpha_combobox, SIGNAL (activated(int)), this, SLOT (edge_alpha_selection_slot (int)));
          gridlayout->addWidget (edge_alpha_combobox, 7, 2);
          hlayout = new HBoxLayout;
          hlayout->setContentsMargins (0, 0, 0, 0);
          hlayout->setSpacing (0);
          edge_alpha_slider = new QSlider (Qt::Horizontal);
          edge_alpha_slider->setRange (0,1000);
          edge_alpha_slider->setSliderPosition (1000);
          connect (edge_alpha_slider, SIGNAL (valueChanged (int)), this, SLOT (edge_alpha_value_slot (int)));
          hlayout->addWidget (edge_alpha_slider, 1);
          gridlayout->addLayout (hlayout, 7, 3, 1, 2);

          hlayout = new HBoxLayout;
          hlayout->setContentsMargins (0, 0, 0, 0);
          hlayout->setSpacing (0);
          edge_alpha_range_label = new QLabel ("Range: ");
          hlayout->addWidget (edge_alpha_range_label);
          edge_alpha_lower_button = new AdjustButton (this);
          edge_alpha_lower_button->setValue (0.0f);
          edge_alpha_lower_button->setMin (-std::numeric_limits<float>::max());
          edge_alpha_lower_button->setMax (std::numeric_limits<float>::max());
          connect (edge_alpha_lower_button, SIGNAL (valueChanged()), this, SLOT (edge_alpha_parameter_slot()));
          hlayout->addWidget (edge_alpha_lower_button);
          edge_alpha_upper_button = new AdjustButton (this);
          edge_alpha_upper_button->setValue (0.0f);
          edge_alpha_upper_button->setMin (-std::numeric_limits<float>::max());
          edge_alpha_upper_button->setMax (std::numeric_limits<float>::max());
          connect (edge_alpha_upper_button, SIGNAL (valueChanged()), this, SLOT (edge_alpha_parameter_slot()));
          hlayout->addWidget (edge_alpha_upper_button);
          edge_alpha_invert_checkbox = new QCheckBox ("Invert");
          edge_alpha_invert_checkbox->setTristate (false);
          connect (edge_alpha_invert_checkbox, SIGNAL (stateChanged(int)), this, SLOT (edge_alpha_parameter_slot()));
          hlayout->addWidget (edge_alpha_invert_checkbox);
          edge_alpha_range_label->setVisible (false);
          edge_alpha_lower_button->setVisible (false);
          edge_alpha_upper_button->setVisible (false);
          edge_alpha_invert_checkbox->setVisible (false);
          gridlayout->addLayout (hlayout, 8, 1, 1, 4);

          main_box->addStretch ();
          setMinimumSize (main_box->minimumSize());

          cube.generate();
          cube_VAO.gen();
          cube_VAO.bind();
          cube.vertex_buffer.bind (gl::ARRAY_BUFFER);
          gl::EnableVertexAttribArray (0);
          gl::VertexAttribPointer (0, 3, gl::FLOAT, gl::FALSE_, 0, (void*)(0));
          cube.normals_buffer.bind (gl::ARRAY_BUFFER);
          gl::EnableVertexAttribArray (1);
          gl::VertexAttribPointer (1, 3, gl::FLOAT, gl::FALSE_, 0, (void*)(0));

          cylinder.LOD (4);
          cylinder_VAO.gen();
          cylinder_VAO.bind();
          cylinder.vertex_buffer.bind (gl::ARRAY_BUFFER);
          gl::EnableVertexAttribArray (0);
          gl::VertexAttribPointer (0, 3, gl::FLOAT, gl::FALSE_, 0, (void*)(0));
          cylinder.normal_buffer.bind (gl::ARRAY_BUFFER);
          gl::EnableVertexAttribArray (1);
          gl::VertexAttribPointer (1, 3, gl::FLOAT, gl::FALSE_, 0, (void*)(0));

          sphere.LOD (4);
          sphere_VAO.gen();
          sphere_VAO.bind();
          sphere.vertex_buffer.bind (gl::ARRAY_BUFFER);
          gl::EnableVertexAttribArray (0);
          gl::VertexAttribPointer (0, 3, gl::FLOAT, gl::FALSE_, 0, (void*)(0));

          Edge::set_streamtube_LOD (3);

          glGetIntegerv (GL_ALIASED_LINE_WIDTH_RANGE, line_thickness_range_aliased);
          glGetIntegerv (GL_SMOOTH_LINE_WIDTH_RANGE, line_thickness_range_smooth);
          GL_CHECK_ERROR;

          enable_all (false);
        }


        Connectome::~Connectome () {}


        void Connectome::draw (const Projection& projection, bool /*is_3D*/, int /*axis*/, int /*slice*/)
        {
          if (hide_all_button->isChecked()) return;

          if (node_visibility != node_visibility_t::NONE) {

            if (node_geometry == node_geometry_t::OVERLAY) {

              if (is_3D) {
                window.get_current_mode()->overlays_for_3D.push_back (node_overlay.get());
              } else {
                // set up OpenGL environment:
                gl::Enable (gl::BLEND);
                gl::Disable (gl::DEPTH_TEST);
                gl::DepthMask (gl::FALSE_);
                gl::ColorMask (gl::TRUE_, gl::TRUE_, gl::TRUE_, gl::TRUE_);
                gl::BlendFunc (gl::SRC_ALPHA, gl::ONE_MINUS_SRC_ALPHA);
                gl::BlendEquation (gl::FUNC_ADD);

                node_overlay->render3D (node_overlay->slice_shader, projection, projection.depth_of (window.focus()));

                // restore OpenGL environment:
                gl::Disable (gl::BLEND);
                gl::Enable (gl::DEPTH_TEST);
                gl::DepthMask (gl::TRUE_);
              }

            } else {

              node_shader.start (*this);
              projection.set (node_shader);

              const bool use_alpha = !(node_alpha == node_alpha_t::FIXED && node_fixed_alpha == 1.0f);

              gl::Enable (gl::DEPTH_TEST);
              if (use_alpha) {
                gl::Enable (gl::BLEND);
                gl::DepthMask (gl::FALSE_);
                gl::BlendEquation (gl::FUNC_ADD);
                //gl::BlendFunc (gl::SRC_ALPHA, gl::ONE_MINUS_SRC_ALPHA);
                //gl::BlendFunc (gl::SRC_ALPHA, gl::DST_ALPHA);
                gl::BlendFuncSeparate (gl::SRC_ALPHA, gl::ONE_MINUS_SRC_ALPHA, gl::SRC_ALPHA, gl::DST_ALPHA);
                gl::BlendColor (1.0, 1.0, 1.0, node_fixed_alpha);
                //gl::Disable (gl::CULL_FACE);
              } else {
                gl::Disable (gl::BLEND);
                 if (is_3D)
                  gl::DepthMask (gl::TRUE_);
                else
                  gl::DepthMask (gl::FALSE_);
                //gl::Enable (gl::CULL_FACE);
              }

              const GLuint node_colour_ID = gl::GetUniformLocation (node_shader, "node_colour");

              GLuint node_alpha_ID = 0;
              if (use_alpha)
                node_alpha_ID = gl::GetUniformLocation (node_shader, "node_alpha");

              GLuint node_centre_ID = 0, node_size_ID = 0, reverse_ID = 0;
              if (node_geometry != node_geometry_t::OVERLAY) {
                node_centre_ID = gl::GetUniformLocation (node_shader, "node_centre");
                node_size_ID = gl::GetUniformLocation (node_shader, "node_size");
              }

              if (node_colour == node_colour_t::FILE && ColourMap::maps[node_colourmap_index].is_colour)
                gl::Uniform3fv (gl::GetUniformLocation (node_shader, "colourmap_colour"), 1, &node_fixed_colour[0]);

              if (node_geometry == node_geometry_t::SPHERE) {
                sphere.vertex_buffer.bind (gl::ARRAY_BUFFER);
                sphere_VAO.bind();
                sphere.index_buffer.bind();
                reverse_ID = gl::GetUniformLocation (node_shader, "reverse");
              } else if (node_geometry == node_geometry_t::CUBE) {
                cube.vertex_buffer.bind (gl::ARRAY_BUFFER);
                cube.normals_buffer.bind (gl::ARRAY_BUFFER);
                cube_VAO.bind();
                cube.index_buffer.bind();
                glShadeModel (GL_FLAT);
                gl::ProvokingVertex (gl::FIRST_VERTEX_CONVENTION);
              }

              if (node_geometry != node_geometry_t::OVERLAY) {
                gl::Uniform3fv (gl::GetUniformLocation (node_shader, "light_pos"), 1, lighting.lightpos);
                gl::Uniform1f  (gl::GetUniformLocation (node_shader, "ambient"), lighting.ambient);
                gl::Uniform1f  (gl::GetUniformLocation (node_shader, "diffuse"), lighting.diffuse);
                gl::Uniform1f  (gl::GetUniformLocation (node_shader, "specular"), lighting.specular);
                gl::Uniform1f  (gl::GetUniformLocation (node_shader, "shine"), lighting.shine);
                gl::Uniform3fv (gl::GetUniformLocation (node_shader, "screen_normal"), 1, projection.screen_normal());
              }

              std::map<float, size_t> node_ordering;
              for (size_t i = 1; i <= num_nodes(); ++i)
                node_ordering.insert (std::make_pair (projection.depth_of (nodes[i].get_com()), i));

              for (auto it = node_ordering.rbegin(); it != node_ordering.rend(); ++it) {
                const Node& node (nodes[it->second]);
                if (node.is_visible()) {
                  gl::Uniform3fv (node_colour_ID, 1, node.get_colour());
                  if (use_alpha)
                    gl::Uniform1f (node_alpha_ID, node.get_alpha() * node_fixed_alpha);
                  if (node_geometry != node_geometry_t::OVERLAY) {
                    gl::Uniform3fv (node_centre_ID, 1, &node.get_com()[0]);
                    gl::Uniform1f (node_size_ID, node.get_size() * node_size_scale_factor);
                  }
                  switch (node_geometry) {
                    case node_geometry_t::SPHERE:
                      gl::Uniform1i (reverse_ID, 0);
                      gl::DrawElements (gl::TRIANGLES, sphere.num_indices, gl::UNSIGNED_INT, (void*)0);
                      gl::Uniform1i (reverse_ID, 1);
                      gl::DrawElements (gl::TRIANGLES, sphere.num_indices, gl::UNSIGNED_INT, (void*)0);
                      break;
                    case node_geometry_t::CUBE:
                      gl::DrawElements (gl::TRIANGLES, cube.num_indices, gl::UNSIGNED_INT, (void*)0);
                      break;
                    case node_geometry_t::OVERLAY:
                      break;
                    case node_geometry_t::MESH:
                      node.render_mesh();
                      break;
                    case node_geometry_t::SMOOTH_MESH:
                      node.render_smooth_mesh();
                      break;
                  }
                }
              }

              // Reset to defaults if we've been doing transparency
              if (use_alpha) {
                gl::Disable (gl::BLEND);
                gl::DepthMask (gl::TRUE_);
              }

              if (node_geometry == node_geometry_t::CUBE)
                glShadeModel (GL_SMOOTH);

              node_shader.stop();

            }

          }

          // =================================================================

          if (edge_visibility != edge_visibility_t::NONE) {

            edge_shader.start (*this);
            projection.set (edge_shader);

            const bool use_alpha = !(edge_alpha == edge_alpha_t::FIXED && edge_fixed_alpha == 1.0f);

            gl::Enable (gl::DEPTH_TEST);
            if (use_alpha) {
              gl::Enable (gl::BLEND);
              gl::DepthMask (gl::FALSE_);
              gl::BlendEquation (gl::FUNC_ADD);
              //gl::BlendFunc (gl::SRC_ALPHA, gl::ONE_MINUS_SRC_ALPHA);
              //gl::BlendFunc (gl::SRC_ALPHA, gl::DST_ALPHA);
              gl::BlendFuncSeparate (gl::SRC_ALPHA, gl::ONE_MINUS_SRC_ALPHA, gl::SRC_ALPHA, gl::DST_ALPHA);
              gl::BlendColor (1.0, 1.0, 1.0, edge_fixed_alpha);
            } else {
              gl::Disable (gl::BLEND);
              if (is_3D)
                gl::DepthMask (gl::TRUE_);
              else
                gl::DepthMask (gl::FALSE_);
            }

            if ((edge_geometry == edge_geometry_t::LINE || edge_geometry == edge_geometry_t::STREAMLINE) && edge_geometry_line_smooth_checkbox->isChecked())
              gl::Enable (GL_LINE_SMOOTH);

            GLuint node_centre_one_ID = 0, node_centre_two_ID = 0, rot_matrix_ID = 0, radius_ID = 0;
            if (edge_geometry == edge_geometry_t::CYLINDER) {
              cylinder.vertex_buffer.bind (gl::ARRAY_BUFFER);
              cylinder_VAO.bind();
              cylinder.index_buffer.bind();
              node_centre_one_ID = gl::GetUniformLocation (edge_shader, "centre_one");
              node_centre_two_ID = gl::GetUniformLocation (edge_shader, "centre_two");
              rot_matrix_ID      = gl::GetUniformLocation (edge_shader, "rot_matrix");
            }

            if (edge_geometry == edge_geometry_t::CYLINDER || edge_geometry == edge_geometry_t::STREAMTUBE) {
              radius_ID     = gl::GetUniformLocation (edge_shader, "radius");
              gl::Uniform3fv (gl::GetUniformLocation (edge_shader, "light_pos"), 1, lighting.lightpos);
              gl::Uniform1f  (gl::GetUniformLocation (edge_shader, "ambient"), lighting.ambient);
              gl::Uniform1f  (gl::GetUniformLocation (edge_shader, "diffuse"), lighting.diffuse);
              gl::Uniform1f  (gl::GetUniformLocation (edge_shader, "specular"), lighting.specular);
              gl::Uniform1f  (gl::GetUniformLocation (edge_shader, "shine"), lighting.shine);
              gl::Uniform3fv (gl::GetUniformLocation (edge_shader, "screen_normal"), 1, projection.screen_normal());
            }

            const GLuint edge_colour_ID = gl::GetUniformLocation (edge_shader, "edge_colour");

            GLuint edge_alpha_ID = 0;
            if (use_alpha)
              edge_alpha_ID = gl::GetUniformLocation (edge_shader, "edge_alpha");

            if (edge_colour == edge_colour_t::FILE && ColourMap::maps[edge_colourmap_index].is_colour)
                gl::Uniform3fv (gl::GetUniformLocation (edge_shader, "colourmap_colour"), 1, &edge_fixed_colour[0]);

            std::map<float, size_t> edge_ordering;
            for (size_t i = 0; i != num_edges(); ++i)
              edge_ordering.insert (std::make_pair (projection.depth_of (edges[i].get_com()), i));

            for (auto it = edge_ordering.rbegin(); it != edge_ordering.rend(); ++it) {
              const Edge& edge (edges[it->second]);
              if (edge.is_visible()) {
                gl::Uniform3fv (edge_colour_ID, 1, edge.get_colour());
                if (use_alpha)
                  gl::Uniform1f (edge_alpha_ID, edge.get_alpha() * edge_fixed_alpha);
                switch (edge_geometry) {
                  case edge_geometry_t::LINE:
                    gl::LineWidth (edge.get_size() * edge_size_scale_factor);
                    edge.render_line();
                    break;
                  case edge_geometry_t::CYLINDER:
                    gl::Uniform3fv       (node_centre_one_ID, 1,        edge.get_node_centre (0));
                    gl::Uniform3fv       (node_centre_two_ID, 1,        edge.get_node_centre (1));
                    gl::UniformMatrix3fv (rot_matrix_ID,      1, false, edge.get_rot_matrix());
                    gl::Uniform1f        (radius_ID,                    std::sqrt (edge.get_size() * edge_size_scale_factor / Math::pi));
                    gl::DrawElements     (gl::TRIANGLES, cylinder.num_indices, gl::UNSIGNED_INT, (void*)0);
                    break;
                  case edge_geometry_t::STREAMLINE:
                    gl::LineWidth (edge.get_size() * edge_size_scale_factor);
                    edge.render_streamline();
                    break;
                  case edge_geometry_t::STREAMTUBE:
                    gl::Uniform1f (radius_ID, std::sqrt (edge.get_size() * edge_size_scale_factor / Math::pi));
                    edge.render_streamtube();
                }
              }
            }

            // Reset to defaults if we've been doing transparency
            if (use_alpha) {
              gl::Disable (gl::BLEND);
              gl::DepthMask (gl::TRUE_);
            }

            if (edge_geometry == edge_geometry_t::LINE || edge_geometry == edge_geometry_t::STREAMLINE) {
              gl::LineWidth (1.0f);
              if (edge_geometry_line_smooth_checkbox->isChecked())
                gl::Disable (GL_LINE_SMOOTH);
            }

            edge_shader.stop();
          }
        }


        //void Connectome::drawOverlays (const Projection& transform)
        void Connectome::drawOverlays (const Projection&)
        {
          if (hide_all_button->isChecked()) return;
        }


        bool Connectome::process_batch_command (const std::string& cmd, const std::string& args)
        {
          // BATCH_COMMAND connectome.load path # Load the connectome tool based on a parcellation image
          if (cmd == "connectome.load") {
            try {
              initialise (args);
              window.updateGL();
            }
            catch (Exception& E) { clear_all(); E.display(); }
            return true;
          }
          return false;
        }


        void Connectome::add_commandline_options (MR::App::OptionList& options)
        {
          using namespace MR::App;
          options
            + OptionGroup ("Connectome tool options")

            + Option ("connectome.load", "Loads the specified parcellation image on the connectome tool.")
            +   Argument ("image").type_image_in();
        }

        bool Connectome::process_commandline_option (const MR::App::ParsedOption& opt)
        {
          if (opt.opt->is ("connectome.load")) {
            try {
              initialise (opt[0]);
              image_button->setText (QString::fromStdString (Path::basename (opt[0])));
              load_properties();
            } catch (Exception& e) { e.display(); clear_all(); }
            return true;
          }
          return false;
        }


        void Connectome::image_open_slot()
        {
          const std::string path = Dialog::File::get_image (this, "Select connectome parcellation image");
          if (path.empty())
            return;

          // If a new parcellation image is opened, all other data should be invalidated
          clear_all();

          // Read in the image file, do the necessary conversions e.g. to mesh, store the number of nodes, ...
          initialise (path);

          image_button->setText (QString::fromStdString (Path::basename (path)));
          load_properties();
          enable_all (true);
          window.updateGL();
        }


        void Connectome::lut_open_slot (int index)
        {
          if (!index) {
            lut.clear();
            lut_mapping.clear();
            lut_combobox->removeItem (5);
            load_properties();
            return;
          }
          if (index == 5)
            return; // Selected currently-open LUT; nothing to do

          const std::string path = Dialog::File::get_file (this, std::string("Select lookup table file (in ") + MR::DWI::Tractography::Connectomics::lut_format_strings[index] + " format)");
          if (path.empty())
            return;

          lut.clear();
          lut_mapping.clear();
          lut_combobox->removeItem (5);

          try {
            switch (index) {
              case 1: lut.load (path, MR::DWI::Tractography::Connectomics::LUT_BASIC);      break;
              case 2: lut.load (path, MR::DWI::Tractography::Connectomics::LUT_FREESURFER); break;
              case 3: lut.load (path, MR::DWI::Tractography::Connectomics::LUT_AAL);        break;
              case 4: lut.load (path, MR::DWI::Tractography::Connectomics::LUT_ITKSNAP);    break;
              default: assert (0);
            }
          } catch (Exception& e) { e.display(); lut.clear(); lut_combobox->setCurrentIndex (0); return; }

          lut_combobox->insertItem (5, QString::fromStdString (Path::basename (path)));
          lut_combobox->setCurrentIndex (5);

          load_properties();
          window.updateGL();
        }


        void Connectome::config_open_slot()
        {
          const std::string path = Dialog::File::get_file (this, "Select connectome configuration file");
          if (path.empty())
            return;
          config.clear();
          lut_mapping.clear();
          config_button->setText ("(none)");
          try {
            MR::DWI::Tractography::Connectomics::load_config (path, config);
            config_button->setText (QString::fromStdString (Path::basename (path)));
          } catch (Exception& e) {
            e.display();
            config.clear();
          }
          load_properties();
          window.updateGL();
        }

        void Connectome::hide_all_slot()
        {
          window.updateGL();
        }






        void Connectome::lighting_change_slot (int /*value*/)
        {
          window.updateGL();
        }
        void Connectome::lighting_settings_slot()
        {
          if (!lighting_dialog)
            lighting_dialog.reset (new Dialog::Lighting (&window, "Connectome lighting", lighting, false));
          lighting_dialog->show();
        }
        void Connectome::lighting_parameter_slot()
        {
          if (use_lighting())
            window.updateGL();
        }
        void Connectome::dimensionality_slot (int index)
        {
          is_3D = index;
          window.updateGL();
        }








        void Connectome::node_visibility_selection_slot (int index)
        {
          node_visibility_warning_icon->setVisible (false);
          switch (index) {
            case 0:
              if (node_visibility == node_visibility_t::ALL) return;
              node_visibility = node_visibility_t::ALL;
              node_visibility_combobox->removeItem (4);
              node_visibility_threshold_label->setVisible (false);
              node_visibility_threshold_button->setVisible (false);
              node_visibility_threshold_invert_checkbox->setVisible (false);
              break;
            case 1:
              if (node_visibility == node_visibility_t::NONE) return;
              node_visibility = node_visibility_t::NONE;
              node_visibility_combobox->removeItem (4);
              node_visibility_threshold_label->setVisible (false);
              node_visibility_threshold_button->setVisible (false);
              node_visibility_threshold_invert_checkbox->setVisible (false);
              break;
            case 2:
              try {
                if (!import_file_for_node_property (node_values_from_file_visibility, "visibility")) {
                  switch (node_visibility) {
                    case node_visibility_t::ALL:    node_visibility_combobox->setCurrentIndex (0); return;
                    case node_visibility_t::NONE:   node_visibility_combobox->setCurrentIndex (1); return;
                    case node_visibility_t::FILE:   node_visibility_combobox->setCurrentIndex (4); return;
                    case node_visibility_t::DEGREE: node_visibility_combobox->setCurrentIndex (3); return;
                  }
                }
                node_visibility = node_visibility_t::FILE;
                if (node_visibility_combobox->count() == 4)
                  node_visibility_combobox->addItem (node_values_from_file_visibility.get_name());
                else
                  node_visibility_combobox->setItemText (4, node_values_from_file_visibility.get_name());
                node_visibility_combobox->setCurrentIndex (4);
                node_visibility_threshold_label->setVisible (true);
                node_visibility_threshold_button->setVisible (true);
                node_visibility_threshold_invert_checkbox->setVisible (true);
                node_visibility_threshold_button->setRate (0.001 * (node_values_from_file_visibility.get_max() - node_values_from_file_visibility.get_min()));
                node_visibility_threshold_button->setMin (node_values_from_file_visibility.get_min());
                node_visibility_threshold_button->setMax (node_values_from_file_visibility.get_max());
                node_visibility_threshold_button->setValue (0.5 * (node_values_from_file_visibility.get_min() + node_values_from_file_visibility.get_max()));
              } catch (Exception& e) {
                e.display();
                node_values_from_file_visibility.clear();
                node_visibility_combobox->setCurrentIndex (0);
                node_visibility = node_visibility_t::ALL;
                node_visibility_combobox->removeItem (4);
                node_visibility_threshold_label->setVisible (false);
                node_visibility_threshold_button->setVisible (false);
                node_visibility_threshold_invert_checkbox->setVisible (false);
              }
              break;
            case 3:
              if (node_visibility == node_visibility_t::DEGREE) return;
              if (edge_visibility == edge_visibility_t::VISIBLE_NODES) {
                QMessageBox::warning (QApplication::activeWindow(),
                                      tr ("Visualisation error"),
                                      tr ("Cannot have node visibility based on edges; edge visibility is based on nodes!"),
                                      QMessageBox::Ok,
                                      QMessageBox::Ok);
                node_visibility_combobox->setCurrentIndex (0);
                node_visibility = node_visibility_t::ALL;
              } else {
                node_visibility = node_visibility_t::DEGREE;
              }
              node_visibility_combobox->removeItem (4);
              node_visibility_threshold_label->setVisible (false);
              node_visibility_threshold_button->setVisible (false);
              node_visibility_threshold_invert_checkbox->setVisible (false);
              break;
            case 4:
              return;
          }
          calculate_node_visibility();
          window.updateGL();
        }

        void Connectome::node_geometry_selection_slot (int index)
        {
          node_visibility_warning_icon->setVisible (false);
          switch (index) {
            case 0:
              if (node_geometry == node_geometry_t::SPHERE) return;
              node_geometry = node_geometry_t::SPHERE;
              node_size_combobox->setEnabled (true);
              node_size_button->setVisible (true);
              node_size_button->setMax (std::numeric_limits<float>::max());
              node_geometry_sphere_lod_label->setVisible (true);
              node_geometry_sphere_lod_spinbox->setVisible (true);
              node_geometry_overlay_interp_checkbox->setVisible (false);
              break;
            case 1:
              if (node_geometry == node_geometry_t::CUBE) return;
              node_geometry = node_geometry_t::CUBE;
              node_size_combobox->setEnabled (true);
              node_size_button->setVisible (true);
              node_size_button->setMax (std::numeric_limits<float>::max());
              node_geometry_sphere_lod_label->setVisible (false);
              node_geometry_sphere_lod_spinbox->setVisible (false);
              node_geometry_overlay_interp_checkbox->setVisible (false);
              break;
            case 2:
              if (node_geometry == node_geometry_t::OVERLAY) return;
              node_geometry = node_geometry_t::OVERLAY;
              node_size = node_size_t::FIXED;
              calculate_node_sizes();
              node_size_combobox->setCurrentIndex (0);
              node_size_combobox->setEnabled (false);
              node_size_button->setVisible (false);
              node_size_range_label->setVisible (false);
              node_size_lower_button->setVisible (false);
              node_size_upper_button->setVisible (false);
              node_size_invert_checkbox->setVisible (false);
              node_geometry_sphere_lod_label->setVisible (false);
              node_geometry_sphere_lod_spinbox->setVisible (false);
              node_geometry_overlay_interp_checkbox->setVisible (true);
              update_node_overlay();
              break;
            case 3:
              if (node_geometry == node_geometry_t::MESH) return;
              node_geometry = node_geometry_t::MESH;
              if (!have_meshes) {
                // Can't generate GL buffer objects in a separate thread: OpenGL context is
                //   specific to one thread only. Therefore, do the heavy work in a
                //   multi-threaded fashion (calculate_mesh()), then create the buffers themselves
                //   in the master thread (assign_mesh())
                std::vector<MR::Mesh::Mesh> meshes (num_nodes()+1, MR::Mesh::Mesh());
                auto source = [&] (uint32_t& out) { static uint32_t i = 1; out = i++; return (out <= num_nodes()); };
                std::mutex mutex;
                ProgressBar progress ("Generating node meshes... ", num_nodes());
                auto sink = [&] (uint32_t& in) { meshes[in] = nodes[in].calculate_mesh(); std::lock_guard<std::mutex> lock (mutex); ++progress; return true; };
                Thread::run_queue (source, uint32_t(), Thread::multi (sink));
                for (uint32_t i = 1; i <= num_nodes(); ++i)
                  nodes[i].assign_mesh (meshes[i]);
                have_meshes = true;
              }
              if (node_size == node_size_t::NODE_VOLUME) {
                node_size = node_size_t::FIXED;
                node_size_combobox->setCurrentIndex (0);
                calculate_node_sizes();
                node_size_range_label->setVisible (false);
                node_size_lower_button->setVisible (false);
                node_size_upper_button->setVisible (false);
                node_size_invert_checkbox->setVisible (false);
              }
              node_size_combobox->setEnabled (true);
              node_size_button->setVisible (true);
              if (node_size_scale_factor > 1.0f) {
                node_size_scale_factor = 1.0f;
                node_size_button->setValue (node_size_scale_factor);
              }
              node_size_button->setMax (1.0f);
              node_geometry_sphere_lod_label->setVisible (false);
              node_geometry_sphere_lod_spinbox->setVisible (false);
              node_geometry_overlay_interp_checkbox->setVisible (false);
              break;
            case 4:
              if (node_geometry == node_geometry_t::SMOOTH_MESH) return;
              node_geometry = node_geometry_t::SMOOTH_MESH;
              if (!have_smooth_meshes) {
                std::vector<MR::Mesh::Mesh> smooth_meshes (num_nodes()+1, MR::Mesh::Mesh());
                auto source = [&] (uint32_t& out) { static uint32_t i = 1; out = i++; return (out <= num_nodes()); };
                std::mutex mutex;
                ProgressBar progress ("Generating smooth node meshes... ", num_nodes());
                auto sink = [&] (uint32_t& in) { smooth_meshes[in] = nodes[in].calculate_smooth_mesh(); std::lock_guard<std::mutex> lock (mutex); ++progress; return true; };
                Thread::run_queue (source, uint32_t(), Thread::multi (sink));
                for (uint32_t i = 1; i <= num_nodes(); ++i)
                  nodes[i].assign_smooth_mesh (smooth_meshes[i]);
                have_smooth_meshes = true;
              }
              if (node_size == node_size_t::NODE_VOLUME) {
                node_size = node_size_t::FIXED;
                node_size_combobox->setCurrentIndex (0);
                calculate_node_sizes();
                node_size_range_label->setVisible (false);
                node_size_lower_button->setVisible (false);
                node_size_upper_button->setVisible (false);
                node_size_invert_checkbox->setVisible (false);
              }
              node_size_combobox->setEnabled (true);
              node_size_button->setVisible (true);
              if (node_size_scale_factor > 1.0f) {
                node_size_scale_factor = 1.0f;
                node_size_button->setValue (node_size_scale_factor);
              }
              node_size_button->setMax (1.0f);
              node_geometry_sphere_lod_label->setVisible (false);
              node_geometry_sphere_lod_spinbox->setVisible (false);
              node_geometry_overlay_interp_checkbox->setVisible (false);
              break;
          }
          if (node_visibility == node_visibility_t::NONE)
            node_visibility_warning_icon->setVisible (true);
          window.updateGL();
        }

        void Connectome::node_colour_selection_slot (int index)
        {
          node_visibility_warning_icon->setVisible (false);
          switch (index) {
            case 0:
              if (node_colour == node_colour_t::FIXED) return;
              node_colour = node_colour_t::FIXED;
              node_colour_colourmap_button->setVisible (false);
              node_colour_fixedcolour_button->setVisible (true);
              node_colour_combobox->removeItem (4);
              node_colour_range_label->setVisible (false);
              node_colour_lower_button->setVisible (false);
              node_colour_upper_button->setVisible (false);
              break;
            case 1:
              // Regenerate random colours on repeat selection
              node_colour = node_colour_t::RANDOM;
              node_colour_colourmap_button->setVisible (false);
              node_colour_fixedcolour_button->setVisible (false);
              node_colour_combobox->removeItem (4);
              node_colour_range_label->setVisible (false);
              node_colour_lower_button->setVisible (false);
              node_colour_upper_button->setVisible (false);
              break;
            case 2:
              if (node_colour == node_colour_t::FROM_LUT) return;
              // TODO Pointless selection if no LUT is loaded... need to detect; or better, disable
              if (lut.size()) {
                node_colour = node_colour_t::FROM_LUT;
                node_colour_fixedcolour_button->setVisible (false);
              } else {
                QMessageBox::warning (QApplication::activeWindow(),
                                      tr ("Visualisation error"),
                                      tr ("Cannot colour nodes based on a lookup table; \n"
                                          "none has been provided (use the 'LUT' combo box at the "
                                          "top of the toolbar)"),
                                      QMessageBox::Ok,
                                      QMessageBox::Ok);
                node_colour_combobox->setCurrentIndex (0);
                node_colour = node_colour_t::FIXED;
                node_colour_fixedcolour_button->setVisible (true);
              }
              node_colour_colourmap_button->setVisible (false);
              node_colour_combobox->removeItem (4);
              node_colour_range_label->setVisible (false);
              node_colour_lower_button->setVisible (false);
              node_colour_upper_button->setVisible (false);
              break;
            case 3:
              try {
                if (!import_file_for_node_property (node_values_from_file_colour, "colours")) {
                  switch (node_colour) {
                    case node_colour_t::FIXED:    node_colour_combobox->setCurrentIndex (0); return;
                    case node_colour_t::RANDOM:   node_colour_combobox->setCurrentIndex (1); return;
                    case node_colour_t::FROM_LUT: node_colour_combobox->setCurrentIndex (2); return;
                    case node_colour_t::FILE:     node_colour_combobox->setCurrentIndex (4); return;
                  }
                }
                node_colour = node_colour_t::FILE;
                node_colour_colourmap_button->setVisible (true);
                node_colour_fixedcolour_button->setVisible (false);
                if (node_colour_combobox->count() == 4)
                  node_colour_combobox->addItem (node_values_from_file_colour.get_name());
                else
                  node_colour_combobox->setItemText (4, node_values_from_file_colour.get_name());
                node_colour_combobox->setCurrentIndex (4);
                node_colour_range_label->setVisible (true);
                node_colour_lower_button->setVisible (true);
                node_colour_upper_button->setVisible (true);
                node_colour_lower_button->setValue (node_values_from_file_colour.get_min());
                node_colour_upper_button->setValue (node_values_from_file_colour.get_max());
                node_colour_lower_button->setMax (node_values_from_file_colour.get_max());
                node_colour_upper_button->setMin (node_values_from_file_colour.get_min());
                node_colour_lower_button->setRate (0.01 * (node_values_from_file_colour.get_max() - node_values_from_file_colour.get_min()));
                node_colour_upper_button->setRate (0.01 * (node_values_from_file_colour.get_max() - node_values_from_file_colour.get_min()));
              } catch (Exception& e) {
                e.display();
                node_values_from_file_colour.clear();
                node_colour_combobox->setCurrentIndex (0);
                node_colour = node_colour_t::FIXED;
                node_colour_colourmap_button->setVisible (false);
                node_colour_fixedcolour_button->setVisible (true);
                node_colour_combobox->removeItem (4);
                node_colour_range_label->setVisible (false);
                node_colour_lower_button->setVisible (false);
                node_colour_upper_button->setVisible (false);
              }
              break;
            case 4:
              return;
          }
          if (node_visibility == node_visibility_t::NONE)
            node_visibility_warning_icon->setVisible (true);
          calculate_node_colours();
          window.updateGL();
        }

        void Connectome::node_size_selection_slot (int index)
        {
          assert (node_geometry != node_geometry_t::OVERLAY);
          node_visibility_warning_icon->setVisible (false);
          switch (index) {
            case 0:
              if (node_size == node_size_t::FIXED) return;
              node_size = node_size_t::FIXED;
              node_size_combobox->removeItem (3);
              node_size_range_label->setVisible (false);
              node_size_lower_button->setVisible (false);
              node_size_upper_button->setVisible (false);
              node_size_invert_checkbox->setVisible (false);
              break;
            case 1:
              if (node_size == node_size_t::NODE_VOLUME) return;
              node_size = node_size_t::NODE_VOLUME;
              node_size_combobox->removeItem (3);
              node_size_range_label->setVisible (false);
              node_size_lower_button->setVisible (false);
              node_size_upper_button->setVisible (false);
              node_size_invert_checkbox->setVisible (false);
              break;
            case 2:
              try {
                if (!import_file_for_node_property (node_values_from_file_size, "size")) {
                  switch (node_size) {
                    case node_size_t::FIXED:       node_size_combobox->setCurrentIndex (0); return;
                    case node_size_t::NODE_VOLUME: node_size_combobox->setCurrentIndex (1); return;
                    case node_size_t::FILE:        node_size_combobox->setCurrentIndex (3); return;
                  }
                }
                node_size = node_size_t::FILE;
                if (node_size_combobox->count() == 3)
                  node_size_combobox->addItem (node_values_from_file_size.get_name());
                else
                  node_size_combobox->setItemText (3, node_values_from_file_size.get_name());
                node_size_combobox->setCurrentIndex (3);
                node_size_range_label->setVisible (true);
                node_size_lower_button->setVisible (true);
                node_size_upper_button->setVisible (true);
                node_size_invert_checkbox->setVisible (true);
                node_size_lower_button->setValue (node_values_from_file_size.get_min());
                node_size_upper_button->setValue (node_values_from_file_size.get_max());
                node_size_lower_button->setMax (node_values_from_file_size.get_max());
                node_size_upper_button->setMin (node_values_from_file_size.get_min());
                node_size_lower_button->setRate (0.01 * (node_values_from_file_size.get_max() - node_values_from_file_size.get_min()));
                node_size_upper_button->setRate (0.01 * (node_values_from_file_size.get_max() - node_values_from_file_size.get_min()));
                node_size_invert_checkbox->setChecked (false);
              } catch (Exception& e) {
                e.display();
                node_values_from_file_size.clear();
                node_size_combobox->setCurrentIndex (0);
                node_size = node_size_t::FIXED;
                node_size_combobox->removeItem (3);
                node_size_range_label->setVisible (false);
                node_size_lower_button->setVisible (false);
                node_size_upper_button->setVisible (false);
                node_size_invert_checkbox->setVisible (false);
              }
              break;
            case 3:
              return;
          }
          if (node_visibility == node_visibility_t::NONE)
            node_visibility_warning_icon->setVisible (true);
          calculate_node_sizes();
          window.updateGL();
        }

        void Connectome::node_alpha_selection_slot (int index)
        {
          node_visibility_warning_icon->setVisible (false);
          switch (index) {
            case 0:
              if (node_alpha == node_alpha_t::FIXED) return;
              node_alpha = node_alpha_t::FIXED;
              node_alpha_combobox->removeItem (3);
              node_alpha_range_label->setVisible (false);
              node_alpha_lower_button->setVisible (false);
              node_alpha_upper_button->setVisible (false);
              node_alpha_invert_checkbox->setVisible (false);
              break;
            case 1:
              if (node_alpha == node_alpha_t::FROM_LUT) return;
              if (lut.size()) {
                node_alpha = node_alpha_t::FROM_LUT;
              } else {
                QMessageBox::warning (QApplication::activeWindow(),
                                      tr ("Visualisation error"),
                                      tr ("Cannot set node transparency based on a lookup table; \n"
                                          "none has been provided (use the 'LUT' combo box at the "
                                          "top of the toolbar)"),
                                      QMessageBox::Ok,
                                      QMessageBox::Ok);
                node_alpha_combobox->setCurrentIndex (0);
                node_alpha = node_alpha_t::FIXED;
              }
              node_alpha_combobox->removeItem (3);
              node_alpha_range_label->setVisible (false);
              node_alpha_lower_button->setVisible (false);
              node_alpha_upper_button->setVisible (false);
              node_alpha_invert_checkbox->setVisible (false);
              break;
            case 2:
              try {
                if (!import_file_for_node_property (node_values_from_file_alpha, "transparency")) {
                  switch (node_alpha) {
                    case node_alpha_t::FIXED:    node_alpha_combobox->setCurrentIndex (0); return;
                    case node_alpha_t::FROM_LUT: node_alpha_combobox->setCurrentIndex (1); return;
                    case node_alpha_t::FILE:     node_alpha_combobox->setCurrentIndex (3); return;
                  }
                }
                node_alpha = node_alpha_t::FILE;
                if (node_alpha_combobox->count() == 3)
                  node_alpha_combobox->addItem (node_values_from_file_alpha.get_name());
                else
                  node_alpha_combobox->setItemText (3, node_values_from_file_alpha.get_name());
                node_alpha_combobox->setCurrentIndex (3);
                node_alpha_range_label->setVisible (true);
                node_alpha_lower_button->setVisible (true);
                node_alpha_upper_button->setVisible (true);
                node_alpha_invert_checkbox->setVisible (true);
                node_alpha_lower_button->setValue (node_values_from_file_alpha.get_min());
                node_alpha_upper_button->setValue (node_values_from_file_alpha.get_max());
                node_alpha_lower_button->setMax (node_values_from_file_alpha.get_max());
                node_alpha_upper_button->setMin (node_values_from_file_alpha.get_min());
                node_alpha_lower_button->setRate (0.01 * (node_values_from_file_alpha.get_max() - node_values_from_file_alpha.get_min()));
                node_alpha_upper_button->setRate (0.01 * (node_values_from_file_alpha.get_max() - node_values_from_file_alpha.get_min()));
                node_alpha_invert_checkbox->setChecked (false);
              } catch (Exception& e) {
                e.display();
                node_values_from_file_alpha.clear();
                node_alpha_combobox->setCurrentIndex (0);
                node_alpha = node_alpha_t::FIXED;
                node_alpha_combobox->removeItem (3);
                node_alpha_range_label->setVisible (false);
                node_alpha_lower_button->setVisible (false);
                node_alpha_upper_button->setVisible (false);
                node_alpha_invert_checkbox->setVisible (false);
              }
              break;
            case 3:
              return;
          }
          if (node_visibility == node_visibility_t::NONE)
            node_visibility_warning_icon->setVisible (true);
          calculate_node_alphas();
          window.updateGL();
        }





        void Connectome::node_visibility_parameter_slot()
        {
          calculate_node_visibility();
          window.updateGL();
        }
        void Connectome::sphere_lod_slot (int value)
        {
          sphere.LOD (value);
          node_visibility_warning_icon->setVisible (node_visibility == node_visibility_t::NONE);
          window.updateGL();
        }
        void Connectome::overlay_interp_slot (int)
        {
          assert (node_overlay);
          node_visibility_warning_icon->setVisible (node_visibility == node_visibility_t::NONE);
          node_overlay->set_interpolate (node_geometry_overlay_interp_checkbox->isChecked());
          window.updateGL();
        }
        void Connectome::node_colour_change_slot()
        {
          QColor c = node_colour_fixedcolour_button->color();
          node_fixed_colour.set (c.red() / 255.0f, c.green() / 255.0f, c.blue() / 255.0f);
          node_visibility_warning_icon->setVisible (node_visibility == node_visibility_t::NONE);
          calculate_node_colours();
          window.updateGL();
        }
        void Connectome::node_colour_parameter_slot()
        {
          node_colour_lower_button->setMax (node_colour_upper_button->value());
          node_colour_upper_button->setMin (node_colour_lower_button->value());
          calculate_node_colours();
          window.updateGL();
        }
        void Connectome::node_size_value_slot()
        {
          node_size_scale_factor = node_size_button->value();
          window.updateGL();
        }
        void Connectome::node_size_parameter_slot()
        {
          node_size_lower_button->setMax (node_size_upper_button->value());
          node_size_upper_button->setMin (node_size_lower_button->value());
          calculate_node_sizes();
          window.updateGL();
        }
        void Connectome::node_alpha_value_slot (int position)
        {
          node_fixed_alpha = position / 1000.0f;
          if (node_overlay)
            node_overlay->alpha = node_fixed_alpha;
          window.updateGL();
        }
        void Connectome::node_alpha_parameter_slot()
        {
          node_alpha_lower_button->setMax (node_alpha_upper_button->value());
          node_alpha_upper_button->setMin (node_alpha_lower_button->value());
          calculate_node_alphas();
          window.updateGL();
        }







        void Connectome::edge_visibility_selection_slot (int index)
        {
          edge_visibility_warning_icon->setVisible (false);
          switch (index) {
            case 0:
              if (edge_visibility == edge_visibility_t::ALL) return;
              edge_visibility = edge_visibility_t::ALL;
              edge_visibility_combobox->removeItem (4);
              edge_visibility_threshold_label->setVisible (false);
              edge_visibility_threshold_button->setVisible (false);
              edge_visibility_threshold_invert_checkbox->setVisible (false);
              break;
            case 1:
              if (edge_visibility == edge_visibility_t::NONE) return;
              edge_visibility = edge_visibility_t::NONE;
              edge_visibility_combobox->removeItem (4);
              edge_visibility_threshold_label->setVisible (false);
              edge_visibility_threshold_button->setVisible (false);
              edge_visibility_threshold_invert_checkbox->setVisible (false);
              break;
            case 2:
              if (edge_visibility == edge_visibility_t::VISIBLE_NODES) return;
              if (node_visibility == node_visibility_t::DEGREE) {
                QMessageBox::warning (QApplication::activeWindow(),
                                      tr ("Visualisation error"),
                                      tr ("Cannot have edge visibility based on nodes; node visibility is based on edges!"),
                                      QMessageBox::Ok,
                                      QMessageBox::Ok);
                edge_visibility_combobox->setCurrentIndex (1);
                edge_visibility = edge_visibility_t::NONE;
              } else {
                edge_visibility = edge_visibility_t::VISIBLE_NODES;
              }
              edge_visibility_combobox->removeItem (4);
              edge_visibility_threshold_label->setVisible (false);
              edge_visibility_threshold_button->setVisible (false);
              edge_visibility_threshold_invert_checkbox->setVisible (false);
              break;
            case 3:
              try {
                if (!import_file_for_edge_property (edge_values_from_file_visibility, "visibility")) {
                  switch (edge_visibility) {
                    case edge_visibility_t::ALL:           edge_visibility_combobox->setCurrentIndex (0); return;
                    case edge_visibility_t::NONE:          edge_visibility_combobox->setCurrentIndex (1); return;
                    case edge_visibility_t::VISIBLE_NODES: edge_visibility_combobox->setCurrentIndex (2); return;
                    case edge_visibility_t::FILE:          edge_visibility_combobox->setCurrentIndex (4); return;
                  }
                }
                edge_visibility = edge_visibility_t::FILE;
                if (edge_visibility_combobox->count() == 4)
                  edge_visibility_combobox->addItem (edge_values_from_file_visibility.get_name());
                else
                  edge_visibility_combobox->setItemText (4, edge_values_from_file_visibility.get_name());
                edge_visibility_combobox->setCurrentIndex (4);
                edge_visibility_threshold_label->setVisible (true);
                edge_visibility_threshold_button->setVisible (true);
                edge_visibility_threshold_invert_checkbox->setVisible (true);
                edge_visibility_threshold_button->setRate (0.001 * (edge_values_from_file_visibility.get_max() - edge_values_from_file_visibility.get_min()));
                edge_visibility_threshold_button->setMin (edge_values_from_file_visibility.get_min());
                edge_visibility_threshold_button->setMax (edge_values_from_file_visibility.get_max());
                edge_visibility_threshold_button->setValue (0.5 * (edge_values_from_file_visibility.get_min() + edge_values_from_file_visibility.get_max()));
              } catch (Exception& e) {
                e.display();
                edge_values_from_file_visibility.clear();
                edge_visibility_combobox->setCurrentIndex (1);
                edge_visibility = edge_visibility_t::NONE;
                edge_visibility_combobox->removeItem (4);
                edge_visibility_threshold_label->setVisible (false);
                edge_visibility_threshold_button->setVisible (false);
                edge_visibility_threshold_invert_checkbox->setVisible (false);
              }
              break;
            case 4:
              return;
          }
          calculate_edge_visibility();
          window.updateGL();
        }

        void Connectome::edge_geometry_selection_slot (int index)
        {
          edge_visibility_warning_icon->setVisible (false);
          switch (index) {
            case 0:
              if (edge_geometry == edge_geometry_t::LINE) return;
              edge_geometry = edge_geometry_t::LINE;
              edge_geometry_cylinder_lod_label->setVisible (false);
              edge_geometry_cylinder_lod_spinbox->setVisible (false);
              edge_geometry_line_smooth_checkbox->setVisible (true);
              break;
            case 1:
              if (edge_geometry == edge_geometry_t::CYLINDER) return;
              edge_geometry = edge_geometry_t::CYLINDER;
              edge_geometry_cylinder_lod_label->setVisible (true);
              edge_geometry_cylinder_lod_spinbox->setVisible (true);
              edge_geometry_line_smooth_checkbox->setVisible (false);
              break;
            case 2:
              try {
                if (!have_exemplars) {
                  get_exemplars();
                  if (!have_exemplars)
                    throw Exception ("No directory path provided; cannot render streamlines");
                }
                edge_geometry = edge_geometry_t::STREAMLINE;
                edge_geometry_cylinder_lod_label->setVisible (false);
                edge_geometry_cylinder_lod_spinbox->setVisible (false);
                edge_geometry_line_smooth_checkbox->setVisible (true);
              } catch (Exception& e) {
                e.display();
                for (auto i = edges.begin(); i != edges.end(); ++i)
                  i->clear_exemplar();
                have_exemplars = false;
                edge_geometry = edge_geometry_t::LINE;
                edge_geometry_combobox->setCurrentIndex (0);
                edge_geometry_cylinder_lod_label->setVisible (false);
                edge_geometry_cylinder_lod_spinbox->setVisible (false);
                edge_geometry_line_smooth_checkbox->setVisible (true);
              }
              break;
            case 3:
              try {
                if (!have_streamtubes) {
                  get_streamtubes();
                  if (!have_exemplars)
                    throw Exception ("No directory path provided; cannot render streamtubes");
                }
                edge_geometry = edge_geometry_t::STREAMTUBE;
                edge_geometry_cylinder_lod_label->setVisible (false);
                edge_geometry_cylinder_lod_spinbox->setVisible (false);
                edge_geometry_line_smooth_checkbox->setVisible (false);
              } catch (Exception& e) {
                e.display();
                for (auto i = edges.begin(); i != edges.end(); ++i)
                  i->clear_streamtube();
                have_exemplars = false;
                edge_geometry = edge_geometry_t::LINE;
                edge_geometry_combobox->setCurrentIndex (0);
                edge_geometry_cylinder_lod_label->setVisible (false);
                edge_geometry_cylinder_lod_spinbox->setVisible (false);
                edge_geometry_line_smooth_checkbox->setVisible (true);
              }
              break;
          }
          if (edge_visibility == edge_visibility_t::NONE)
            edge_visibility_warning_icon->setVisible (true);
          window.updateGL();
        }

        void Connectome::edge_colour_selection_slot (int index)
        {
          edge_visibility_warning_icon->setVisible (false);
          switch (index) {
            case 0:
              if (edge_colour == edge_colour_t::FIXED) return;
              edge_colour = edge_colour_t::FIXED;
              edge_colour_colourmap_button->setVisible (false);
              edge_colour_fixedcolour_button->setVisible (true);
              edge_colour_combobox->removeItem (3);
              edge_colour_range_label->setVisible (false);
              edge_colour_lower_button->setVisible (false);
              edge_colour_upper_button->setVisible (false);
              break;
            case 1:
              if (edge_colour == edge_colour_t::DIRECTION) return;
              edge_colour = edge_colour_t::DIRECTION;
              edge_colour_colourmap_button->setVisible (false);
              edge_colour_fixedcolour_button->setVisible (false);
              edge_colour_combobox->removeItem (3);
              edge_colour_range_label->setVisible (false);
              edge_colour_lower_button->setVisible (false);
              edge_colour_upper_button->setVisible (false);
              break;
            case 2:
              try {
                if (!import_file_for_edge_property (edge_values_from_file_colour, "colours")) {
                  switch (edge_colour) {
                    case edge_colour_t::FIXED:     edge_colour_combobox->setCurrentIndex (0); return;
                    case edge_colour_t::DIRECTION: edge_colour_combobox->setCurrentIndex (1); return;
                    case edge_colour_t::FILE:      edge_colour_combobox->setCurrentIndex (3); return;
                  }
                }
                edge_colour = edge_colour_t::FILE;
                edge_colour_colourmap_button->setVisible (true);
                edge_colour_fixedcolour_button->setVisible (false);
                if (edge_colour_combobox->count() == 3)
                  edge_colour_combobox->addItem (edge_values_from_file_colour.get_name());
                else
                  edge_colour_combobox->setItemText (3, edge_values_from_file_colour.get_name());
                edge_colour_combobox->setCurrentIndex (3);
                edge_colour_range_label->setVisible (true);
                edge_colour_lower_button->setVisible (true);
                edge_colour_upper_button->setVisible (true);
                edge_colour_lower_button->setValue (edge_values_from_file_colour.get_min());
                edge_colour_upper_button->setValue (edge_values_from_file_colour.get_max());
                edge_colour_lower_button->setMax (edge_values_from_file_colour.get_max());
                edge_colour_upper_button->setMin (edge_values_from_file_colour.get_min());
                edge_colour_lower_button->setRate (0.01 * (edge_values_from_file_colour.get_max() - edge_values_from_file_colour.get_min()));
                edge_colour_upper_button->setRate (0.01 * (edge_values_from_file_colour.get_max() - edge_values_from_file_colour.get_min()));
              } catch (Exception& e) {
                e.display();
                edge_values_from_file_colour.clear();
                edge_colour_combobox->setCurrentIndex (0);
                edge_colour = edge_colour_t::FIXED;
                edge_colour_colourmap_button->setVisible (false);
                edge_colour_fixedcolour_button->setVisible (true);
                edge_colour_combobox->removeItem (3);
                edge_colour_range_label->setVisible (false);
                edge_colour_lower_button->setVisible (false);
                edge_colour_upper_button->setVisible (false);
              }
              break;
            case 3:
              return;
          }
          if (edge_visibility == edge_visibility_t::NONE)
            edge_visibility_warning_icon->setVisible (true);
          calculate_edge_colours();
          window.updateGL();
        }

        void Connectome::edge_size_selection_slot (int index)
        {
          edge_visibility_warning_icon->setVisible (false);
          switch (index) {
            case 0:
              if (edge_size == edge_size_t::FIXED) return;
              edge_size = edge_size_t::FIXED;
              edge_size_combobox->removeItem (2);
              edge_size_range_label->setVisible (false);
              edge_size_lower_button->setVisible (false);
              edge_size_upper_button->setVisible (false);
              edge_size_invert_checkbox->setVisible (false);
              break;
            case 1:
              try {
                if (!import_file_for_edge_property (edge_values_from_file_size, "size")) {
                  switch (edge_size) {
                    case edge_size_t::FIXED: edge_size_combobox->setCurrentIndex (0); return;
                    case edge_size_t::FILE:  edge_size_combobox->setCurrentIndex (2); return;
                  }
                }
                edge_size = edge_size_t::FILE;
                if (edge_size_combobox->count() == 2)
                  edge_size_combobox->addItem (edge_values_from_file_size.get_name());
                else
                  edge_size_combobox->setItemText (2, edge_values_from_file_size.get_name());
                edge_size_combobox->setCurrentIndex (2);
                edge_size_range_label->setVisible (true);
                edge_size_lower_button->setVisible (true);
                edge_size_upper_button->setVisible (true);
                edge_size_invert_checkbox->setVisible (true);
                edge_size_lower_button->setValue (edge_values_from_file_size.get_min());
                edge_size_upper_button->setValue (edge_values_from_file_size.get_max());
                edge_size_lower_button->setMax (edge_values_from_file_size.get_max());
                edge_size_upper_button->setMin (edge_values_from_file_size.get_min());
                edge_size_lower_button->setRate (0.01 * (edge_values_from_file_size.get_max() - edge_values_from_file_size.get_min()));
                edge_size_upper_button->setRate (0.01 * (edge_values_from_file_size.get_max() - edge_values_from_file_size.get_min()));
              } catch (Exception& e) {
                e.display();
                edge_values_from_file_size.clear();
                edge_size_combobox->setCurrentIndex (0);
                edge_size = edge_size_t::FIXED;
                edge_size_combobox->removeItem (2);
                edge_size_range_label->setVisible (false);
                edge_size_lower_button->setVisible (false);
                edge_size_upper_button->setVisible (false);
                edge_size_invert_checkbox->setVisible (false);
              }
              break;
            case 2:
              return;
          }
          if (edge_visibility == edge_visibility_t::NONE)
            edge_visibility_warning_icon->setVisible (true);
          calculate_edge_sizes();
          window.updateGL();
        }

        void Connectome::edge_alpha_selection_slot (int index)
        {
          edge_visibility_warning_icon->setVisible (false);
          switch (index) {
            case 0:
              if (edge_alpha == edge_alpha_t::FIXED) return;
              edge_alpha = edge_alpha_t::FIXED;
              edge_alpha_combobox->removeItem (2);
              edge_alpha_range_label->setVisible (false);
              edge_alpha_lower_button->setVisible (false);
              edge_alpha_upper_button->setVisible (false);
              edge_alpha_invert_checkbox->setVisible (false);
              break;
            case 1:
              try {
                if (!import_file_for_edge_property (edge_values_from_file_alpha, "transparency")) {
                  switch (edge_alpha) {
                    case edge_alpha_t::FIXED: edge_alpha_combobox->setCurrentIndex (0); return;
                    case edge_alpha_t::FILE:  edge_alpha_combobox->setCurrentIndex (2); return;
                  }
                }
                edge_alpha = edge_alpha_t::FILE;
                if (edge_alpha_combobox->count() == 2)
                  edge_alpha_combobox->addItem (edge_values_from_file_alpha.get_name());
                else
                  edge_alpha_combobox->setItemText (2, edge_values_from_file_alpha.get_name());
                edge_alpha_combobox->setCurrentIndex (2);
                edge_alpha_range_label->setVisible (true);
                edge_alpha_lower_button->setVisible (true);
                edge_alpha_upper_button->setVisible (true);
                edge_alpha_invert_checkbox->setVisible (true);
                edge_alpha_lower_button->setValue (edge_values_from_file_alpha.get_min());
                edge_alpha_upper_button->setValue (edge_values_from_file_alpha.get_max());
                edge_alpha_lower_button->setMax (edge_values_from_file_alpha.get_max());
                edge_alpha_upper_button->setMin (edge_values_from_file_alpha.get_min());
                edge_alpha_lower_button->setRate (0.01 * (edge_values_from_file_alpha.get_max() - edge_values_from_file_alpha.get_min()));
                edge_alpha_upper_button->setRate (0.01 * (edge_values_from_file_alpha.get_max() - edge_values_from_file_alpha.get_min()));
                edge_alpha_invert_checkbox->setChecked (false);
              } catch (Exception& e) {
                e.display();
                edge_values_from_file_alpha.clear();
                edge_alpha_combobox->setCurrentIndex (0);
                edge_alpha = edge_alpha_t::FIXED;
                edge_alpha_combobox->removeItem (2);
                edge_alpha_range_label->setVisible (false);
                edge_alpha_lower_button->setVisible (false);
                edge_alpha_upper_button->setVisible (false);
                edge_alpha_invert_checkbox->setVisible (false);
              }
              break;
            case 2:
              return;
          }
          if (edge_visibility == edge_visibility_t::NONE)
            edge_visibility_warning_icon->setVisible (true);
          calculate_edge_alphas();
          window.updateGL();
        }





        void Connectome::edge_visibility_parameter_slot()
        {
          calculate_edge_visibility();
          window.updateGL();
        }
        void Connectome::cylinder_lod_slot (int index)
        {
          cylinder.LOD (index);
          edge_visibility_warning_icon->setVisible (edge_visibility == edge_visibility_t::NONE);
          window.updateGL();
        }
        void Connectome::edge_colour_change_slot()
        {
          QColor c = edge_colour_fixedcolour_button->color();
          edge_fixed_colour.set (c.red() / 255.0f, c.green() / 255.0f, c.blue() / 255.0f);
          edge_visibility_warning_icon->setVisible (edge_visibility == edge_visibility_t::NONE);
          calculate_edge_colours();
          window.updateGL();
        }
        void Connectome::edge_colour_parameter_slot()
        {
          calculate_edge_colours();
          window.updateGL();
        }
        void Connectome::edge_size_value_slot()
        {
          edge_size_scale_factor = edge_size_button->value();
          window.updateGL();
        }
        void Connectome::edge_size_parameter_slot()
        {
          calculate_edge_sizes();
          window.updateGL();
        }
        void Connectome::edge_alpha_value_slot (int position)
        {
          edge_fixed_alpha = position / 1000.0f;
          window.updateGL();
        }
        void Connectome::edge_alpha_parameter_slot()
        {
          calculate_edge_alphas();
          window.updateGL();
        }










        void Connectome::clear_all()
        {
          image_button ->setText ("");
          lut_combobox->removeItem (5);
          lut_combobox->setCurrentIndex (0);
          config_button->setText ("(none)");
          if (node_visibility == node_visibility_t::FILE) {
            node_visibility_combobox->removeItem (5);
            node_visibility_combobox->setCurrentIndex (0);
            node_visibility = node_visibility_t::ALL;
          }
          if (node_colour == node_colour_t::FILE) {
            node_colour_combobox->removeItem (4);
            node_colour_combobox->setCurrentIndex (0);
            node_colour = node_colour_t::FIXED;
          }
          if (node_size == node_size_t::FILE) {
            node_size_combobox->removeItem (3);
            node_size_combobox->setCurrentIndex (0);
            node_size = node_size_t::FIXED;
          }
          if (node_alpha == node_alpha_t::FILE) {
            node_alpha_combobox->removeItem (3);
            node_alpha_combobox->setCurrentIndex (0);
            node_alpha = node_alpha_t::FIXED;
          }
          if (edge_visibility == edge_visibility_t::FILE) {
            edge_visibility_combobox->removeItem (4);
            edge_visibility_combobox->setCurrentIndex (1);
            edge_visibility = edge_visibility_t::NONE;
          }
          if (edge_colour == edge_colour_t::FILE) {
            edge_colour_combobox->removeItem (3);
            edge_colour_combobox->setCurrentIndex (0);
            edge_colour = edge_colour_t::FIXED;
          }
          if (edge_size == edge_size_t::FILE) {
            edge_size_combobox->removeItem (2);
            edge_size_combobox->setCurrentIndex (0);
            edge_size = edge_size_t::FIXED;
          }
          if (edge_alpha == edge_alpha_t::FILE) {
            edge_alpha_combobox->removeItem (2);
            edge_alpha_combobox->setCurrentIndex (0);
            edge_alpha = edge_alpha_t::FIXED;
          }
          if (buffer)
            delete buffer.release();
          nodes.clear();
          edges.clear();
          lut.clear();
          config.clear();
          lut_mapping.clear();
          if (node_overlay)
            delete node_overlay.release();
          node_values_from_file_visibility.clear();
          node_values_from_file_colour.clear();
          node_values_from_file_size.clear();
          node_values_from_file_alpha.clear();
          edge_values_from_file_visibility.clear();
          edge_values_from_file_colour.clear();
          edge_values_from_file_size.clear();
          edge_values_from_file_alpha.clear();
          node_visibility_warning_icon->setVisible (false);
          edge_visibility_warning_icon->setVisible (false);
        }

        void Connectome::enable_all (const bool value)
        {
          lut_combobox->setEnabled (value);
          config_button->setEnabled (value);

          lighting_checkbox->setEnabled (value);
          lighting_settings_button->setEnabled (value);
          dimensionality_combobox->setEnabled (value);

          node_visibility_combobox->setEnabled (value);
          node_visibility_threshold_button->setEnabled (value);
          node_visibility_threshold_invert_checkbox->setEnabled (value);

          node_geometry_combobox->setEnabled (value);
          node_geometry_sphere_lod_spinbox->setEnabled (value);
          node_geometry_overlay_interp_checkbox->setEnabled (value);

          node_colour_combobox->setEnabled (value);
          node_colour_fixedcolour_button->setEnabled (value);
          node_colour_colourmap_button->setEnabled (value);
          node_colour_lower_button->setEnabled (value);
          node_colour_upper_button->setEnabled (value);

          node_size_combobox->setEnabled (value);
          node_size_button->setEnabled (value);
          node_size_lower_button->setEnabled (value);
          node_size_upper_button->setEnabled (value);
          node_size_invert_checkbox->setEnabled (value);

          node_alpha_combobox->setEnabled (value);
          node_alpha_slider->setEnabled (value);
          node_alpha_lower_button->setEnabled (value);
          node_alpha_upper_button->setEnabled (value);
          node_alpha_invert_checkbox->setEnabled (value);

          edge_visibility_combobox->setEnabled (value);
          edge_visibility_warning_icon->setEnabled (value);
          edge_visibility_threshold_button->setEnabled (value);
          edge_visibility_threshold_invert_checkbox->setEnabled (value);

          edge_geometry_combobox->setEnabled (value);
          edge_geometry_cylinder_lod_spinbox->setEnabled (value);
          edge_geometry_line_smooth_checkbox->setEnabled (value);

          edge_colour_combobox->setEnabled (value);
          edge_colour_fixedcolour_button->setEnabled (value);
          edge_colour_colourmap_button->setEnabled (value);
          edge_colour_lower_button->setEnabled (value);
          edge_colour_upper_button->setEnabled (value);

          edge_size_combobox->setEnabled (value);
          edge_size_button->setEnabled (value);
          edge_size_lower_button->setEnabled (value);
          edge_size_upper_button->setEnabled (value);
          edge_size_invert_checkbox->setEnabled (value);

          edge_alpha_combobox->setEnabled (value);
          edge_alpha_slider->setEnabled (value);
          edge_alpha_lower_button->setEnabled (value);
          edge_alpha_upper_button->setEnabled (value);
          edge_alpha_invert_checkbox->setEnabled (value);
        }

        void Connectome::initialise (const std::string& path)
        {

          MR::Image::Header H (path);
          if (!H.datatype().is_integer())
            throw Exception ("Input parcellation image must have an integer datatype");
          if (H.ndim() != 3)
            throw Exception ("Input parcellation image must be a 3D image");
          voxel_volume = H.vox(0) * H.vox(1) * H.vox(2);
          buffer.reset (new MR::Image::BufferPreload<node_t> (path));
          auto voxel = buffer->voxel();
          MR::Image::Transform transform (H);
          std::vector< Point<float> > node_coms;
          std::vector<size_t> node_volumes;
          std::vector< Point<int> > node_lower_corners, node_upper_corners;
          size_t max_index = 0;

          {
            MR::Image::LoopInOrder loop (voxel, "Importing parcellation image... ");
            for (loop.start (voxel); loop.ok(); loop.next (voxel)) {
              const node_t node_index = voxel.value();
              if (node_index) {

                if (node_index >= max_index) {
                  node_coms         .resize (node_index+1, Point<float> (0.0f, 0.0f, 0.0f));
                  node_volumes      .resize (node_index+1, 0);
                  node_lower_corners.resize (node_index+1, Point<int> (H.dim(0), H.dim(1), H.dim(2)));
                  node_upper_corners.resize (node_index+1, Point<int> (-1, -1, -1));
                  max_index = node_index;
                }

                node_coms   [node_index] += transform.voxel2scanner (voxel);
                node_volumes[node_index]++;

                for (size_t axis = 0; axis != 3; ++axis) {
                  node_lower_corners[node_index][axis] = std::min (node_lower_corners[node_index][axis], int(voxel[axis]));
                  node_upper_corners[node_index][axis] = std::max (node_upper_corners[node_index][axis], int(voxel[axis]));
                }

              }
            }
          }
          for (node_t n = 1; n <= max_index; ++n)
            node_coms[n] *= (1.0f / float(node_volumes[n]));

          nodes.clear();

          // TODO Multi-thread this
          {
            MR::ProgressBar progress ("Constructing nodes...", max_index);
            nodes.push_back (Node());
            for (size_t node_index = 1; node_index <= max_index; ++node_index) {
              if (node_volumes[node_index]) {

                MR::Image::Adapter::Subset<decltype(voxel)> subset (voxel, node_lower_corners[node_index], node_upper_corners[node_index] - node_lower_corners[node_index] + Point<int> (1, 1, 1));

                std::shared_ptr< MR::Image::BufferScratch<bool> > node_mask (new MR::Image::BufferScratch<bool> (subset.info(), "Node " + str(node_index) + " mask"));
                auto voxel = node_mask->voxel();

                auto copy_func = [&] (const decltype(subset)& in, decltype(voxel)& out) { out.value() = (in.value() == node_index); };
                MR::Image::ThreadedLoop (subset).run (copy_func, subset, voxel);

                nodes.push_back (Node (node_coms[node_index], node_volumes[node_index], node_mask));

              } else {
                nodes.push_back (Node());
              }
              ++progress;
            }
          }

          mat2vec = MR::Connectome::Mat2Vec (num_nodes());

          edges.clear();
          edges.reserve (mat2vec.vec_size());
          for (size_t edge_index = 0; edge_index != mat2vec.vec_size(); ++edge_index) {
            const node_t one = mat2vec(edge_index).first + 1;
            const node_t two = mat2vec(edge_index).second + 1;
            edges.push_back (Edge (one, two, nodes[one].get_com(), nodes[two].get_com()));
          }

          // Construct the node overlay image
          MR::Image::Info overlay_info (H.info());
          overlay_info.set_ndim (4);
          overlay_info.dim (3) = 4; // RGBA
          overlay_info.stride (3) = 0;
          overlay_info.sanitise();
          node_overlay.reset (new NodeOverlay (overlay_info));
          update_node_overlay();

        }






        bool Connectome::import_file_for_node_property (FileDataVector& data, const std::string& attribute)
        {
          const std::string path = Dialog::File::get_file (this, "Select vector file to determine node " + attribute);
          if (path.empty())
            return false;
          data.clear();
          data.load (path);
          const size_t numel = data.size();
          if (data.size() != num_nodes()) {
            data.clear();
            throw Exception ("File " + Path::basename (path) + " contains " + str (numel) + " elements, but connectome has " + str(num_nodes()) + " nodes");
          }
          data.set_name (Path::basename (path));
          return true;
        }

        bool Connectome::import_file_for_edge_property (FileDataVector& data, const std::string& attribute)
        {
          const std::string path = Dialog::File::get_file (this, "Select matrix file to determine edge " + attribute);
          if (path.empty())
            return false;
          Math::Matrix<float> temp (path);
          MR::Connectome::verify_matrix (temp, num_nodes());
          data.clear();
          mat2vec (temp, data);
          data.calc_minmax();
          data.set_name (Path::basename (path));
          return true;
        }






        void Connectome::load_properties()
        {
          lut_mapping.clear();
          if (lut.size()) {

            lut_mapping.push_back (lut.end());
            for (size_t node_index = 1; node_index <= num_nodes(); ++node_index) {

              if (config.size()) {
                const std::string name = config[node_index];
                nodes[node_index].set_name (name);
                Node_map::const_iterator it;
                for (it = lut.begin(); it != lut.end() && it->second.get_name() != name; ++it);
                lut_mapping.push_back (it);

              } else { // LUT, but no config file

                const auto it = lut.find (node_index);
                if (it == lut.end())
                  nodes[node_index].set_name ("Node " + str(node_index));
                else
                  nodes[node_index].set_name (it->second.get_name());
                lut_mapping.push_back (it);

              }

            } // End looping over all nodes when LUT is present

          } else { // No LUT; just name nodes according to their indices

            lut_mapping.assign (num_nodes()+1, lut.end());
            for (size_t node_index = 1; node_index <= num_nodes(); ++node_index)
              nodes[node_index].set_name ("Node " + str(node_index));

          }

          calculate_node_visibility();
          calculate_node_colours();
          calculate_node_sizes();
          calculate_node_alphas();

          calculate_edge_visibility();
          calculate_edge_colours();
          calculate_edge_sizes();
          calculate_edge_alphas();

        }




        void Connectome::calculate_node_visibility()
        {
          if (node_visibility == node_visibility_t::ALL) {

            for (auto i = nodes.begin(); i != nodes.end(); ++i)
              i->set_visible (true);

          } else if (node_visibility == node_visibility_t::NONE) {

            for (auto i = nodes.begin(); i != nodes.end(); ++i)
              i->set_visible (false);

          } else if (node_visibility == node_visibility_t::FILE) {

            assert (node_values_from_file_visibility.size());
            const bool invert = node_visibility_threshold_invert_checkbox->isChecked();
            const float threshold = node_visibility_threshold_button->value();
            for (size_t i = 1; i <= num_nodes(); ++i) {
              const bool above_threshold = (node_values_from_file_visibility[i-1] >= threshold);
              nodes[i].set_visible (above_threshold != invert);
            }

          } else if (node_visibility == node_visibility_t::DEGREE) {

            for (auto i = nodes.begin(); i != nodes.end(); ++i)
              i->set_visible (false);
            for (auto i = edges.begin(); i != edges.end(); ++i) {
              if (i->is_visible()) {
                nodes[i->get_node_index(0)].set_visible (true);
                nodes[i->get_node_index(1)].set_visible (true);
              }
            }

          }
          update_node_overlay();
          if (edge_visibility == edge_visibility_t::VISIBLE_NODES)
            calculate_edge_visibility();
        }

        void Connectome::calculate_node_colours()
        {
          if (node_colour == node_colour_t::FIXED) {

            for (auto i = nodes.begin(); i != nodes.end(); ++i)
              i->set_colour (node_fixed_colour);

          } else if (node_colour == node_colour_t::RANDOM) {

            Point<float> rgb;
            Math::RNG::Uniform<float> rng;
            for (auto i = nodes.begin(); i != nodes.end(); ++i) {
              do {
                rgb.set (rng(), rng(), rng());
              } while (rgb[0] < 0.5 && rgb[1] < 0.5 && rgb[2] < 0.5);
              i->set_colour (rgb);
            }

          } else if (node_colour == node_colour_t::FROM_LUT) {

            assert (lut.size());
            for (size_t node_index = 1; node_index <= num_nodes(); ++node_index) {
              if (lut_mapping[node_index] == lut.end())
                nodes[node_index].set_colour (node_fixed_colour);
              else
                nodes[node_index].set_colour (Point<float> (lut_mapping[node_index]->second.get_colour()) / 255.0f);
            }

          } else if (node_colour == node_colour_t::FILE) {

            assert (node_values_from_file_colour.size());
            const float lower = node_colour_lower_button->value(), upper = node_colour_upper_button->value();
            for (size_t i = 1; i <= num_nodes(); ++i) {
              float factor = (node_values_from_file_colour[i-1]-lower) / (upper - lower);
              factor = std::min (1.0f, std::max (factor, 0.0f));
              factor = node_colourmap_invert ? 1.0f-factor : factor;
              nodes[i].set_colour (Point<float> (factor, 0.0f, 0.0f));
            }

          }
          update_node_overlay();
        }

        void Connectome::calculate_node_sizes()
        {
          if (node_size == node_size_t::FIXED) {

            for (auto i = nodes.begin(); i != nodes.end(); ++i)
              i->set_size (1.0f);

          } else if (node_size == node_size_t::NODE_VOLUME) {

            for (auto i = nodes.begin(); i != nodes.end(); ++i)
              i->set_size (voxel_volume * std::cbrt (i->get_volume() / (4.0 * Math::pi)));

          } else if (node_size == node_size_t::FILE) {

            assert (node_values_from_file_size.size());
            const float lower = node_size_lower_button->value(), upper = node_size_upper_button->value();
            const bool invert = node_size_invert_checkbox->isChecked();
            for (size_t i = 1; i <= num_nodes(); ++i) {
              float factor = (node_values_from_file_size[i-1]-lower) / (upper - lower);
              factor = std::min (1.0f, std::max (factor, 0.0f));
              factor = invert ? 1.0f-factor : factor;
              nodes[i].set_size (factor);
            }

          }
        }

        void Connectome::calculate_node_alphas()
        {
          if (node_alpha == node_alpha_t::FIXED) {

            for (auto i = nodes.begin(); i != nodes.end(); ++i)
              i->set_alpha (1.0f);

          } else if (node_alpha == node_alpha_t::FROM_LUT) {

            assert (lut.size());
            for (size_t node_index = 1; node_index <= num_nodes(); ++node_index) {
              if (lut_mapping[node_index] == lut.end())
                nodes[node_index].set_alpha (node_fixed_alpha);
              else
                nodes[node_index].set_alpha (lut_mapping[node_index]->second.get_alpha() / 255.0f);
            }

          } else if (node_alpha == node_alpha_t::FILE) {

            assert (node_values_from_file_alpha.size());
            const float lower = node_alpha_lower_button->value(), upper = node_alpha_upper_button->value();
            const bool invert = node_alpha_invert_checkbox->isChecked();
            for (size_t i = 1; i <= num_nodes(); ++i) {
              float factor = (node_values_from_file_alpha[i-1]-lower) / (upper - lower);
              factor = std::min (1.0f, std::max (factor, 0.0f));
              factor = invert ? 1.0f-factor : factor;
              nodes[i].set_alpha (factor);
            }

          }
          update_node_overlay();
        }










        void Connectome::update_node_overlay()
        {
          assert (buffer);
          assert (node_overlay);
          auto in = buffer->voxel();
          auto out = node_overlay->voxel();
          if (node_geometry == node_geometry_t::OVERLAY) {
            // TODO Multi-thread this
            // Do NOT put a progress message here; causes an updateGL() call, which
            //   loads the texture even though the scratch buffer hasn't been filled yet...
            MR::Image::LoopInOrder loop (in);
            for (loop.start (in, out); loop.ok(); loop.next (in, out)) {
              const node_t node_index = in.value();
              if (node_index) {
                assert (node_index <= num_nodes());
                if (nodes[node_index].is_visible()) {
                  const Point<float>& colour (nodes[node_index].get_colour());
                  for (out[3] = 0; out[3] != 3; ++out[3])
                    out.value() = colour[int(out[3])];
                  out.value() = nodes[node_index].get_alpha();
                } else {
                  for (out[3] = 0; out[3] != 4; ++out[3])
                    out.value() = 0.0f;
                }
              }
            }
          }
        }













        void Connectome::calculate_edge_visibility()
        {
          if (edge_visibility == edge_visibility_t::ALL) {

            for (auto i = edges.begin(); i != edges.end(); ++i)
              i->set_visible (!i->is_diagonal());

          } else if (edge_visibility == edge_visibility_t::NONE) {

            for (auto i = edges.begin(); i != edges.end(); ++i)
              i->set_visible (false);

          } else if (edge_visibility == edge_visibility_t::VISIBLE_NODES) {

            for (auto i = edges.begin(); i != edges.end(); ++i)
              i->set_visible (!i->is_diagonal() && nodes[i->get_node_index(0)].is_visible() && nodes[i->get_node_index(1)].is_visible());

          } else if (edge_visibility == edge_visibility_t::FILE) {

            assert (edge_values_from_file_visibility.size());
            const bool invert = edge_visibility_threshold_invert_checkbox->isChecked();
            const float threshold = edge_visibility_threshold_button->value();
            for (size_t i = 0; i != num_edges(); ++i) {
              if (edges[i].is_diagonal()) {
                edges[i].set_visible (false);
              } else {
                const bool above_threshold = (edge_values_from_file_visibility[i] >= threshold);
                edges[i].set_visible (above_threshold != invert);
              }
            }

          }
          if (node_visibility == node_visibility_t::DEGREE)
            calculate_node_visibility();
        }

        void Connectome::calculate_edge_colours()
        {
          if (edge_colour == edge_colour_t::FIXED) {

            for (auto i = edges.begin(); i != edges.end(); ++i)
              i->set_colour (edge_fixed_colour);

          } else if (edge_colour == edge_colour_t::DIRECTION) {

            for (auto i = edges.begin(); i != edges.end(); ++i)
              i->set_colour (Point<float> (std::abs (i->get_dir()[0]), std::abs (i->get_dir()[1]), std::abs (i->get_dir()[2])));

          } else if (edge_colour == edge_colour_t::FILE) {

            assert (edge_values_from_file_colour.size());
            const float lower = edge_colour_lower_button->value(), upper = edge_colour_upper_button->value();
            for (size_t i = 0; i != num_edges(); ++i) {
              float factor = (edge_values_from_file_colour[i]-lower) / (upper - lower);
              factor = std::min (1.0f, std::max (factor, 0.0f));
              factor = edge_colourmap_invert ? 1.0f-factor : factor;
              edges[i].set_colour (Point<float> (factor, 0.0f, 0.0f));
            }

          }
        }

        void Connectome::calculate_edge_sizes()
        {
          if (edge_size == edge_size_t::FIXED) {

            for (auto i = edges.begin(); i != edges.end(); ++i)
              i->set_size (1.0f);

          } else if (edge_size == edge_size_t::FILE) {

            assert (edge_values_from_file_size.size());
            const float lower = edge_size_lower_button->value(), upper = edge_size_upper_button->value();
            const bool invert = edge_size_invert_checkbox->isChecked();
            for (size_t i = 0; i != num_edges(); ++i) {
              float factor = (edge_values_from_file_size[i]-lower) / (upper - lower);
              factor = std::min (1.0f, std::max (factor, 0.0f));
              factor = invert ? 1.0f-factor : factor;
              edges[i].set_size (factor);
            }

          }
        }

        void Connectome::calculate_edge_alphas()
        {
          if (edge_alpha == edge_alpha_t::FIXED) {

            for (auto i = edges.begin(); i != edges.end(); ++i)
              i->set_alpha (1.0f);

          } else if (edge_alpha == edge_alpha_t::FILE) {

            assert (edge_values_from_file_alpha.size());
            const float lower = edge_alpha_lower_button->value(), upper = edge_alpha_upper_button->value();
            const bool invert = edge_alpha_invert_checkbox->isChecked();
            for (size_t i = 0; i != num_edges(); ++i) {
              float factor = (edge_values_from_file_alpha[i]-lower) / (upper - lower);
              factor = std::min (1.0f, std::max (factor, 0.0f));
              factor = invert ? 1.0f-factor : factor;
              edges[i].set_alpha (factor);
            }

          }
        }






        void Connectome::get_exemplars()
        {
          // Request directory path from the user
          const std::string dir = GUI::Dialog::File::get_folder (this, "Select directory where command tcknodeextract has generated its output");
          if (!dir.size()) return;
          // Build a vector of track file paths, and verify their presence
          std::vector<std::string> paths (num_edges(), std::string());
          for (size_t edge_index = 0; edge_index != num_edges(); ++edge_index) {
            std::pair<node_t, node_t> node_indices = mat2vec (edge_index);
            if (node_indices.first != node_indices.second) {
              ++node_indices.first; ++node_indices.second; // Compensate for node 1 appearing at index 1
              const std::string basename = str(node_indices.first) + "-" + str(node_indices.second) + ".tck";
              const std::string expected = MR::Path::join (dir, basename);
              if (!MR::Path::exists (expected))
                throw Exception ("Missing track file: " + basename);
              paths[edge_index] = expected;
            }
          }
          auto source = [&] (uint32_t& out) { static uint32_t i = 0; out = i++; return (out != num_edges()); };
          std::mutex mutex;
          ProgressBar progress ("Generating connection exemplars... ", num_edges());
          auto sink = [&] (uint32_t& in) { edges[in].calculate_exemplar (paths[in]); std::lock_guard<std::mutex> lock (mutex); ++progress; return true; };
          Thread::run_queue (source, uint32_t(), Thread::multi (sink));
          for (auto i = edges.begin(); i != edges.end(); ++i)
            i->create_streamline();
          have_exemplars = true;
        }

        void Connectome::get_streamtubes()
        {
          if (!have_exemplars)
            get_exemplars();
          ProgressBar progress ("Generating connection streamtubes... ", num_edges());
          for (auto i = edges.begin(); i != edges.end(); ++i) {
            i->create_streamtube();
            ++progress;
          }
          have_streamtubes = true;
        }





        bool Connectome::use_lighting() const
        {
          return lighting_checkbox->isChecked();
        }




      }
    }
  }
}





