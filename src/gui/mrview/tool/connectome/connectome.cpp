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

#include "gui/mrview/tool/connectome/connectome.h"

#include "header.h"
#include "transform.h"

#include "adapter/subset.h"
#include "algo/loop.h"
#include "algo/threaded_loop.h"
#include "file/path.h"
#include "gui/dialog/file.h"
#include "gui/mrview/colourmap.h"
#include "math/math.h"
#include "math/rng.h"

#include "dwi/tractography/file.h"
#include "dwi/tractography/properties.h"

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






        Connectome::Connectome (Dock* parent) :
            Base (parent),
            mat2vec (0),
            lighting (this),
            lighting_dock (nullptr),
            node_list (new Tool::Dock ("Connectome node list")),
            is_3D (true),
            crop_to_slab (false),
            slab_thickness (0.0f),
            show_node_colour_bar (true),
            show_edge_colour_bar (true),
            node_visibility (node_visibility_t::ALL),
            node_geometry (node_geometry_t::SPHERE),
            node_colour (node_colour_t::FIXED),
            node_size (node_size_t::FIXED),
            node_alpha (node_alpha_t::FIXED),
            selected_nodes (0),
            selected_node_count (0),
            node_selection_settings (),
            have_meshes (false),
            node_visibility_matrix_operator (node_visibility_matrix_operator_t::ANY),
            node_colour_matrix_operator (node_property_matrix_operator_t::SUM),
            node_size_matrix_operator (node_property_matrix_operator_t::SUM),
            node_alpha_matrix_operator (node_property_matrix_operator_t::SUM),
            node_fixed_colour { 0.5f, 0.5f, 0.5f },
            node_colourmap_index (1),
            node_colourmap_invert (false),
            node_fixed_alpha (1.0f),
            node_size_scale_factor (1.0f),
            voxel_volume (0.0f),
            edge_visibility (edge_visibility_t::CONNECTOME),
            edge_geometry (edge_geometry_t::LINE),
            edge_colour (edge_colour_t::CONNECTOME),
            edge_size (edge_size_t::FIXED),
            edge_alpha (edge_alpha_t::FIXED),
            have_exemplars (false),
            edge_fixed_colour { 0.5f, 0.5f, 0.5f },
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

          group_box = new QGroupBox ("Connectome matrices");
          main_box->addWidget (group_box);
          vlayout = new VBoxLayout;
          group_box->setLayout (vlayout);

          hlayout = new HBoxLayout;
          matrix_open_button = new QPushButton (this);
          matrix_open_button->setToolTip (tr ("Open connectome file(s)"));
          matrix_open_button->setIcon (QIcon (":/open.svg"));
          connect (matrix_open_button, SIGNAL (clicked()), this, SLOT (matrix_open_slot ()));
          hlayout->addWidget (matrix_open_button, 1);
          matrix_close_button = new QPushButton (this);
          matrix_close_button->setToolTip (tr ("Close connectome file(s)"));
          matrix_close_button->setIcon (QIcon (":/close.svg"));
          connect (matrix_close_button, SIGNAL (clicked()), this, SLOT (matrix_close_slot ()));
          hlayout->addWidget (matrix_close_button, 1);
          vlayout->addLayout (hlayout);

          matrix_list_view = new QListView (this);
          matrix_list_view->setSelectionMode (QAbstractItemView::SingleSelection);
          matrix_list_view->setDragEnabled (true);
          matrix_list_view->viewport()->setAcceptDrops (true);
          matrix_list_view->setDropIndicatorShown (true);
          matrix_list_model = new Matrix_list_model (this);
          matrix_list_view->setModel (matrix_list_model);
          vlayout->addWidget (matrix_list_view);

          connect (matrix_list_view->selectionModel(),
              SIGNAL(selectionChanged (const QItemSelection &, const QItemSelection &)),
              SLOT (connectome_selection_changed_slot (const QItemSelection &, const QItemSelection &)) );

          group_box = new QGroupBox ("Node visualisation");
          main_box->addWidget (group_box);
          GridLayout* gridlayout = new GridLayout();
          group_box->setLayout (gridlayout);

          QLabel* label = new QLabel ("Visibility: ");
          gridlayout->addWidget (label, 0, 0, 1, 2);
          node_visibility_combobox = new QComboBox (this);
          node_visibility_combobox->setToolTip (tr ("Set which nodes are visible"));
          node_visibility_combobox->addItem ("All");
          node_visibility_combobox->addItem ("None");
          node_visibility_combobox->addItem ("Degree >= 1");
          node_visibility_combobox->addItem ("Connectome");
          node_visibility_combobox->addItem ("Vector file");
          node_visibility_combobox->addItem ("Matrix file");
          connect (node_visibility_combobox, SIGNAL (activated(int)), this, SLOT (node_visibility_selection_slot (int)));
          gridlayout->addWidget (node_visibility_combobox, 0, 2);
          hlayout = new HBoxLayout;
          hlayout->setContentsMargins (0, 0, 0, 0);
          hlayout->setSpacing (0);
          QIcon warning_icon (":/warn.svg");
          node_visibility_matrix_operator_combobox = new QComboBox (this);
          node_visibility_matrix_operator_combobox->setToolTip (tr ("If node visibility is determined from a matrix file, and multiple\n"
                                                                    "nodes are selected, this operator defines which nodes are visible\n"
                                                                    "and which are not based on the corresponding rows of the matrix."));
          node_visibility_matrix_operator_combobox->addItem ("Any");
          node_visibility_matrix_operator_combobox->addItem ("All");
          node_visibility_matrix_operator_combobox->addItem ("N/A");
          node_visibility_matrix_operator_combobox->setVisible (false);
          node_visibility_matrix_operator_combobox->setEnabled (false);
          connect (node_visibility_matrix_operator_combobox, SIGNAL (activated(int)), this, SLOT (node_visibility_matrix_operator_slot (int)));
          hlayout->addWidget (node_visibility_matrix_operator_combobox);
          node_visibility_warning_icon = new QLabel();
          node_visibility_warning_icon->setPixmap (warning_icon.pixmap (node_visibility_combobox->height(), Qt::KeepAspectRatio));
          node_visibility_warning_icon->setScaledContents (true);
          node_visibility_warning_icon->setToolTip ("Changes to node visualisation will have no apparent effect if node visibility is set to \'none\'");
          node_visibility_warning_icon->setVisible (false);
          hlayout->addWidget (node_visibility_warning_icon);
          gridlayout->addLayout (hlayout, 0, 3, 1, 2);

          node_visibility_threshold_controls = new QWidget (this);
          hlayout = new HBoxLayout;
          hlayout->setContentsMargins (0, 0, 0, 0);
          hlayout->setSpacing (0);
          node_visibility_threshold_controls->setLayout (hlayout);
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
          node_visibility_threshold_controls->setVisible (false);
          gridlayout->addWidget (node_visibility_threshold_controls, 1, 1, 1, 4);

          label = new QLabel ("Geometry: ");
          gridlayout->addWidget (label, 2, 0, 1, 2);
          node_geometry_combobox = new QComboBox (this);
          node_geometry_combobox->setToolTip (tr ("The 3D geometrical shape used to draw each node"));
          node_geometry_combobox->addItem ("Sphere");
          node_geometry_combobox->addItem ("Cube");
          node_geometry_combobox->addItem ("Overlay");
          node_geometry_combobox->addItem ("Mesh");
          connect (node_geometry_combobox, SIGNAL (activated(int)), this, SLOT (node_geometry_selection_slot (int)));
          gridlayout->addWidget (node_geometry_combobox, 2, 2);
          hlayout = new HBoxLayout;
          hlayout->setContentsMargins (0, 0, 0, 0);
          hlayout->setSpacing (0);
          node_geometry_sphere_lod_label = new QLabel ("LOD: ");
          hlayout->addWidget (node_geometry_sphere_lod_label, 1);
          node_geometry_sphere_lod_spinbox = new SpinBox (this);
          node_geometry_sphere_lod_spinbox->setToolTip (tr ("Level of Detail for drawing spheres"));
          node_geometry_sphere_lod_spinbox->setMinimum (1);
          node_geometry_sphere_lod_spinbox->setMaximum (7);
          node_geometry_sphere_lod_spinbox->setSingleStep (1);
          node_geometry_sphere_lod_spinbox->setValue (4);
          connect (node_geometry_sphere_lod_spinbox, SIGNAL (valueChanged(int)), this, SLOT(sphere_lod_slot(int)));
          hlayout->addWidget (node_geometry_sphere_lod_spinbox, 1);
          node_geometry_overlay_interp_checkbox = new QCheckBox ("Interp");
          node_geometry_overlay_interp_checkbox->setToolTip (tr ("Interpolate the node overlay image"));
          node_geometry_overlay_interp_checkbox->setTristate (false);
          node_geometry_overlay_interp_checkbox->setVisible (false);
          connect (node_geometry_overlay_interp_checkbox, SIGNAL (stateChanged(int)), this, SLOT(overlay_interp_slot(int)));
          hlayout->addWidget (node_geometry_overlay_interp_checkbox, 1);
          node_geometry_overlay_3D_warning_icon = new QLabel();
          node_geometry_overlay_3D_warning_icon->setPixmap (warning_icon.pixmap (node_geometry_combobox->height(), Qt::KeepAspectRatio));
          node_geometry_overlay_3D_warning_icon->setScaledContents (true);
          node_geometry_overlay_3D_warning_icon->setToolTip ("The node overlay image can only be displayed in pure 2D mode (slab thickness of zero)");
          node_geometry_overlay_3D_warning_icon->setVisible (false);
          hlayout->addWidget (node_geometry_overlay_3D_warning_icon, 1);
          gridlayout->addLayout (hlayout, 2, 3, 1, 2);

          label = new QLabel ("Colour: ");
          gridlayout->addWidget (label, 3, 0, 1, 2);
          node_colour_combobox = new QComboBox (this);
          node_colour_combobox->setToolTip (tr ("Set how the colour of each node is determined"));
          node_colour_combobox->addItem ("Fixed");
          node_colour_combobox->addItem ("Random");
          node_colour_combobox->addItem ("LUT");
          node_colour_combobox->addItem ("Connectome");
          node_colour_combobox->addItem ("Vector file");
          node_colour_combobox->addItem ("Matrix file");
          connect (node_colour_combobox, SIGNAL (activated(int)), this, SLOT (node_colour_selection_slot (int)));
          gridlayout->addWidget (node_colour_combobox, 3, 2);
          hlayout = new HBoxLayout;
          hlayout->setContentsMargins (0, 0, 0, 0);
          hlayout->setSpacing (0);
          node_colour_matrix_operator_combobox = new QComboBox (this);
          node_colour_matrix_operator_combobox->setToolTip (tr ("If node colours are determined from a matrix file, and multiple\n"
                                                                "nodes are selected, this operator defines how the entries from\n"
                                                                "the corresponding rows of the matrix are combined to produce a\n"
                                                                "colour for each node."));
          node_colour_matrix_operator_combobox->addItem ("Min");
          node_colour_matrix_operator_combobox->addItem ("Mean");
          node_colour_matrix_operator_combobox->addItem ("Sum");
          node_colour_matrix_operator_combobox->addItem ("Max");
          node_colour_matrix_operator_combobox->addItem ("N/A");
          node_colour_matrix_operator_combobox->setCurrentIndex (2);
          node_colour_matrix_operator_combobox->setVisible (false);
          node_colour_matrix_operator_combobox->setEnabled (false);
          connect (node_colour_matrix_operator_combobox, SIGNAL (activated(int)), this, SLOT (node_colour_matrix_operator_slot (int)));
          hlayout->addWidget (node_colour_matrix_operator_combobox);
          node_colour_fixedcolour_button = new QColorButton;
          node_colour_fixedcolour_button->setToolTip (tr ("Set the fixed colour to use for all nodes"));
          connect (node_colour_fixedcolour_button, SIGNAL (clicked()), this, SLOT (node_fixed_colour_change_slot()));
          hlayout->addWidget (node_colour_fixedcolour_button, 1);
          node_colour_colourmap_button = new ColourMapButton (this, node_colourmap_observer, false, false, true);
          node_colour_colourmap_button->setToolTip (tr ("Select the colourmap for nodes"));
          node_colour_colourmap_button->setVisible (false);
          hlayout->addWidget (node_colour_colourmap_button, 1);
          gridlayout->addLayout (hlayout, 3, 3, 1, 2);

          node_colour_range_controls = new QWidget (this);
          hlayout = new HBoxLayout;
          hlayout->setContentsMargins (0, 0, 0, 0);
          hlayout->setSpacing (0);
          node_colour_range_controls->setLayout (hlayout);
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
          node_colour_range_controls->setVisible (false);
          gridlayout->addWidget (node_colour_range_controls, 4, 1, 1, 4);

          label = new QLabel ("Size scaling: ");
          gridlayout->addWidget (label, 5, 0, 1, 2);
          node_size_combobox = new QComboBox (this);
          node_size_combobox->setToolTip (tr ("Set how the size of each node is determined"));
          node_size_combobox->addItem ("Fixed");
          node_size_combobox->addItem ("Node volume");
          node_size_combobox->addItem ("Connectome");
          node_size_combobox->addItem ("Vector file");
          node_size_combobox->addItem ("Matrix file");
          connect (node_size_combobox, SIGNAL (activated(int)), this, SLOT (node_size_selection_slot (int)));
          gridlayout->addWidget (node_size_combobox, 5, 2);
          hlayout = new HBoxLayout;
          hlayout->setContentsMargins (0, 0, 0, 0);
          hlayout->setSpacing (0);
          node_size_matrix_operator_combobox = new QComboBox (this);
          node_size_matrix_operator_combobox->setToolTip (tr ("If node sizes are determined from a matrix file, and multiple\n"
                                                              "nodes are selected, this operator defines how the entries from\n"
                                                              "the corresponding rows of the matrix are combined to produce a\n"
                                                              "size value for each node."));
          node_size_matrix_operator_combobox->addItem ("Min");
          node_size_matrix_operator_combobox->addItem ("Mean");
          node_size_matrix_operator_combobox->addItem ("Sum");
          node_size_matrix_operator_combobox->addItem ("Max");
          node_size_matrix_operator_combobox->addItem ("N/A");
          node_size_matrix_operator_combobox->setCurrentIndex (2);
          node_size_matrix_operator_combobox->setVisible (false);
          node_size_matrix_operator_combobox->setEnabled (false);
          connect (node_size_matrix_operator_combobox, SIGNAL (activated(int)), this, SLOT (node_size_matrix_operator_slot (int)));
          hlayout->addWidget (node_size_matrix_operator_combobox);
          node_size_button = new AdjustButton (this, 0.01);
          node_size_button->setValue (node_size_scale_factor);
          node_size_button->setMin (0.0f);
          connect (node_size_button, SIGNAL (valueChanged()), this, SLOT (node_size_value_slot()));
          hlayout->addWidget (node_size_button, 1);
          gridlayout->addLayout (hlayout, 5, 3, 1, 2);

          node_size_range_controls = new QWidget (this);
          hlayout = new HBoxLayout;
          hlayout->setContentsMargins (0, 0, 0, 0);
          hlayout->setSpacing (0);
          node_size_range_controls->setLayout (hlayout);
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
          node_size_range_controls->setVisible (false);
          gridlayout->addWidget (node_size_range_controls, 6, 1, 1, 4);

          label = new QLabel ("Transparency: ");
          gridlayout->addWidget (label, 7, 0, 1, 2);
          node_alpha_combobox = new QComboBox (this);
          node_alpha_combobox->setToolTip (tr ("Set how node transparency is determined"));
          node_alpha_combobox->addItem ("Fixed");
          node_alpha_combobox->addItem ("Connectome");
          node_alpha_combobox->addItem ("LUT");
          node_alpha_combobox->addItem ("Vector file");
          node_alpha_combobox->addItem ("Matrix file");
          connect (node_alpha_combobox, SIGNAL (activated(int)), this, SLOT (node_alpha_selection_slot (int)));
          gridlayout->addWidget (node_alpha_combobox, 7, 2);
          hlayout = new HBoxLayout;
          hlayout->setContentsMargins (0, 0, 0, 0);
          hlayout->setSpacing (0);
          node_alpha_matrix_operator_combobox = new QComboBox (this);
          node_alpha_matrix_operator_combobox->setToolTip (tr ("If node transparency is determined from a matrix file, and multiple\n"
                                                                "nodes are selected, this operator defines how the entries from\n"
                                                                "the corresponding rows of the matrix are combined to produce an\n"
                                                                "alpha value for each node."));
          node_alpha_matrix_operator_combobox->addItem ("Min");
          node_alpha_matrix_operator_combobox->addItem ("Mean");
          node_alpha_matrix_operator_combobox->addItem ("Sum");
          node_alpha_matrix_operator_combobox->addItem ("Max");
          node_alpha_matrix_operator_combobox->addItem ("N/A");
          node_alpha_matrix_operator_combobox->setCurrentIndex (2);
          node_alpha_matrix_operator_combobox->setVisible (false);
          node_alpha_matrix_operator_combobox->setEnabled (false);
          connect (node_alpha_matrix_operator_combobox, SIGNAL (activated(int)), this, SLOT (node_alpha_matrix_operator_slot (int)));
          hlayout->addWidget (node_alpha_matrix_operator_combobox);
          node_alpha_slider = new QSlider (Qt::Horizontal);
          node_alpha_slider->setRange (0,1000);
          node_alpha_slider->setSliderPosition (1000);
          connect (node_alpha_slider, SIGNAL (valueChanged (int)), this, SLOT (node_alpha_value_slot (int)));
          hlayout->addWidget (node_alpha_slider, 1);
          gridlayout->addLayout (hlayout, 7, 3, 1, 2);

          node_alpha_range_controls = new QWidget (this);
          hlayout = new HBoxLayout;
          hlayout->setContentsMargins (0, 0, 0, 0);
          hlayout->setSpacing (0);
          node_alpha_range_controls->setLayout (hlayout);
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
          node_alpha_range_controls->setVisible (false);
          gridlayout->addWidget (node_alpha_range_controls, 8, 1, 1, 4);

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
          edge_visibility_combobox->addItem ("Connectome");
          edge_visibility_combobox->addItem ("Matrix file");
          edge_visibility_combobox->setCurrentIndex (3);
          connect (edge_visibility_combobox, SIGNAL (activated(int)), this, SLOT (edge_visibility_selection_slot (int)));
          gridlayout->addWidget (edge_visibility_combobox, 0, 2);
          edge_visibility_warning_icon = new QLabel();
          edge_visibility_warning_icon->setPixmap (warning_icon.pixmap (edge_visibility_combobox->height(), Qt::KeepAspectRatio));
          edge_visibility_warning_icon->setScaledContents (true);
          edge_visibility_warning_icon->setToolTip ("Changes to edge visualisation will have no apparent effect if edge visibility is set to \'none\'");
          edge_visibility_warning_icon->setVisible (false);
          gridlayout->addWidget (edge_visibility_warning_icon, 0, 3);

          edge_visibility_threshold_controls = new QWidget (this);
          hlayout = new HBoxLayout;
          hlayout->setContentsMargins (0, 0, 0, 0);
          hlayout->setSpacing (0);
          edge_visibility_threshold_controls->setLayout (hlayout);
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
          gridlayout->addWidget (edge_visibility_threshold_controls, 1, 1, 1, 4);

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
          edge_geometry_cylinder_lod_spinbox = new SpinBox (this);
          edge_geometry_cylinder_lod_spinbox->setToolTip (tr ("Level of Detail for drawing cylinders / streamtubes"));
          edge_geometry_cylinder_lod_spinbox->setMinimum (1);
          edge_geometry_cylinder_lod_spinbox->setMaximum (7);
          edge_geometry_cylinder_lod_spinbox->setSingleStep (1);
          edge_geometry_cylinder_lod_spinbox->setValue (4);
          edge_geometry_cylinder_lod_spinbox->setVisible (false);
          connect (edge_geometry_cylinder_lod_spinbox, SIGNAL (valueChanged(int)), this, SLOT(cylinder_lod_slot(int)));
          hlayout->addWidget (edge_geometry_cylinder_lod_spinbox, 1);
          edge_geometry_line_smooth_checkbox = new QCheckBox ("Smooth");
          edge_geometry_line_smooth_checkbox->setToolTip (tr ("Use OpenGL's smooth line drawing feature"));
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
          edge_colour_combobox->addItem ("Connectome");
          edge_colour_combobox->addItem ("Matrix file");
          edge_colour_combobox->setCurrentIndex (2);
          connect (edge_colour_combobox, SIGNAL (activated(int)), this, SLOT (edge_colour_selection_slot (int)));
          gridlayout->addWidget (edge_colour_combobox, 3, 2);
          hlayout = new HBoxLayout;
          hlayout->setContentsMargins (0, 0, 0, 0);
          hlayout->setSpacing (0);
          edge_colour_fixedcolour_button = new QColorButton;
          edge_colour_fixedcolour_button->setToolTip (tr ("Set the fixed colour to use for all edges"));
          edge_colour_fixedcolour_button->setVisible (false);
          connect (edge_colour_fixedcolour_button, SIGNAL (clicked()), this, SLOT (edge_colour_change_slot()));
          hlayout->addWidget (edge_colour_fixedcolour_button, 1);
          edge_colour_colourmap_button = new ColourMapButton (this, edge_colourmap_observer, false, false, true);
          edge_colour_colourmap_button->setToolTip (tr ("Select the colourmap for nodes"));
          hlayout->addWidget (edge_colour_colourmap_button, 1);
          gridlayout->addLayout (hlayout, 3, 3, 1, 2);

          edge_colour_range_controls = new QWidget (this);
          hlayout = new HBoxLayout;
          hlayout->setContentsMargins (0, 0, 0, 0);
          hlayout->setSpacing (0);
          edge_colour_range_controls->setLayout (hlayout);
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
          gridlayout->addWidget (edge_colour_range_controls, 4, 1, 1, 4);

          label = new QLabel ("Size scaling: ");
          gridlayout->addWidget (label, 5, 0, 1, 2);
          edge_size_combobox = new QComboBox (this);
          edge_size_combobox->setToolTip (tr ("Set how the width of each edge is determined"));
          edge_size_combobox->addItem ("Fixed");
          edge_size_combobox->addItem ("Connectome");
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

          edge_size_range_controls = new QWidget (this);
          hlayout = new HBoxLayout;
          hlayout->setContentsMargins (0, 0, 0, 0);
          hlayout->setSpacing (0);
          edge_size_range_controls->setLayout (hlayout);
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
          edge_size_range_controls->setVisible (false);
          gridlayout->addWidget (edge_size_range_controls, 6, 1, 1, 4);

          label = new QLabel ("Transparency: ");
          gridlayout->addWidget (label, 7, 0, 1, 2);
          edge_alpha_combobox = new QComboBox (this);
          edge_alpha_combobox->setToolTip (tr ("Set how edge transparency is determined"));
          edge_alpha_combobox->addItem ("Fixed");
          edge_alpha_combobox->addItem ("Connectome");
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

          edge_alpha_range_controls = new QWidget (this);
          hlayout = new HBoxLayout;
          hlayout->setContentsMargins (0, 0, 0, 0);
          hlayout->setSpacing (0);
          edge_alpha_range_controls->setLayout (hlayout);
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
          edge_alpha_range_controls->setVisible (false);
          gridlayout->addWidget (edge_alpha_range_controls, 8, 1, 1, 4);

          group_box = new QGroupBox ("Miscellaneous options");
          main_box->addWidget (group_box);
          gridlayout = new GridLayout();
          group_box->setLayout (gridlayout);

          gridlayout->addWidget (new QLabel ("LUT: "), 0, 0);
          lut_button = new QPushButton (this);
          lut_button->setToolTip (tr ("Open lookup table file\n"
                                      "If the primary parcellation image has come from an atlas that\n"
                                      "provides a look-up table, select that file here so that MRview \n"
                                      "can access the appropriate node colours."));
          lut_button->setText ("(none)");
          connect (lut_button, SIGNAL (clicked()), this, SLOT (lut_open_slot ()));
          gridlayout->addWidget (lut_button, 0, 1);

          lighting_checkbox = new QCheckBox ("Lighting");
          lighting_checkbox->setTristate (false);
          lighting_checkbox->setChecked (true);
          lighting_checkbox->setToolTip (tr ("Toggle whether lighting should be applied to compatible elements"));
          connect (lighting_checkbox, SIGNAL (stateChanged (int)), this, SLOT (lighting_change_slot (int)));
          gridlayout->addWidget (lighting_checkbox, 1, 0);
          lighting_settings_button = new QPushButton ("Settings...");
          lighting_settings_button->setToolTip (tr ("Advanced lighting configuration"));
          connect (lighting_settings_button, SIGNAL (clicked()), this, SLOT (lighting_settings_slot()));
          gridlayout->addWidget (lighting_settings_button, 1, 1);
          connect (&lighting, SIGNAL (changed()), SLOT (lighting_parameter_slot()));

          crop_to_slab_checkbox = new QCheckBox ("Crop to slab");
          crop_to_slab_checkbox->setTristate (false);
          connect (crop_to_slab_checkbox, SIGNAL (stateChanged(int)), this, SLOT (crop_to_slab_toggle_slot (int)));
          gridlayout->addWidget (crop_to_slab_checkbox, 2, 0);
          hlayout = new HBoxLayout;
          hlayout->setContentsMargins (0, 0, 0, 0);
          hlayout->setSpacing (0);
          crop_to_slab_label = new QLabel ("Thickness: ");
          crop_to_slab_label->setEnabled (false);
          hlayout->addWidget (crop_to_slab_label);
          crop_to_slab_button = new AdjustButton (this);
          crop_to_slab_button->setValue (0.0f);
          crop_to_slab_button->setMin (0.0f);
          crop_to_slab_button->setRate (0.1f);
          crop_to_slab_button->setEnabled (false);
          connect (crop_to_slab_button, SIGNAL (valueChanged()), this, SLOT (crop_to_slab_parameter_slot()));
          hlayout->addWidget (crop_to_slab_button);
          gridlayout->addLayout (hlayout, 2, 1);

          show_node_list_label = new QLabel ("Node selection: ");
          gridlayout->addWidget (show_node_list_label, 3, 0);
          show_node_list_button = new QPushButton ("Show list");
          show_node_list_button->setToolTip (tr ("Open window that displays list of nodes and enables their selection"));
          connect (show_node_list_button, SIGNAL (clicked()), this, SLOT (show_node_list_slot()));
          gridlayout->addWidget (show_node_list_button, 3, 1);

          main_box->addWidget (node_list);

          main_box->addStretch ();
          setMinimumSize (main_box->minimumSize());

          node_list->tool = new Node_list (node_list, this);
          node_list->tool->adjustSize();
          node_list->setWidget (node_list->tool);
          node_list->setFeatures (QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
          window().addDockWidget (Qt::RightDockWidgetArea, node_list);
          connect (&node_selection_settings, SIGNAL(dataChanged()), this, SLOT (node_selection_settings_changed_slot()));

          const int height = node_visibility_combobox->sizeHint().height();
          node_visibility_warning_icon         ->setFixedSize   (height, height);
          node_geometry_overlay_3D_warning_icon->setFixedSize   (height, height);
          node_colour_fixedcolour_button       ->setFixedHeight (height);
          node_colour_colourmap_button         ->setFixedHeight (height);
          edge_visibility_warning_icon         ->setFixedSize   (height, height);
          edge_colour_fixedcolour_button       ->setFixedHeight (height);
          edge_colour_colourmap_button         ->setFixedHeight (height);

          MRView::GrabContext context;
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;

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

          glGetIntegerv (gl::ALIASED_LINE_WIDTH_RANGE, line_thickness_range_aliased);
          glGetIntegerv (gl::SMOOTH_LINE_WIDTH_RANGE, line_thickness_range_smooth);
          GL_CHECK_ERROR;

          enable_all (false);
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;
        }


        Connectome::~Connectome () {}


        void Connectome::draw (const Projection& projection, bool /*is_3D*/, int /*axis*/, int /*slice*/)
        {
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;
          if (hide_all_button->isChecked()) return;

          // If using transparency, only want to draw the close surface;
          //   trying to draw both surfaces results in problems because the
          //   triangle render order is not correctly set
          // If not using transparency, might as well enable it; potential performance gain
          //   since we guarantee correct surface normals
          GLboolean current_cull_face = false;
          gl::GetBooleanv (gl::CULL_FACE, &current_cull_face);
          if (crop_to_slab)
            gl::Disable (gl::CULL_FACE);
          else
            gl::Enable (gl::CULL_FACE);

          if (use_alpha_nodes() && !use_alpha_edges()) {
            draw_edges (projection);
            draw_nodes (projection);
          } else {
            draw_nodes (projection);
            draw_edges (projection);
          }

          if (!current_cull_face)
            gl::Disable (gl::CULL_FACE);
          else
            gl::Enable (gl::CULL_FACE);
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;
        }


        void Connectome::draw_colourbars()
        {
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;
          if (!buffer) return;
          if (hide_all_button->isChecked()) return;
          if (((node_colour == node_colour_t::CONNECTOME && matrix_list_model->rowCount()) || node_colour == node_colour_t::VECTOR_FILE || node_colour == node_colour_t::MATRIX_FILE) && show_node_colour_bar)
            window().colourbar_renderer.render (node_colourmap_index, node_colourmap_invert,
                                                node_colour_lower_button->value(), node_colour_upper_button->value(),
                                                node_colour_lower_button->value(), node_colour_upper_button->value() - node_colour_lower_button->value(),
                                                node_fixed_colour);
          if (((edge_colour == edge_colour_t::CONNECTOME && matrix_list_model->rowCount()) || edge_colour == edge_colour_t::MATRIX_FILE) && show_edge_colour_bar)
            window().colourbar_renderer.render (edge_colourmap_index, edge_colourmap_invert,
                                                edge_colour_lower_button->value(), edge_colour_upper_button->value(),
                                                edge_colour_lower_button->value(), edge_colour_upper_button->value() - edge_colour_lower_button->value(),
                                                edge_fixed_colour);
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;
        }


        size_t Connectome::visible_number_colourbars()
        {
          if (!buffer) return 0;
          return (((((node_colour == node_colour_t::CONNECTOME && matrix_list_model->rowCount()) || node_colour == node_colour_t::VECTOR_FILE || node_colour == node_colour_t::MATRIX_FILE) && show_node_colour_bar) ? 1 : 0)
                  + ((((edge_colour == edge_colour_t::CONNECTOME && matrix_list_model->rowCount()) || edge_colour == edge_colour_t::MATRIX_FILE) && show_edge_colour_bar) ? 1 : 0));
        }




        void Connectome::add_commandline_options (MR::App::OptionList& options)
        {
          using namespace MR::App;
          options
            + OptionGroup ("Connectome tool options")

            + Option ("connectome.init", "Initialise the connectome tool using a parcellation image.")
            +   Argument ("image").type_image_in()

            + Option ("connectome.load", "Load a matrix file into the connectome tool.")
            +   Argument ("path").type_file_in();

        }

        bool Connectome::process_commandline_option (const MR::App::ParsedOption& opt)
        {
          if (opt.opt->is ("connectome.init")) {
            try {
              initialise (opt[0]);
              image_button->setText (QString::fromStdString (Path::basename (opt[0])));
              load_properties();
              enable_all (true);
            } catch (Exception& e) { e.display(); clear_all(); }
            return true;
          }
          if (opt.opt->is ("connectome.load")) {
            try {
              std::vector<std::string> list (1, opt[0]);
              add_matrices (list);
            } catch (Exception& e) { e.display(); }
            return true;
          }
          return false;
        }

        void Connectome::image_open_slot()
        {
          const std::string path = Dialog::File::get_image (this, "Select connectome parcellation image");
          if (path.empty())
            return;

          // Read in the image file, do the necessary conversions e.g. to mesh, store the number of nodes, ...
          try {
            initialise (path);
            image_button->setText (QString::fromStdString (Path::basename (path)));
            load_properties();
            enable_all (true);
          } catch (Exception& e) {
            e.display();
            // If importing a new image has failed, but another image was loaded previously, keep existing data
          }
          window().updateGL();
        }


        void Connectome::hide_all_slot()
        {
          window().updateGL();
        }







        void Connectome::matrix_open_slot ()
        {
          std::vector<std::string> list = Dialog::File::get_files (&window(), "Select connectome file(s) to open");
          if (list.empty())
            return;
          add_matrices (list);
        }

        void Connectome::matrix_close_slot ()
        {
          QModelIndexList indexes = matrix_list_view->selectionModel()->selectedIndexes();
          if (indexes.size())
            matrix_list_model->remove_item (indexes.first());
          window().updateGL();
        }

        void Connectome::connectome_selection_changed_slot (const QItemSelection&, const QItemSelection&)
        {
          if (node_visibility == node_visibility_t::CONNECTOME)
            calculate_node_visibility();
          if (node_colour == node_colour_t::CONNECTOME)
            calculate_node_colours();
          if (node_size == node_size_t::CONNECTOME)
            calculate_node_sizes();
          if (node_alpha == node_alpha_t::CONNECTOME)
            calculate_node_alphas();
          if (edge_visibility == edge_visibility_t::CONNECTOME)
            calculate_edge_visibility();
          if (edge_colour == edge_colour_t::CONNECTOME)
            calculate_edge_colours();
          if (edge_size == edge_size_t::CONNECTOME)
            calculate_edge_sizes();
          if (edge_alpha == edge_alpha_t::CONNECTOME)
            calculate_edge_alphas();
          window().updateGL();
        }









        void Connectome::node_visibility_selection_slot (int index)
        {
          node_visibility_warning_icon->setVisible (false);
          switch (index) {
            case 0:
              if (node_visibility == node_visibility_t::ALL) return;
              node_visibility = node_visibility_t::ALL;
              node_visibility_combobox->removeItem (6);
              node_visibility_matrix_operator_combobox->setVisible (false);
              node_visibility_threshold_controls->setVisible (false);
              break;
            case 1:
              if (node_visibility == node_visibility_t::NONE) return;
              node_visibility = node_visibility_t::NONE;
              node_visibility_combobox->removeItem (6);
              node_visibility_matrix_operator_combobox->setVisible (false);
              node_visibility_threshold_controls->setVisible (false);
              break;
            case 2:
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
              node_visibility_combobox->removeItem (6);
              node_visibility_matrix_operator_combobox->setVisible (false);
              node_visibility_threshold_controls->setVisible (false);
              break;
            case 3:
              if (node_visibility == node_visibility_t::CONNECTOME) return;
              node_visibility = node_visibility_t::CONNECTOME;
              node_visibility_combobox->removeItem (6);
              node_visibility_matrix_operator_combobox->setVisible (true);
              if (selected_node_count >= 2) {
                node_visibility_matrix_operator_combobox->removeItem (2);
                switch (node_visibility_matrix_operator) {
                  case node_visibility_matrix_operator_t::ANY: node_visibility_matrix_operator_combobox->setCurrentIndex (0); break;
                  case node_visibility_matrix_operator_t::ALL: node_visibility_matrix_operator_combobox->setCurrentIndex (1); break;
                }
                node_visibility_matrix_operator_combobox->setEnabled (true);
              } else {
                if (node_visibility_matrix_operator_combobox->count() == 2)
                  node_visibility_matrix_operator_combobox->addItem ("N/A");
                node_visibility_matrix_operator_combobox->setCurrentIndex (2);
                node_visibility_matrix_operator_combobox->setEnabled (false);
              }
              node_visibility_threshold_controls->setVisible (true);
              {
                float min = 0.0f, mean = 0.0f, max = 0.0f;
                QModelIndexList list = matrix_list_view->selectionModel()->selectedRows();
                if (list.size()) {
                  const FileDataVector& data (matrix_list_model->get (list[0]));
                  min = data.get_min(); mean = data.get_mean(); max = data.get_max();
                }
                update_controls_node_visibility (min, mean, max);
              }
              break;
            case 4:
              if (!import_vector_file (node_values_from_file_visibility, "node visibility")) {
                switch (node_visibility) {
                  case node_visibility_t::ALL:         node_visibility_combobox->setCurrentIndex (0); return;
                  case node_visibility_t::NONE:        node_visibility_combobox->setCurrentIndex (1); return;
                  case node_visibility_t::DEGREE:      node_visibility_combobox->setCurrentIndex (2); return;
                  case node_visibility_t::CONNECTOME:  node_visibility_combobox->setCurrentIndex (3); return;
                  case node_visibility_t::VECTOR_FILE: node_visibility_combobox->setCurrentIndex (6); return;
                  case node_visibility_t::MATRIX_FILE: node_visibility_combobox->setCurrentIndex (6); return;
                }
              }
              node_visibility = node_visibility_t::VECTOR_FILE;
              if (node_visibility_combobox->count() == 6)
                node_visibility_combobox->addItem (node_values_from_file_visibility.get_name());
              else
                node_visibility_combobox->setItemText (6, node_values_from_file_visibility.get_name());
              node_visibility_combobox->setCurrentIndex (6);
              node_visibility_matrix_operator_combobox->setVisible (false);
              node_visibility_threshold_controls->setVisible (true);
              update_controls_node_visibility (node_values_from_file_visibility.get_min(), node_values_from_file_visibility.get_mean(), node_values_from_file_visibility.get_max());
              break;
            case 5:
              if (!import_matrix_file (node_values_from_file_visibility, "node visibility")) {
                switch (node_visibility) {
                  case node_visibility_t::ALL:         node_visibility_combobox->setCurrentIndex (0); return;
                  case node_visibility_t::NONE:        node_visibility_combobox->setCurrentIndex (1); return;
                  case node_visibility_t::DEGREE:      node_visibility_combobox->setCurrentIndex (2); return;
                  case node_visibility_t::CONNECTOME:  node_visibility_combobox->setCurrentIndex (3); return;
                  case node_visibility_t::VECTOR_FILE: node_visibility_combobox->setCurrentIndex (6); return;
                  case node_visibility_t::MATRIX_FILE: node_visibility_combobox->setCurrentIndex (6); return;
                }
              }
              node_visibility = node_visibility_t::MATRIX_FILE;
              if (node_visibility_combobox->count() == 6)
                node_visibility_combobox->addItem (node_values_from_file_visibility.get_name());
              else
                node_visibility_combobox->setItemText (6, node_values_from_file_visibility.get_name());
              node_visibility_combobox->setCurrentIndex (6);
              node_visibility_matrix_operator_combobox->setVisible (true);
              if (selected_node_count >= 2) {
                node_visibility_matrix_operator_combobox->removeItem (2);
                switch (node_visibility_matrix_operator) {
                  case node_visibility_matrix_operator_t::ANY: node_visibility_matrix_operator_combobox->setCurrentIndex (0); break;
                  case node_visibility_matrix_operator_t::ALL: node_visibility_matrix_operator_combobox->setCurrentIndex (1); break;
                }
                node_visibility_matrix_operator_combobox->setEnabled (true);
              } else {
                if (node_visibility_matrix_operator_combobox->count() == 2)
                  node_visibility_matrix_operator_combobox->addItem ("N/A");
                node_visibility_matrix_operator_combobox->setCurrentIndex (2);
                node_visibility_matrix_operator_combobox->setEnabled (false);
              }
              node_visibility_threshold_controls->setVisible (true);
              update_controls_node_visibility (node_values_from_file_visibility.get_min(), node_values_from_file_visibility.get_mean(), node_values_from_file_visibility.get_max());
              break;
            case 6:
              return;
          }
          calculate_node_visibility();
          window().updateGL();
        }

        void Connectome::node_geometry_selection_slot (int index)
        {
          node_visibility_warning_icon->setVisible (false);
          node_geometry_overlay_3D_warning_icon->setVisible (false);
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
              node_geometry_overlay_3D_warning_icon->setVisible (is_3D);
              update_node_overlay();
              break;
            case 3:
              try {
                // Re-prompt user if they are already displaying meshes and they re-select the mesh option
                if (!have_meshes || node_geometry == node_geometry_t::MESH) {
                  get_meshes();
                  if (!have_meshes)
                    throw Exception ("No file path provided; cannot render meshes");
                }
                node_geometry = node_geometry_t::MESH;
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
              } catch (Exception& e) {
                e.display();
                for (auto i = nodes.begin(); i != nodes.end(); ++i)
                  i->clear_mesh();
                have_meshes = false;
                node_geometry = node_geometry_t::SPHERE;
                node_geometry_combobox->setCurrentIndex (0);
                node_size_combobox->setEnabled (true);
                node_size_button->setVisible (true);
                node_size_button->setMax (std::numeric_limits<float>::max());
                node_geometry_sphere_lod_label->setVisible (true);
                node_geometry_sphere_lod_spinbox->setVisible (true);
                node_geometry_overlay_interp_checkbox->setVisible (false);
              }
              break;
          }
          if (node_visibility == node_visibility_t::NONE)
            node_visibility_warning_icon->setVisible (true);
          window().updateGL();
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
              node_colour_combobox->removeItem (6);
              node_colour_matrix_operator_combobox->setVisible (false);
              node_colour_range_controls->setVisible (false);
              break;
            case 1:
              // Regenerate random colours on repeat selection
              node_colour = node_colour_t::RANDOM;
              node_colour_colourmap_button->setVisible (false);
              node_colour_fixedcolour_button->setVisible (false);
              node_colour_combobox->removeItem (6);
              node_colour_matrix_operator_combobox->setVisible (false);
              node_colour_range_controls->setVisible (false);
              break;
            case 2:
              if (node_colour == node_colour_t::FROM_LUT) return;
              node_colour = node_colour_t::FROM_LUT;
              node_colour_fixedcolour_button->setVisible (false);
              node_colour_colourmap_button->setVisible (false);
              node_colour_combobox->removeItem (6);
              node_colour_matrix_operator_combobox->setVisible (false);
              node_colour_range_controls->setVisible (false);
              break;
            case 3:
              if (node_colour == node_colour_t::CONNECTOME) return;
              node_colour = node_colour_t::CONNECTOME;
              node_colour_colourmap_button->setVisible (true);
              node_colour_fixedcolour_button->setVisible (false);
              node_colour_combobox->removeItem (6);
              node_colour_matrix_operator_combobox->setVisible (true);
              if (selected_node_count >= 2) {
                node_colour_matrix_operator_combobox->removeItem (4);
                switch (node_colour_matrix_operator) {
                  case node_property_matrix_operator_t::MIN:  node_colour_matrix_operator_combobox->setCurrentIndex (0); break;
                  case node_property_matrix_operator_t::MEAN: node_colour_matrix_operator_combobox->setCurrentIndex (1); break;
                  case node_property_matrix_operator_t::SUM:  node_colour_matrix_operator_combobox->setCurrentIndex (2); break;
                  case node_property_matrix_operator_t::MAX:  node_colour_matrix_operator_combobox->setCurrentIndex (3); break;
                }
                node_colour_matrix_operator_combobox->setEnabled (true);
              } else {
                if (node_colour_matrix_operator_combobox->count() == 4)
                  node_colour_matrix_operator_combobox->addItem ("N/A");
                node_colour_matrix_operator_combobox->setCurrentIndex (4);
                node_colour_matrix_operator_combobox->setEnabled (false);
              }
              node_colour_range_controls->setVisible (true);
              {
                float min = 0.0f, mean = 0.0f, max = 0.0f;
                QModelIndexList list = matrix_list_view->selectionModel()->selectedRows();
                if (list.size()) {
                  const FileDataVector& data (matrix_list_model->get (list[0]));
                  min = data.get_min(); mean = data.get_mean(); max = data.get_max();
                }
                update_controls_node_colour (min, mean, max);
              }
              break;
            case 4:
              if (!import_vector_file (node_values_from_file_colour, "node colours")) {
                switch (node_colour) {
                  case node_colour_t::FIXED:       node_colour_combobox->setCurrentIndex (0); return;
                  case node_colour_t::RANDOM:      node_colour_combobox->setCurrentIndex (1); return;
                  case node_colour_t::FROM_LUT:    node_colour_combobox->setCurrentIndex (2); return;
                  case node_colour_t::CONNECTOME:  node_colour_combobox->setCurrentIndex (3); return;
                  case node_colour_t::VECTOR_FILE: node_colour_combobox->setCurrentIndex (6); return;
                  case node_colour_t::MATRIX_FILE: node_colour_combobox->setCurrentIndex (6); return;
                }
              }
              node_colour = node_colour_t::VECTOR_FILE;
              node_colour_colourmap_button->setVisible (true);
              node_colour_fixedcolour_button->setVisible (false);
              if (node_colour_combobox->count() == 6)
                node_colour_combobox->addItem (node_values_from_file_colour.get_name());
              else
                node_colour_combobox->setItemText (6, node_values_from_file_colour.get_name());
              node_colour_combobox->setCurrentIndex (6);
              node_colour_matrix_operator_combobox->setVisible (false);
              node_colour_range_controls->setVisible (true);
              update_controls_node_colour (node_values_from_file_colour.get_min(), node_values_from_file_colour.get_mean(), node_values_from_file_colour.get_max());
              break;
            case 5:
              if (!import_matrix_file (node_values_from_file_colour, "node colours")) {
                switch (node_colour) {
                  case node_colour_t::FIXED:       node_colour_combobox->setCurrentIndex (0); return;
                  case node_colour_t::RANDOM:      node_colour_combobox->setCurrentIndex (1); return;
                  case node_colour_t::FROM_LUT:    node_colour_combobox->setCurrentIndex (2); return;
                  case node_colour_t::CONNECTOME:  node_colour_combobox->setCurrentIndex (3); return;
                  case node_colour_t::VECTOR_FILE: node_colour_combobox->setCurrentIndex (6); return;
                  case node_colour_t::MATRIX_FILE: node_colour_combobox->setCurrentIndex (6); return;
                }
              }
              node_colour = node_colour_t::MATRIX_FILE;
              node_colour_colourmap_button->setVisible (true);
              node_colour_fixedcolour_button->setVisible (false);
              if (node_colour_combobox->count() == 6)
                node_colour_combobox->addItem (node_values_from_file_colour.get_name());
              else
                node_colour_combobox->setItemText (6, node_values_from_file_colour.get_name());
              node_colour_combobox->setCurrentIndex (6);
              node_colour_matrix_operator_combobox->setVisible (true);
              if (selected_node_count >= 2) {
                node_colour_matrix_operator_combobox->removeItem (4);
                switch (node_colour_matrix_operator) {
                  case node_property_matrix_operator_t::MIN:  node_colour_matrix_operator_combobox->setCurrentIndex (0); break;
                  case node_property_matrix_operator_t::MEAN: node_colour_matrix_operator_combobox->setCurrentIndex (1); break;
                  case node_property_matrix_operator_t::SUM:  node_colour_matrix_operator_combobox->setCurrentIndex (2); break;
                  case node_property_matrix_operator_t::MAX:  node_colour_matrix_operator_combobox->setCurrentIndex (3); break;
                }
                node_colour_matrix_operator_combobox->setEnabled (true);
              } else {
                if (node_colour_matrix_operator_combobox->count() == 4)
                  node_colour_matrix_operator_combobox->addItem ("N/A");
                node_colour_matrix_operator_combobox->setCurrentIndex (4);
                node_colour_matrix_operator_combobox->setEnabled (false);
              }
              node_colour_range_controls->setVisible (true);
              update_controls_node_colour (node_values_from_file_colour.get_min(), node_values_from_file_colour.get_mean(), node_values_from_file_colour.get_max());
              break;
            case 6:
              return;
          }
          if (node_visibility == node_visibility_t::NONE)
            node_visibility_warning_icon->setVisible (true);
          calculate_node_colours();
          window().updateGL();
        }

        void Connectome::node_size_selection_slot (int index)
        {
          assert (node_geometry != node_geometry_t::OVERLAY);
          node_visibility_warning_icon->setVisible (false);
          switch (index) {
            case 0:
              if (node_size == node_size_t::FIXED) return;
              node_size = node_size_t::FIXED;
              node_size_combobox->removeItem (5);
              node_size_matrix_operator_combobox->setVisible (false);
              node_size_range_controls->setVisible (false);
              break;
            case 1:
              if (node_size == node_size_t::NODE_VOLUME) return;
              node_size = node_size_t::NODE_VOLUME;
              node_size_combobox->removeItem (5);
              node_size_matrix_operator_combobox->setVisible (false);
              node_size_range_controls->setVisible (false);
              break;
            case 2:
              if (node_size == node_size_t::CONNECTOME) return;
              node_size = node_size_t::CONNECTOME;
              node_size_combobox->removeItem (5);
              node_size_matrix_operator_combobox->setVisible (true);
              if (selected_node_count >= 2) {
                node_size_matrix_operator_combobox->removeItem (4);
                switch (node_size_matrix_operator) {
                  case node_property_matrix_operator_t::MIN:  node_size_matrix_operator_combobox->setCurrentIndex (0); break;
                  case node_property_matrix_operator_t::MEAN: node_size_matrix_operator_combobox->setCurrentIndex (1); break;
                  case node_property_matrix_operator_t::SUM:  node_size_matrix_operator_combobox->setCurrentIndex (2); break;
                  case node_property_matrix_operator_t::MAX:  node_size_matrix_operator_combobox->setCurrentIndex (3); break;
                }
                node_size_matrix_operator_combobox->setEnabled (true);
              } else {
                if (node_size_matrix_operator_combobox->count() == 4)
                  node_size_matrix_operator_combobox->addItem ("N/A");
                node_size_matrix_operator_combobox->setCurrentIndex (4);
                node_size_matrix_operator_combobox->setEnabled (false);
              }
              node_size_range_controls->setVisible (true);
              {
                float min = 0.0f, mean = 0.0f, max = 0.0f;
                QModelIndexList list = matrix_list_view->selectionModel()->selectedRows();
                if (list.size()) {
                  const FileDataVector& data (matrix_list_model->get (list[0]));
                  min = data.get_min(); mean = data.get_mean(); max = data.get_max();
                }
                update_controls_node_size (min, mean, max);
              }
              node_size_invert_checkbox->setChecked (false);
              break;
            case 3:
              if (!import_vector_file (node_values_from_file_size, "node size")) {
                switch (node_size) {
                  case node_size_t::FIXED:       node_size_combobox->setCurrentIndex (0); return;
                  case node_size_t::NODE_VOLUME: node_size_combobox->setCurrentIndex (1); return;
                  case node_size_t::CONNECTOME:  node_size_combobox->setCurrentIndex (2); return;
                  case node_size_t::VECTOR_FILE: node_size_combobox->setCurrentIndex (5); return;
                  case node_size_t::MATRIX_FILE: node_size_combobox->setCurrentIndex (5); return;
                }
              }
              node_size = node_size_t::VECTOR_FILE;
              if (node_size_combobox->count() == 5)
                node_size_combobox->addItem (node_values_from_file_size.get_name());
              else
                node_size_combobox->setItemText (5, node_values_from_file_size.get_name());
              node_size_combobox->setCurrentIndex (5);
              node_size_matrix_operator_combobox->setVisible (false);
              node_size_range_controls->setVisible (true);
              update_controls_node_size (node_values_from_file_size.get_min(), node_values_from_file_size.get_mean(), node_values_from_file_size.get_max());
              node_size_invert_checkbox->setChecked (false);
              break;
            case 4:
              if (!import_matrix_file (node_values_from_file_size, "node size")) {
                switch (node_size) {
                  case node_size_t::FIXED:       node_size_combobox->setCurrentIndex (0); return;
                  case node_size_t::NODE_VOLUME: node_size_combobox->setCurrentIndex (1); return;
                  case node_size_t::CONNECTOME:  node_size_combobox->setCurrentIndex (2); return;
                  case node_size_t::VECTOR_FILE: node_size_combobox->setCurrentIndex (5); return;
                  case node_size_t::MATRIX_FILE: node_size_combobox->setCurrentIndex (5); return;
                }
              }
              node_size = node_size_t::MATRIX_FILE;
              if (node_size_combobox->count() == 4)
                node_size_combobox->addItem (node_values_from_file_size.get_name());
              else
                node_size_combobox->setItemText (4, node_values_from_file_size.get_name());
              node_size_combobox->setCurrentIndex (4);
              node_size_matrix_operator_combobox->setVisible (true);
              if (selected_node_count >= 2) {
                node_size_matrix_operator_combobox->removeItem (4);
                switch (node_size_matrix_operator) {
                  case node_property_matrix_operator_t::MIN:  node_size_matrix_operator_combobox->setCurrentIndex (0); break;
                  case node_property_matrix_operator_t::MEAN: node_size_matrix_operator_combobox->setCurrentIndex (1); break;
                  case node_property_matrix_operator_t::SUM:  node_size_matrix_operator_combobox->setCurrentIndex (2); break;
                  case node_property_matrix_operator_t::MAX:  node_size_matrix_operator_combobox->setCurrentIndex (3); break;
                }
                node_size_matrix_operator_combobox->setEnabled (true);
              } else {
                if (node_size_matrix_operator_combobox->count() == 4)
                  node_size_matrix_operator_combobox->addItem ("N/A");
                node_size_matrix_operator_combobox->setCurrentIndex (4);
                node_size_matrix_operator_combobox->setEnabled (false);
              }
              node_size_range_controls->setVisible (true);
              update_controls_node_size (node_values_from_file_size.get_min(), node_values_from_file_size.get_mean(), node_values_from_file_size.get_max());
              node_size_invert_checkbox->setChecked (false);
              break;
            case 5:
              return;
          }
          if (node_visibility == node_visibility_t::NONE)
            node_visibility_warning_icon->setVisible (true);
          calculate_node_sizes();
          window().updateGL();
        }

        void Connectome::node_alpha_selection_slot (int index)
        {
          node_visibility_warning_icon->setVisible (false);
          switch (index) {
            case 0:
              if (node_alpha == node_alpha_t::FIXED) return;
              node_alpha = node_alpha_t::FIXED;
              node_alpha_combobox->removeItem (5);
              node_alpha_matrix_operator_combobox->setVisible (false);
              node_alpha_range_controls->setVisible (false);
              break;
            case 1:
              if (node_alpha == node_alpha_t::CONNECTOME) return;
              node_alpha = node_alpha_t::CONNECTOME;
              node_alpha_combobox->removeItem (5);
              node_alpha_matrix_operator_combobox->setVisible (true);
              if (selected_node_count >= 2) {
                node_alpha_matrix_operator_combobox->removeItem (4);
                switch (node_alpha_matrix_operator) {
                  case node_property_matrix_operator_t::MIN:  node_alpha_matrix_operator_combobox->setCurrentIndex (0); break;
                  case node_property_matrix_operator_t::MEAN: node_alpha_matrix_operator_combobox->setCurrentIndex (1); break;
                  case node_property_matrix_operator_t::SUM:  node_alpha_matrix_operator_combobox->setCurrentIndex (2); break;
                  case node_property_matrix_operator_t::MAX:  node_alpha_matrix_operator_combobox->setCurrentIndex (3); break;
                }
                node_alpha_matrix_operator_combobox->setEnabled (true);
              } else {
                if (node_alpha_matrix_operator_combobox->count() == 4)
                  node_alpha_matrix_operator_combobox->addItem ("N/A");
                node_alpha_matrix_operator_combobox->setCurrentIndex (4);
                node_alpha_matrix_operator_combobox->setEnabled (false);
              }
              node_alpha_range_controls->setVisible (true);
              {
                float min = 0.0f, mean = 0.0f, max = 0.0f;
                QModelIndexList list = matrix_list_view->selectionModel()->selectedRows();
                if (list.size()) {
                  const FileDataVector& data (matrix_list_model->get (list[0]));
                  min = data.get_min(); mean = data.get_mean(); max = data.get_max();
                }
                update_controls_node_alpha (min, mean, max);
              }
              node_alpha_invert_checkbox->setChecked (false);
              break;
            case 2:
              if (node_alpha == node_alpha_t::FROM_LUT) return;
              node_alpha = node_alpha_t::FROM_LUT;
              node_alpha_combobox->removeItem (5);
              node_alpha_matrix_operator_combobox->setVisible (false);
              node_alpha_range_controls->setVisible (false);
              break;
            case 3:
              if (!import_vector_file (node_values_from_file_alpha, "node transparency")) {
                switch (node_alpha) {
                  case node_alpha_t::FIXED:       node_alpha_combobox->setCurrentIndex (0); return;
                  case node_alpha_t::CONNECTOME:  node_alpha_combobox->setCurrentIndex (1); return;
                  case node_alpha_t::FROM_LUT:    node_alpha_combobox->setCurrentIndex (2); return;
                  case node_alpha_t::VECTOR_FILE: node_alpha_combobox->setCurrentIndex (5); return;
                  case node_alpha_t::MATRIX_FILE: node_alpha_combobox->setCurrentIndex (5); return;
                }
              }
              node_alpha = node_alpha_t::VECTOR_FILE;
              if (node_alpha_combobox->count() == 5)
                node_alpha_combobox->addItem (node_values_from_file_alpha.get_name());
              else
                node_alpha_combobox->setItemText (5, node_values_from_file_alpha.get_name());
              node_alpha_combobox->setCurrentIndex (5);
              node_alpha_matrix_operator_combobox->setVisible (false);
              node_alpha_range_controls->setVisible (true);
              update_controls_node_alpha (node_values_from_file_alpha.get_min(), node_values_from_file_alpha.get_mean(), node_values_from_file_alpha.get_max());
              node_alpha_invert_checkbox->setChecked (false);
              break;
            case 4:
              if (!import_matrix_file (node_values_from_file_alpha, "node transparency")) {
                switch (node_alpha) {
                  case node_alpha_t::FIXED:       node_alpha_combobox->setCurrentIndex (0); return;
                  case node_alpha_t::CONNECTOME:  node_alpha_combobox->setCurrentIndex (1); return;
                  case node_alpha_t::FROM_LUT:    node_alpha_combobox->setCurrentIndex (2); return;
                  case node_alpha_t::VECTOR_FILE: node_alpha_combobox->setCurrentIndex (5); return;
                  case node_alpha_t::MATRIX_FILE: node_alpha_combobox->setCurrentIndex (5); return;
                }
              }
              node_alpha = node_alpha_t::MATRIX_FILE;
              if (node_alpha_combobox->count() == 5)
                node_alpha_combobox->addItem (node_values_from_file_alpha.get_name());
              else
                node_alpha_combobox->setItemText (5, node_values_from_file_alpha.get_name());
              node_alpha_combobox->setCurrentIndex (5);
              node_alpha_matrix_operator_combobox->setVisible (true);
              if (selected_node_count >= 2) {
                node_alpha_matrix_operator_combobox->removeItem (4);
                switch (node_alpha_matrix_operator) {
                  case node_property_matrix_operator_t::MIN:  node_alpha_matrix_operator_combobox->setCurrentIndex (0); break;
                  case node_property_matrix_operator_t::MEAN: node_alpha_matrix_operator_combobox->setCurrentIndex (1); break;
                  case node_property_matrix_operator_t::SUM:  node_alpha_matrix_operator_combobox->setCurrentIndex (2); break;
                  case node_property_matrix_operator_t::MAX:  node_alpha_matrix_operator_combobox->setCurrentIndex (3); break;
                }
                node_alpha_matrix_operator_combobox->setEnabled (true);
              } else {
                if (node_alpha_matrix_operator_combobox->count() == 4)
                  node_alpha_matrix_operator_combobox->addItem ("N/A");
                node_alpha_matrix_operator_combobox->setCurrentIndex (4);
                node_alpha_matrix_operator_combobox->setEnabled (false);
              }
              node_alpha_range_controls->setVisible (true);
              update_controls_node_alpha (node_values_from_file_alpha.get_min(), node_values_from_file_alpha.get_mean(), node_values_from_file_alpha.get_max());
              node_alpha_invert_checkbox->setChecked (false);
              break;
            case 5:
              return;
          }
          if (node_visibility == node_visibility_t::NONE)
            node_visibility_warning_icon->setVisible (true);
          calculate_node_alphas();
          window().updateGL();
        }



        void Connectome::node_visibility_matrix_operator_slot (int value)
        {
          switch (value) {
            case 0: node_visibility_matrix_operator = node_visibility_matrix_operator_t::ANY; break;
            case 1: node_visibility_matrix_operator = node_visibility_matrix_operator_t::ALL; break;
            default: assert (0); break;
          }
          calculate_node_visibility();
          window().updateGL();
        }
        void Connectome::node_visibility_parameter_slot()
        {
          calculate_node_visibility();
          window().updateGL();
        }
        void Connectome::sphere_lod_slot (int value)
        {
          sphere.LOD (value);
          node_visibility_warning_icon->setVisible (node_visibility == node_visibility_t::NONE);
          window().updateGL();
        }
        void Connectome::overlay_interp_slot (int)
        {
          assert (node_overlay);
          node_visibility_warning_icon->setVisible (node_visibility == node_visibility_t::NONE);
          node_overlay->set_interpolate (node_geometry_overlay_interp_checkbox->isChecked());
          window().updateGL();
        }
        void Connectome::node_colour_matrix_operator_slot (int value)
        {
          switch (value) {
            case 0: node_colour_matrix_operator = node_property_matrix_operator_t::MIN;  break;
            case 1: node_colour_matrix_operator = node_property_matrix_operator_t::MEAN; break;
            case 2: node_colour_matrix_operator = node_property_matrix_operator_t::SUM;  break;
            case 3: node_colour_matrix_operator = node_property_matrix_operator_t::MAX;  break;
            default: assert (0); break;
          }
          calculate_node_colours();
          window().updateGL();
        }
        void Connectome::node_fixed_colour_change_slot()
        {
          QColor c = node_colour_fixedcolour_button->color();
          node_fixed_colour = { c.red() / 255.0f, c.green() / 255.0f, c.blue() / 255.0f };
          node_visibility_warning_icon->setVisible (node_visibility == node_visibility_t::NONE);
          calculate_node_colours();
          window().updateGL();
        }
        void Connectome::node_colour_parameter_slot()
        {
          node_colour_lower_button->blockSignals (true);
          node_colour_upper_button->blockSignals (true);
          node_colour_lower_button->setMax (node_colour_upper_button->value());
          node_colour_upper_button->setMin (node_colour_lower_button->value());
          node_colour_lower_button->blockSignals (false);
          node_colour_upper_button->blockSignals (false);
          calculate_node_colours();
          window().updateGL();
        }
        void Connectome::node_size_matrix_operator_slot (int value)
        {
          switch (value) {
            case 0: node_size_matrix_operator = node_property_matrix_operator_t::MIN;  break;
            case 1: node_size_matrix_operator = node_property_matrix_operator_t::MEAN; break;
            case 2: node_size_matrix_operator = node_property_matrix_operator_t::SUM;  break;
            case 3: node_size_matrix_operator = node_property_matrix_operator_t::MAX;  break;
            default: assert (0); break;
          }
          calculate_node_sizes();
          window().updateGL();
        }
        void Connectome::node_size_value_slot()
        {
          node_size_scale_factor = node_size_button->value();
          window().updateGL();
        }
        void Connectome::node_size_parameter_slot()
        {
          node_size_lower_button->blockSignals (true);
          node_size_upper_button->blockSignals (true);
          node_size_lower_button->setMax (node_size_upper_button->value());
          node_size_upper_button->setMin (node_size_lower_button->value());
          node_size_lower_button->blockSignals (false);
          node_size_upper_button->blockSignals (false);
          calculate_node_sizes();
          window().updateGL();
        }
        void Connectome::node_alpha_matrix_operator_slot (int value)
        {
          switch (value) {
            case 0: node_alpha_matrix_operator = node_property_matrix_operator_t::MIN;  break;
            case 1: node_alpha_matrix_operator = node_property_matrix_operator_t::MEAN; break;
            case 2: node_alpha_matrix_operator = node_property_matrix_operator_t::SUM;  break;
            case 3: node_alpha_matrix_operator = node_property_matrix_operator_t::MAX;  break;
            default: assert (0); break;
          }
          calculate_node_alphas();
          window().updateGL();
        }
        void Connectome::node_alpha_value_slot (int position)
        {
          node_fixed_alpha = position / 1000.0f;
          if (node_overlay)
            node_overlay->alpha = node_fixed_alpha;
          window().updateGL();
        }
        void Connectome::node_alpha_parameter_slot()
        {
          node_alpha_lower_button->blockSignals (true);
          node_alpha_upper_button->blockSignals (true);
          node_alpha_lower_button->setMax (node_alpha_upper_button->value());
          node_alpha_upper_button->setMin (node_alpha_lower_button->value());
          node_alpha_lower_button->blockSignals (false);
          node_alpha_upper_button->blockSignals (false);
          calculate_node_alphas();
          window().updateGL();
        }







        void Connectome::edge_visibility_selection_slot (int index)
        {
          edge_visibility_warning_icon->setVisible (false);
          switch (index) {
            case 0:
              if (edge_visibility == edge_visibility_t::ALL) return;
              edge_visibility = edge_visibility_t::ALL;
              edge_visibility_combobox->removeItem (5);
              edge_visibility_threshold_controls->setVisible (false);
              break;
            case 1:
              if (edge_visibility == edge_visibility_t::NONE) return;
              edge_visibility = edge_visibility_t::NONE;
              edge_visibility_combobox->removeItem (5);
              edge_visibility_threshold_controls->setVisible (false);
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
              edge_visibility_combobox->removeItem (5);
              edge_visibility_threshold_controls->setVisible (false);
              break;
            case 3:
              if (edge_visibility == edge_visibility_t::CONNECTOME) return;
              edge_visibility = edge_visibility_t::CONNECTOME;
              edge_visibility_combobox->removeItem (5);
              edge_visibility_threshold_controls->setVisible (true);
              {
                float min = 0.0f, mean = 0.0f, max = 0.0f;
                QModelIndexList list = matrix_list_view->selectionModel()->selectedRows();
                if (list.size()) {
                  const FileDataVector& data (matrix_list_model->get (list[0]));
                  min = data.get_min(); mean = data.get_mean(); max = data.get_max();
                }
                update_controls_edge_visibility (min, mean, max);
              }
              break;
            case 4:
              if (!import_matrix_file (edge_values_from_file_visibility, "edge visibility")) {
                switch (edge_visibility) {
                  case edge_visibility_t::ALL:           edge_visibility_combobox->setCurrentIndex (0); return;
                  case edge_visibility_t::NONE:          edge_visibility_combobox->setCurrentIndex (1); return;
                  case edge_visibility_t::VISIBLE_NODES: edge_visibility_combobox->setCurrentIndex (2); return;
                  case edge_visibility_t::CONNECTOME:    edge_visibility_combobox->setCurrentIndex (3); return;
                  case edge_visibility_t::MATRIX_FILE:   edge_visibility_combobox->setCurrentIndex (5); return;
                }
              }
              edge_visibility = edge_visibility_t::MATRIX_FILE;
              if (edge_visibility_combobox->count() == 5)
                edge_visibility_combobox->addItem (edge_values_from_file_visibility.get_name());
              else
                edge_visibility_combobox->setItemText (5, edge_values_from_file_visibility.get_name());
              edge_visibility_combobox->setCurrentIndex (5);
              edge_visibility_threshold_controls->setVisible (true);
              update_controls_edge_visibility (edge_values_from_file_visibility.get_min(), edge_values_from_file_visibility.get_mean(), edge_values_from_file_visibility.get_max());
              break;
            case 5:
              return;
          }
          calculate_edge_visibility();
          window().updateGL();
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
          window().updateGL();
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
              edge_colour_combobox->removeItem (4);
              edge_colour_range_controls->setVisible (false);
              break;
            case 1:
              if (edge_colour == edge_colour_t::DIRECTION) return;
              edge_colour = edge_colour_t::DIRECTION;
              edge_colour_colourmap_button->setVisible (false);
              edge_colour_fixedcolour_button->setVisible (false);
              edge_colour_combobox->removeItem (4);
              edge_colour_range_controls->setVisible (false);
              break;
            case 2:
              if (edge_colour == edge_colour_t::CONNECTOME) return;
              edge_colour = edge_colour_t::CONNECTOME;
              edge_colour_colourmap_button->setVisible (true);
              edge_colour_fixedcolour_button->setVisible (false);
              edge_colour_combobox->removeItem (4);
              edge_colour_range_controls->setVisible (true);
              {
                float min = 0.0f, mean = 0.0f, max = 0.0f;
                QModelIndexList list = matrix_list_view->selectionModel()->selectedRows();
                if (list.size()) {
                  const FileDataVector& data (matrix_list_model->get (list[0]));
                  min = data.get_min(); mean = data.get_mean(); max = data.get_max();
                }
                update_controls_edge_colour (min, mean, max);
              }
              break;
            case 3:
              if (!import_matrix_file (edge_values_from_file_colour, "edge colours")) {
                switch (edge_colour) {
                  case edge_colour_t::FIXED:       edge_colour_combobox->setCurrentIndex (0); return;
                  case edge_colour_t::DIRECTION:   edge_colour_combobox->setCurrentIndex (1); return;
                  case edge_colour_t::CONNECTOME:  edge_colour_combobox->setCurrentIndex (2); return;
                  case edge_colour_t::MATRIX_FILE: edge_colour_combobox->setCurrentIndex (4); return;
                }
              }
              edge_colour = edge_colour_t::MATRIX_FILE;
              edge_colour_colourmap_button->setVisible (true);
              edge_colour_fixedcolour_button->setVisible (false);
              if (edge_colour_combobox->count() == 4)
                edge_colour_combobox->addItem (edge_values_from_file_colour.get_name());
              else
                edge_colour_combobox->setItemText (4, edge_values_from_file_colour.get_name());
              edge_colour_combobox->setCurrentIndex (4);
              edge_colour_range_controls->setVisible (true);
              update_controls_edge_colour (edge_values_from_file_colour.get_min(), edge_values_from_file_colour.get_mean(), edge_values_from_file_colour.get_max());
              break;
            case 4:
              return;
          }
          if (edge_visibility == edge_visibility_t::NONE)
            edge_visibility_warning_icon->setVisible (true);
          calculate_edge_colours();
          window().updateGL();
        }

        void Connectome::edge_size_selection_slot (int index)
        {
          edge_visibility_warning_icon->setVisible (false);
          switch (index) {
            case 0:
              if (edge_size == edge_size_t::FIXED) return;
              edge_size = edge_size_t::FIXED;
              edge_size_combobox->removeItem (3);
              edge_size_range_controls->setVisible (false);
              break;
            case 1:
              if (edge_size == edge_size_t::CONNECTOME) return;
              edge_size = edge_size_t::CONNECTOME;
              edge_size_combobox->removeItem (3);
              edge_size_range_controls->setVisible (true);
              {
                float min = 0.0f, mean = 0.0f, max = 0.0f;
                QModelIndexList list = matrix_list_view->selectionModel()->selectedRows();
                if (list.size()) {
                  const FileDataVector& data (matrix_list_model->get (list[0]));
                  min = data.get_min(); mean = data.get_mean(); max = data.get_max();
                }
                update_controls_edge_size (min, mean, max);
              }
              break;
            case 2:
              if (!import_matrix_file (edge_values_from_file_size, "edge size")) {
                switch (edge_size) {
                  case edge_size_t::FIXED:       edge_size_combobox->setCurrentIndex (0); return;
                  case edge_size_t::CONNECTOME:  edge_size_combobox->setCurrentIndex (1); return;
                  case edge_size_t::MATRIX_FILE: edge_size_combobox->setCurrentIndex (3); return;
                }
              }
              edge_size = edge_size_t::MATRIX_FILE;
              if (edge_size_combobox->count() == 3)
                edge_size_combobox->addItem (edge_values_from_file_size.get_name());
              else
                edge_size_combobox->setItemText (3, edge_values_from_file_size.get_name());
              edge_size_combobox->setCurrentIndex (3);
              edge_size_range_controls->setVisible (true);
              update_controls_edge_size (edge_values_from_file_size.get_min(), edge_values_from_file_size.get_mean(), edge_values_from_file_size.get_max());
              break;
            case 3:
              return;
          }
          if (edge_visibility == edge_visibility_t::NONE)
            edge_visibility_warning_icon->setVisible (true);
          calculate_edge_sizes();
          window().updateGL();
        }

        void Connectome::edge_alpha_selection_slot (int index)
        {
          edge_visibility_warning_icon->setVisible (false);
          switch (index) {
            case 0:
              if (edge_alpha == edge_alpha_t::FIXED) return;
              edge_alpha = edge_alpha_t::FIXED;
              edge_alpha_combobox->removeItem (3);
              edge_alpha_range_controls->setVisible (false);
              break;
            case 1:
              if (edge_alpha == edge_alpha_t::CONNECTOME) return;
              edge_alpha = edge_alpha_t::CONNECTOME;
              edge_alpha_combobox->removeItem (3);
              edge_alpha_range_controls->setVisible (true);
              {
                float min = 0.0f, mean = 0.0f, max = 0.0f;
                QModelIndexList list = matrix_list_view->selectionModel()->selectedRows();
                if (list.size()) {
                  const FileDataVector& data (matrix_list_model->get (list[0]));
                  min = data.get_min(); mean = data.get_mean(); max = data.get_max();
                }
                update_controls_edge_alpha (min, mean, max);
              }
              break;
            case 2:
              if (!import_matrix_file (edge_values_from_file_alpha, "edge transparency")) {
                switch (edge_alpha) {
                  case edge_alpha_t::FIXED:       edge_alpha_combobox->setCurrentIndex (0); return;
                  case edge_alpha_t::CONNECTOME:  edge_alpha_combobox->setCurrentIndex (1); return;
                  case edge_alpha_t::MATRIX_FILE: edge_alpha_combobox->setCurrentIndex (3); return;
                }
              }
              edge_alpha = edge_alpha_t::MATRIX_FILE;
              if (edge_alpha_combobox->count() == 3)
                edge_alpha_combobox->addItem (edge_values_from_file_alpha.get_name());
              else
                edge_alpha_combobox->setItemText (3, edge_values_from_file_alpha.get_name());
              edge_alpha_combobox->setCurrentIndex (3);
              edge_alpha_range_controls->setVisible (true);
              update_controls_edge_alpha (edge_values_from_file_alpha.get_min(), edge_values_from_file_alpha.get_mean(), edge_values_from_file_alpha.get_max());
              edge_alpha_invert_checkbox->setChecked (false);
              break;
            case 3:
              return;
          }
          if (edge_visibility == edge_visibility_t::NONE)
            edge_visibility_warning_icon->setVisible (true);
          calculate_edge_alphas();
          window().updateGL();
        }





        void Connectome::edge_visibility_parameter_slot()
        {
          calculate_edge_visibility();
          window().updateGL();
        }
        void Connectome::cylinder_lod_slot (int index)
        {
          cylinder.LOD (index);
          edge_visibility_warning_icon->setVisible (edge_visibility == edge_visibility_t::NONE);
          window().updateGL();
        }
        void Connectome::edge_colour_change_slot()
        {
          QColor c = edge_colour_fixedcolour_button->color();
          edge_fixed_colour = { c.red() / 255.0f, c.green() / 255.0f, c.blue() / 255.0f };
          edge_visibility_warning_icon->setVisible (edge_visibility == edge_visibility_t::NONE);
          calculate_edge_colours();
          window().updateGL();
        }
        void Connectome::edge_colour_parameter_slot()
        {
          edge_colour_lower_button->blockSignals (true);
          edge_colour_upper_button->blockSignals (true);
          edge_colour_lower_button->setMax (edge_colour_upper_button->value());
          edge_colour_upper_button->setMin (edge_colour_lower_button->value());
          edge_colour_lower_button->blockSignals (false);
          edge_colour_upper_button->blockSignals (false);
          calculate_edge_colours();
          window().updateGL();
        }
        void Connectome::edge_size_value_slot()
        {
          edge_size_scale_factor = edge_size_button->value();
          window().updateGL();
        }
        void Connectome::edge_size_parameter_slot()
        {
          edge_size_lower_button->blockSignals (true);
          edge_size_upper_button->blockSignals (true);
          edge_size_lower_button->setMax (edge_size_upper_button->value());
          edge_size_upper_button->setMin (edge_size_lower_button->value());
          edge_size_lower_button->blockSignals (false);
          edge_size_upper_button->blockSignals (false);
          calculate_edge_sizes();
          window().updateGL();
        }
        void Connectome::edge_alpha_value_slot (int position)
        {
          edge_fixed_alpha = position / 1000.0f;
          window().updateGL();
        }
        void Connectome::edge_alpha_parameter_slot()
        {
          edge_alpha_lower_button->blockSignals (true);
          edge_alpha_upper_button->blockSignals (true);
          edge_alpha_lower_button->setMax (edge_alpha_upper_button->value());
          edge_alpha_upper_button->setMin (edge_alpha_lower_button->value());
          edge_alpha_lower_button->blockSignals (false);
          edge_alpha_upper_button->blockSignals (false);
          calculate_edge_alphas();
          window().updateGL();
        }








        void Connectome::lut_open_slot ()
        {
          const std::string path = Dialog::File::get_file (this, std::string("Select lookup table file"));
          if (path.empty())
            return;
          lut.clear();
          try {
            lut.load (path);
          } catch (Exception& e) {
            e.display();
            return;
          }
          load_properties();
          window().updateGL();
        }
        void Connectome::lighting_change_slot (int /*value*/)
        {
          window().updateGL();
        }
        void Connectome::lighting_settings_slot()
        {
          if (!lighting_dock)
            lighting_dock.reset (new LightingDock ("Connectome lighting", lighting));
          lighting_dock->show();
        }
        void Connectome::lighting_parameter_slot()
        {
          if (use_lighting())
            window().updateGL();
        }
        void Connectome::crop_to_slab_toggle_slot (int /*value*/)
        {
          crop_to_slab = crop_to_slab_checkbox->isChecked();
          is_3D = !(crop_to_slab && !slab_thickness);
          crop_to_slab_label->setEnabled (crop_to_slab);
          crop_to_slab_button->setEnabled (crop_to_slab);
          node_geometry_overlay_3D_warning_icon->setVisible (node_geometry == node_geometry_t::OVERLAY && is_3D);
          window().updateGL();
        }
        void Connectome::crop_to_slab_parameter_slot()
        {
          slab_thickness = crop_to_slab_button->value();
          is_3D = !(crop_to_slab && !slab_thickness);
          node_geometry_overlay_3D_warning_icon->setVisible (node_geometry == node_geometry_t::OVERLAY && is_3D);
          window().updateGL();
        }
        void Connectome::show_node_list_slot()
        {
          node_list->show();
        }
        void Connectome::node_selection_settings_changed_slot()
        {
          window().updateGL();
        }










        void Connectome::clear_all()
        {
          image_button ->setText ("");
          lut_button->setText ("(none)");
          matrix_list_model->clear();
          selected_nodes.resize (0);
          selected_node_count = 0;
          if (node_visibility == node_visibility_t::CONNECTOME || node_visibility == node_visibility_t::VECTOR_FILE || node_visibility == node_visibility_t::MATRIX_FILE) {
            node_visibility_combobox->removeItem (6);
            node_visibility_combobox->setCurrentIndex (0);
            node_visibility = node_visibility_t::ALL;
          }
          if (node_colour == node_colour_t::CONNECTOME || node_colour == node_colour_t::VECTOR_FILE || node_colour == node_colour_t::MATRIX_FILE) {
            node_colour_combobox->removeItem (6);
            node_colour_combobox->setCurrentIndex (0);
            node_colour = node_colour_t::FIXED;
          }
          if (node_size == node_size_t::CONNECTOME || node_size == node_size_t::VECTOR_FILE || node_size == node_size_t::MATRIX_FILE) {
            node_size_combobox->removeItem (5);
            node_size_combobox->setCurrentIndex (0);
            node_size = node_size_t::FIXED;
          }
          if (node_alpha == node_alpha_t::CONNECTOME || node_alpha == node_alpha_t::VECTOR_FILE || node_alpha == node_alpha_t::MATRIX_FILE) {
            node_alpha_combobox->removeItem (5);
            node_alpha_combobox->setCurrentIndex (0);
            node_alpha = node_alpha_t::FIXED;
          }
          if (edge_visibility == edge_visibility_t::MATRIX_FILE) {
            edge_visibility_combobox->removeItem (5);
            edge_visibility_combobox->setCurrentIndex (3);
            edge_visibility = edge_visibility_t::CONNECTOME;
          }
          if (edge_colour == edge_colour_t::MATRIX_FILE) {
            edge_colour_combobox->removeItem (4);
            edge_colour_combobox->setCurrentIndex (2);
            edge_colour = edge_colour_t::CONNECTOME;
          }
          if (edge_size == edge_size_t::MATRIX_FILE) {
            edge_size_combobox->removeItem (3);
            edge_size_combobox->setCurrentIndex (0);
            edge_size = edge_size_t::FIXED;
          }
          if (edge_alpha == edge_alpha_t::MATRIX_FILE) {
            edge_alpha_combobox->removeItem (3);
            edge_alpha_combobox->setCurrentIndex (0);
            edge_alpha = edge_alpha_t::FIXED;
          }
          if (buffer)
            delete buffer.release();
          nodes.clear();
          edges.clear();
          lut.clear();
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
          node_geometry_overlay_3D_warning_icon->setVisible (false);
          edge_visibility_warning_icon->setVisible (false);
        }

        void Connectome::enable_all (const bool value)
        {
          lut_button->setEnabled (value);

          lighting_checkbox->setEnabled (value);
          lighting_settings_button->setEnabled (value);
          crop_to_slab_checkbox->setEnabled (value);
          crop_to_slab_label->setEnabled (value && crop_to_slab);
          crop_to_slab_button->setEnabled (value && crop_to_slab);
          show_node_list_button->setEnabled (value);

          matrix_open_button->setEnabled (value);
          matrix_close_button->setEnabled (value);
          matrix_list_view->setEnabled (value);

          node_visibility_combobox->setEnabled (value);
          node_visibility_threshold_button->setEnabled (value);
          node_visibility_threshold_invert_checkbox->setEnabled (value);

          node_geometry_combobox->setEnabled (value);
          node_geometry_sphere_lod_spinbox->setEnabled (value);
          node_geometry_overlay_interp_checkbox->setEnabled (value);
          node_geometry_overlay_3D_warning_icon->setEnabled (value);

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
          MR::Header H = MR::Header::open (path);
          if (!H.datatype().is_integer())
            throw Exception ("Input parcellation image must have an integer datatype; try running mrconvert -datatype uint32");
          if (H.ndim() != 3)
            throw Exception ("Input parcellation image must be a 3D image");
          voxel_volume = H.spacing(0) * H.spacing(1) * H.spacing(2);
          {
            // Prevent progress dialog from appearing in a multi-threading context
            LogLevelLatch latch (0);
            buffer.reset (new MR::Image<node_t> (H.get_image<node_t>().with_direct_io()));
          }
          MR::Transform transform (H);
          std::vector<Eigen::Vector3f> node_coms;
          std::vector<size_t> node_volumes;
          std::vector<Eigen::Array3i> node_lower_corners, node_upper_corners;
          size_t max_index = 0;

          {
            for (auto loop = Loop(*buffer) (*buffer); loop; ++loop) {
              const node_t node_index = buffer->value();
              if (node_index) {

                if (node_index >= max_index) {
                  node_coms         .resize (node_index+1, Eigen::Vector3f { 0.0f, 0.0f, 0.0f });
                  node_volumes      .resize (node_index+1, 0);
                  node_lower_corners.resize (node_index+1, Eigen::Array3i { int(H.size(0)), int(H.size(1)), int(H.size(2)) });
                  node_upper_corners.resize (node_index+1, Eigen::Array3i { -1, -1, -1 });
                  max_index = node_index;
                }

                const Eigen::Vector3f voxel { float(buffer->index(0)), float(buffer->index(1)), float(buffer->index(2)) };
                node_coms   [node_index] += transform.voxel2scanner.cast<float>() * voxel;
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
          const size_t pixheight = dynamic_cast<Node_list*>(node_list->tool)->row_height();

          {
            nodes.push_back (Node());
            for (size_t node_index = 1; node_index <= max_index; ++node_index) {
              if (node_volumes[node_index]) {

                std::vector<int> from (3), dim (3);
                for (size_t axis = 0; axis != 3; ++axis) {
                  from[axis] = node_lower_corners[node_index][axis];
                  dim[axis] = node_upper_corners[node_index][axis] - node_lower_corners[node_index][axis] + 1;
                }
                MR::Adapter::Subset< MR::Image<node_t> > subset (*buffer, from, dim);
                MR::Image<bool> node_mask (MR::Image<bool>::scratch (subset, "Node " + str(node_index) + " mask"));

                auto copy_func = [&] (const decltype(subset)& in, decltype(node_mask)& out) { out.value() = (in.value() == node_index); };
                MR::ThreadedLoop (subset).run (copy_func, subset, node_mask);

                nodes.push_back (Node (node_coms[node_index], node_volumes[node_index], pixheight, node_mask));

              } else {
                nodes.push_back (Node());
              }
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
          MR::Header H_overlay (H);
          H_overlay.ndim() = 4;
          H_overlay.size (3) = 4; // RGBA
          H_overlay.stride (0) = 2;
          H_overlay.stride (1) = 3;
          H_overlay.stride (2) = 4;
          H_overlay.stride (3) = 1;
          H_overlay.sanitise();
          node_overlay.reset (new NodeOverlay (std::move (H_overlay)));
          update_node_overlay();

          selected_nodes.resize (num_nodes()+1);

          dynamic_cast<Node_list*>(node_list->tool)->initialize();
        }

        void Connectome::add_matrices (const std::vector<std::string>& list)
        {
          std::vector<FileDataVector> data;
          for (size_t i = 0; i < list.size(); ++i) {
            try {
              MR::Connectome::matrix_type matrix = MR::load_matrix<default_type> (list[i]);
              MR::Connectome::verify_matrix (matrix, num_nodes());
              FileDataVector temp;
              mat2vec (matrix, temp);
              temp.calc_stats();
              temp.set_name (list[i]);
              data.push_back (std::move (temp));
            }
            catch (Exception& E) {
              E.display();
            }
          }

          if (data.size()) {
            const size_t previous_size = matrix_list_model->rowCount();
            matrix_list_model->add_items (data);
            QModelIndex first = matrix_list_model->index (previous_size, 0, QModelIndex());
            matrix_list_view->selectionModel()->select (first, QItemSelectionModel::ClearAndSelect);
            // In the specific case where there was previously no connectome data,
            //   it is necessary to set the min/max/value of various controls
            if (!previous_size) {
              const FileDataVector& data (matrix_list_model->get (previous_size));
              if (node_visibility == node_visibility_t::CONNECTOME)
                update_controls_node_visibility (data.get_min(), data.get_mean(), data.get_max());
              if (node_colour == node_colour_t::CONNECTOME)
                update_controls_node_colour (data.get_min(), data.get_mean(), data.get_max());
              if (node_size == node_size_t::CONNECTOME)
                update_controls_node_size (data.get_min(), data.get_mean(), data.get_max());
              if (node_alpha == node_alpha_t::CONNECTOME)
                update_controls_node_alpha (data.get_min(), data.get_mean(), data.get_max());
              if (edge_visibility == edge_visibility_t::CONNECTOME)
                update_controls_edge_visibility (data.get_min(), data.get_mean(), data.get_max());
              if (edge_colour == edge_colour_t::CONNECTOME)
                update_controls_edge_colour (data.get_min(), data.get_mean(), data.get_max());
              if (edge_size == edge_size_t::CONNECTOME)
                update_controls_edge_size (data.get_min(), data.get_mean(), data.get_max());
              if (edge_alpha == edge_alpha_t::CONNECTOME)
                update_controls_edge_alpha (data.get_min(), data.get_mean(), data.get_max());
            }

            // Force re-calculation of any visual properties that are based on connectome data
            connectome_selection_changed_slot (QItemSelection(), QItemSelection());
          }
        }





        void Connectome::draw_nodes (const Projection& projection)
        {
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;
          if (node_visibility != node_visibility_t::NONE) {

            if (node_geometry == node_geometry_t::OVERLAY) {

              if (is_3D) {
                //
                //window().get_current_mode()->overlays_for_3D.push_back (node_overlay.get());
                // FIXME Need a better approach for displaying the node overlay image in 3D
                // Can't rely on the volume shader; requires user to change mode, doesn't
                //   support alpha channel, conflicts with connectome tool manual configuration
                //   of 2D / 3D, wouldn't support slab crop
                //
                // Is there anything better that can be done? Something like a volume render, but
                //   instead of accumulating values along the ray, do a Bresenham test to find
                //   voxels intersected by the ray. Go back to front, and render each node only
                //   once per fragment. Can use transparency.
                //
              } else {
                // set up OpenGL environment:
                gl::Enable (gl::BLEND);
                gl::Disable (gl::DEPTH_TEST);
                gl::DepthMask (gl::FALSE_);
                gl::ColorMask (gl::TRUE_, gl::TRUE_, gl::TRUE_, gl::TRUE_);
                gl::BlendFunc (gl::SRC_ALPHA, gl::ONE_MINUS_SRC_ALPHA);
                gl::BlendEquation (gl::FUNC_ADD);

                node_overlay->render3D (node_overlay->slice_shader, projection, projection.depth_of (window().focus()));

                // restore OpenGL environment:
                gl::Disable (gl::BLEND);
                gl::Enable (gl::DEPTH_TEST);
                gl::DepthMask (gl::TRUE_);
              }

            } else {

              node_shader.start (*this);
              projection.set (node_shader);

              const bool alpha = use_alpha_nodes();

              gl::Enable (gl::DEPTH_TEST);
              if (alpha) {
                gl::Enable (gl::BLEND);
                gl::DepthMask (gl::FALSE_);
                gl::BlendEquation (gl::FUNC_ADD);
                //gl::BlendFunc (gl::SRC_ALPHA, gl::ONE_MINUS_SRC_ALPHA);
                //gl::BlendFunc (gl::SRC_ALPHA, gl::DST_ALPHA);
                gl::BlendFuncSeparate (gl::SRC_ALPHA, gl::ONE_MINUS_SRC_ALPHA, gl::SRC_ALPHA, gl::DST_ALPHA);
                gl::BlendColor (1.0, 1.0, 1.0, node_fixed_alpha);
              } else {
                gl::Disable (gl::BLEND);
                 if (is_3D)
                  gl::DepthMask (gl::TRUE_);
                else
                  gl::DepthMask (gl::FALSE_);
              }

              const GLuint node_colour_ID = gl::GetUniformLocation (node_shader, "node_colour");

              GLuint node_alpha_ID = 0;
              if (alpha)
                node_alpha_ID = gl::GetUniformLocation (node_shader, "node_alpha");

              const GLuint node_centre_ID = gl::GetUniformLocation (node_shader, "node_centre");
              const GLuint node_size_ID = gl::GetUniformLocation (node_shader, "node_size");

              if (node_colour == node_colour_t::VECTOR_FILE && ColourMap::maps[node_colourmap_index].is_colour)
                gl::Uniform3fv (gl::GetUniformLocation (node_shader, "colourmap_colour"), 1, node_fixed_colour.data());

              if (node_geometry == node_geometry_t::SPHERE) {
                sphere.vertex_buffer.bind (gl::ARRAY_BUFFER);
                sphere_VAO.bind();
                sphere.index_buffer.bind();
              } else if (node_geometry == node_geometry_t::CUBE) {
                cube.vertex_buffer.bind (gl::ARRAY_BUFFER);
                cube.normals_buffer.bind (gl::ARRAY_BUFFER);
                cube_VAO.bind();
                cube.index_buffer.bind();
              }

              GLuint specular_ID = 0;
              if (use_lighting()) {
                gl::UniformMatrix4fv (gl::GetUniformLocation (node_shader, "MV"), 1, gl::FALSE_, projection.modelview());
                gl::Uniform3fv (gl::GetUniformLocation (node_shader, "light_pos"), 1, lighting.lightpos);
                gl::Uniform1f  (gl::GetUniformLocation (node_shader, "ambient"), lighting.ambient);
                gl::Uniform1f  (gl::GetUniformLocation (node_shader, "diffuse"), lighting.diffuse);
                specular_ID = gl::GetUniformLocation (node_shader, "specular");
                gl::Uniform1f  (specular_ID, lighting.specular);
                gl::Uniform1f  (gl::GetUniformLocation (node_shader, "shine"), lighting.shine);
              }

              if (crop_to_slab) {
                gl::Uniform3fv (gl::GetUniformLocation (node_shader, "screen_normal"), 1, projection.screen_normal().data());
                if (is_3D) {
                  gl::Uniform1f (gl::GetUniformLocation (node_shader, "slab_thickness"), slab_thickness);
                  gl::Uniform1f (gl::GetUniformLocation (node_shader, "crop_var"), window().focus().dot (projection.screen_normal()) - slab_thickness / 2.0f);
                } else {
                  gl::Uniform1f (gl::GetUniformLocation (node_shader, "depth_offset"), window().focus().dot (projection.screen_normal()));
                }
              }

              std::map<float, size_t> node_ordering;
              for (size_t i = 1; i <= num_nodes(); ++i)
                node_ordering.insert (std::make_pair (projection.depth_of (nodes[i].get_com()), i));

              for (auto it = node_ordering.rbegin(); it != node_ordering.rend(); ++it) {
                const Node& node (nodes[it->second]);
                if (node_visibility_given_selection (it->second)) {
                  gl::Uniform3fv (node_colour_ID, 1, node_colour_given_selection (it->second).data());
                  if (alpha)
                    gl::Uniform1f (node_alpha_ID, node_alpha_given_selection (it->second) * node_fixed_alpha);
                  gl::Uniform3fv (node_centre_ID, 1, node.get_com().data());
                  gl::Uniform1f (node_size_ID, node_size_given_selection (it->second) * node_size_scale_factor);
                  switch (node_geometry) {
                    case node_geometry_t::SPHERE:
                      if (alpha) {
                        gl::CullFace (gl::FRONT);
                        gl::Uniform1f  (specular_ID, (1.0 - node_alpha_given_selection (it->second) * node_fixed_alpha) * lighting.specular);
                        gl::DrawElements (gl::TRIANGLES, sphere.num_indices, gl::UNSIGNED_INT, (void*)0);
                        gl::CullFace (gl::BACK);
                        gl::Uniform1f  (specular_ID, lighting.specular);
                      }
                      gl::DrawElements (gl::TRIANGLES, sphere.num_indices, gl::UNSIGNED_INT, (void*)0);
                      break;
                    case node_geometry_t::CUBE:
                      if (alpha) {
                        gl::CullFace (gl::FRONT);
                        gl::Uniform1f  (specular_ID, (1.0 - node_alpha_given_selection (it->second) * node_fixed_alpha) * lighting.specular);
                        gl::DrawElements (gl::TRIANGLES, cube.num_indices, gl::UNSIGNED_INT, (void*)0);
                        gl::CullFace (gl::BACK);
                        gl::Uniform1f  (specular_ID, lighting.specular);
                      }
                      gl::DrawElements (gl::TRIANGLES, cube.num_indices, gl::UNSIGNED_INT, (void*)0);
                      break;
                    case node_geometry_t::OVERLAY:
                      assert (0);
                      break;
                    case node_geometry_t::MESH:
                      if (alpha) {
                        gl::CullFace (gl::FRONT);
                        gl::Uniform1f  (specular_ID, (1.0 - node_alpha_given_selection (it->second) * node_fixed_alpha) * lighting.specular);
                        node.render_mesh();
                        gl::CullFace (gl::BACK);
                        gl::Uniform1f  (specular_ID, lighting.specular);
                      }
                      node.render_mesh();
                      break;
                  }
                }
              }

              // Reset to defaults if we've been doing transparency
              if (alpha) {
                gl::Disable (gl::BLEND);
                gl::DepthMask (gl::TRUE_);
              }

              node_shader.stop();
            }

          }

          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;
        }

        void Connectome::draw_edges (const Projection& projection)
        {
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;
          if (edge_visibility != edge_visibility_t::NONE) {

            edge_shader.start (*this);
            projection.set (edge_shader);

            bool alpha = use_alpha_edges();

            gl::Enable (gl::DEPTH_TEST);
            if (alpha) {
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
              gl::Enable (gl::LINE_SMOOTH);

            GLuint node_centre_one_ID = 0, node_centre_two_ID = 0, rot_matrix_ID = 0;
            if (edge_geometry == edge_geometry_t::CYLINDER) {
              cylinder.vertex_buffer.bind (gl::ARRAY_BUFFER);
              cylinder_VAO.bind();
              cylinder.index_buffer.bind();
              node_centre_one_ID = gl::GetUniformLocation (edge_shader, "centre_one");
              node_centre_two_ID = gl::GetUniformLocation (edge_shader, "centre_two");
              rot_matrix_ID      = gl::GetUniformLocation (edge_shader, "rot_matrix");
            }

            GLuint radius_ID = 0;
            if (edge_geometry == edge_geometry_t::CYLINDER || edge_geometry == edge_geometry_t::STREAMTUBE)
              radius_ID = gl::GetUniformLocation (edge_shader, "radius");

            GLuint specular_ID = 0;
            if (use_lighting()) {
              gl::UniformMatrix4fv (gl::GetUniformLocation (edge_shader, "MV"), 1, gl::FALSE_, projection.modelview());
              gl::Uniform3fv (gl::GetUniformLocation (edge_shader, "light_pos"), 1, lighting.lightpos);
              gl::Uniform1f  (gl::GetUniformLocation (edge_shader, "ambient"), lighting.ambient);
              gl::Uniform1f  (gl::GetUniformLocation (edge_shader, "diffuse"), lighting.diffuse);
              specular_ID   = gl::GetUniformLocation (edge_shader, "specular");
              gl::Uniform1f  (specular_ID, lighting.specular);
              gl::Uniform1f  (gl::GetUniformLocation (edge_shader, "shine"), lighting.shine);
            }

            if (crop_to_slab) {
              gl::Uniform3fv (gl::GetUniformLocation (edge_shader, "screen_normal"), 1, projection.screen_normal().data());
              if (is_3D) {
                gl::Uniform1f (gl::GetUniformLocation (edge_shader, "slab_thickness"), slab_thickness);
                gl::Uniform1f (gl::GetUniformLocation (edge_shader, "crop_var"), window().focus().dot (projection.screen_normal()) - slab_thickness / 2.0f);
              } else {
                gl::Uniform1f (gl::GetUniformLocation (edge_shader, "depth_offset"), window().focus().dot (projection.screen_normal()));
              }
            }

            const GLuint edge_colour_ID = gl::GetUniformLocation (edge_shader, "edge_colour");

            GLuint edge_alpha_ID = 0;
            if (alpha)
              edge_alpha_ID = gl::GetUniformLocation (edge_shader, "edge_alpha");

            if ((edge_colour == edge_colour_t::CONNECTOME || edge_colour == edge_colour_t::MATRIX_FILE) && ColourMap::maps[edge_colourmap_index].is_colour)
              gl::Uniform3fv (gl::GetUniformLocation (edge_shader, "colourmap_colour"), 1, edge_fixed_colour.data());

            std::map<float, size_t> edge_ordering;
            for (size_t i = 0; i != num_edges(); ++i)
              edge_ordering.insert (std::make_pair (projection.depth_of (edges[i].get_com()), i));

            for (auto it = edge_ordering.rbegin(); it != edge_ordering.rend(); ++it) {
              const Edge& edge (edges[it->second]);
              if (edge_visibility_given_selection (edge)) {
                gl::Uniform3fv (edge_colour_ID, 1, edge_colour_given_selection (edge).data());
                if (alpha)
                  gl::Uniform1f (edge_alpha_ID, edge_alpha_given_selection (edge) * edge_fixed_alpha);
                switch (edge_geometry) {
                  case edge_geometry_t::LINE:
                    gl::LineWidth (calc_line_width (edge_size_given_selection (edge) * edge_size_scale_factor, edge_geometry_line_smooth_checkbox->isChecked()));
                    edge.render_line();
                    break;
                  case edge_geometry_t::CYLINDER:
                    gl::Uniform3fv       (node_centre_one_ID, 1,        edge.get_node_centre(0).data());
                    gl::Uniform3fv       (node_centre_two_ID, 1,        edge.get_node_centre(1).data());
                    gl::UniformMatrix3fv (rot_matrix_ID,      1, false, edge.get_rot_matrix());
                    gl::Uniform1f        (radius_ID,                    std::sqrt (edge_size_given_selection (edge) * edge_size_scale_factor / Math::pi));
                    if (alpha) {
                      gl::CullFace (gl::FRONT);
                      gl::Uniform1f  (specular_ID, (1.0 - edge_alpha_given_selection (edge) * edge_fixed_alpha) * lighting.specular);
                      gl::DrawElements (gl::TRIANGLES, cylinder.num_indices, gl::UNSIGNED_INT, (void*)0);
                      gl::CullFace (gl::BACK);
                      gl::Uniform1f  (specular_ID, lighting.specular);
                    }
                    gl::DrawElements (gl::TRIANGLES, cylinder.num_indices, gl::UNSIGNED_INT, (void*)0);
                    break;
                  case edge_geometry_t::STREAMLINE:
                    gl::LineWidth (calc_line_width (edge_size_given_selection (edge) * edge_size_scale_factor, edge_geometry_line_smooth_checkbox->isChecked()));
                    edge.render_streamline();
                    break;
                  case edge_geometry_t::STREAMTUBE:
                    gl::Uniform1f (radius_ID, std::sqrt (edge_size_given_selection (edge) * edge_size_scale_factor / Math::pi));
                    if (alpha) {
                      gl::CullFace (gl::FRONT);
                      gl::Uniform1f  (specular_ID, (1.0 - edge_alpha_given_selection (edge) * edge_fixed_alpha) * lighting.specular);
                      edge.render_streamtube();
                      gl::CullFace (gl::BACK);
                      gl::Uniform1f  (specular_ID, lighting.specular);
                    }
                    edge.render_streamtube();
                }
              }
            }

            // Reset to defaults if we've been doing transparency
            if (alpha) {
              gl::Disable (gl::BLEND);
              gl::DepthMask (gl::TRUE_);
            }

            if (edge_geometry == edge_geometry_t::LINE || edge_geometry == edge_geometry_t::STREAMLINE) {
              gl::LineWidth (1.0f);
              if (edge_geometry_line_smooth_checkbox->isChecked())
                gl::Disable (gl::LINE_SMOOTH);
            }

            edge_shader.stop();
          }
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;
        }






        bool Connectome::import_vector_file (FileDataVector& data, const std::string& attribute)
        {
          const std::string path = Dialog::File::get_file (this, "Select vector file to determine " + attribute, "Data files (*.csv)");
          if (path.empty())
            return false;
          try {
            FileDataVector prev_data (data);
            data.clear();
            data.load (path);
            const size_t numel = data.size();
            if (data.size() != num_nodes()) {
              // Restore data in case user is trying to change from one file to another
              data = std::move (prev_data);
              throw Exception ("File " + Path::basename (path) + " contains " + str (numel) + " elements, but connectome has " + str(num_nodes()) + " nodes");
            }
            data.set_name (Path::basename (path));
            return true;
          } catch (Exception& e) {
            e.display();
            data.clear();
            return false;
          }
        }

        bool Connectome::import_matrix_file (FileDataVector& data, const std::string& attribute)
        {
          const std::string path = Dialog::File::get_file (this, "Select matrix file to determine " + attribute, "Data files (*.csv)");
          if (path.empty())
            return false;
          MR::Connectome::matrix_type temp;
          try {
            temp = MR::load_matrix<default_type> (path);
            MR::Connectome::verify_matrix (temp, num_nodes());
          } catch (Exception& e) {
            e.display();
            return false;
          }
          data.clear();
          mat2vec (temp, data);
          data.calc_stats();
          data.set_name (Path::basename (path));
          return true;
        }






        void Connectome::load_properties()
        {
          if (lut.empty()) {
            // Create LUT entries for nodes with non-zero volume
            for (node_t node_index = 1; node_index <= num_nodes(); ++node_index) {
              if (nodes[node_index].get_volume()) {
                const std::string name = "Node " + str(node_index);
                nodes[node_index].set_name (name);
                lut.insert (std::make_pair (node_index, MR::Connectome::LUT_node (name)));
              }
            }
          } else {
            // Load the node names from the LUT
            // Other properties will only be pulled from the LUT if requested
            for (node_t node_index = 1; node_index <= num_nodes(); ++node_index) {
              const size_t count = lut.count (node_index);
              if (count) {
                if (count > 1)
                  throw Exception ("Duplicate entries in lookup table file for index " + str(node_index));
                nodes[node_index].set_name (lut.find(node_index)->second.get_name());
              }
            }
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

          } else if (node_visibility == node_visibility_t::DEGREE) {

            for (auto i = nodes.begin(); i != nodes.end(); ++i)
              i->set_visible (false);
            for (auto i = edges.begin(); i != edges.end(); ++i) {
              if (i->to_draw()) {
                nodes[i->get_node_index(0)].set_visible (true);
                nodes[i->get_node_index(1)].set_visible (true);
              }
            }

          } else if (node_visibility == node_visibility_t::CONNECTOME) {

            QModelIndexList list = matrix_list_view->selectionModel()->selectedRows();
            if (list.size() && selected_node_count) {

              const FileDataVector& data (matrix_list_model->get (list[0]));

              const bool invert = node_visibility_threshold_invert_checkbox->isChecked();
              const float threshold = node_visibility_threshold_button->value();
              for (node_t i = 1; i <= num_nodes(); ++i) {
                bool any = false, all = true;
                for (node_t j = 1; j <= num_nodes(); ++j) {
                  if (selected_nodes[j]) {
                    const float value = data[mat2vec (i-1, j-1)];
                    if (value >= threshold)
                      any = true;
                    else
                      all = false;
                  }
                }
                switch (node_visibility_matrix_operator) {
                  case node_visibility_matrix_operator_t::ANY: nodes[i].set_visible (any != invert); break;
                  case node_visibility_matrix_operator_t::ALL: nodes[i].set_visible (all != invert); break;
                }
              }

            } else {
              for (node_t i = 1; i <= num_nodes(); ++i)
                nodes[i].set_visible (true);
            }

          } else if (node_visibility == node_visibility_t::VECTOR_FILE) {

            assert (node_values_from_file_visibility.size() == num_nodes());
            const bool invert = node_visibility_threshold_invert_checkbox->isChecked();
            const float threshold = node_visibility_threshold_button->value();
            for (node_t i = 1; i <= num_nodes(); ++i) {
              const bool above_threshold = (node_values_from_file_visibility[i-1] >= threshold);
              nodes[i].set_visible (above_threshold != invert);
            }

          } else if (node_visibility == node_visibility_t::MATRIX_FILE) {

            assert (size_t(node_values_from_file_visibility.size()) == num_edges());

            if (selected_node_count) {
              const bool invert = node_visibility_threshold_invert_checkbox->isChecked();
              const float threshold = node_visibility_threshold_button->value();
              for (node_t i = 1; i <= num_nodes(); ++i) {
                bool any = false, all = true;
                for (node_t j = 1; j <= num_nodes(); ++j) {
                  if (selected_nodes[j]) {
                    const float value = node_values_from_file_visibility[mat2vec (i-1, j-1)];
                    if (value >= threshold)
                      any = true;
                    else
                      all = false;
                  }
                }
                switch (node_visibility_matrix_operator) {
                  case node_visibility_matrix_operator_t::ANY: nodes[i].set_visible (any != invert); break;
                  case node_visibility_matrix_operator_t::ALL: nodes[i].set_visible (all != invert); break;
                }
              }
            } else {
              for (node_t i = 1; i <= num_nodes(); ++i)
                nodes[i].set_visible (true);
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

            Eigen::Array3f rgb;
            Math::RNG::Uniform<float> rng;
            for (auto i = nodes.begin(); i != nodes.end(); ++i) {
              do {
                rgb = { rng(), rng(), rng() };
              } while (rgb[0] < 0.5 && rgb[1] < 0.5 && rgb[2] < 0.5);
              i->set_colour (rgb);
            }

          } else if (node_colour == node_colour_t::FROM_LUT) {

            if (lut.size()) {
              for (node_t node_index = 1; node_index <= num_nodes(); ++node_index) {
                const LUT::const_iterator i = lut.find (node_index);
                if (i == lut.end())
                  nodes[node_index].set_colour (node_fixed_colour);
                else
                  nodes[node_index].set_colour (Eigen::Array3f (i->second.get_colour().cast<float>()) / 255.0f);
              }
            } else {
              for (auto i = nodes.begin(); i != nodes.end(); ++i)
                i->set_colour (node_fixed_colour);
            }

          } else if (node_colour == node_colour_t::CONNECTOME) {

            QModelIndexList list = matrix_list_view->selectionModel()->selectedRows();
            if (list.size() && selected_node_count) {

              const FileDataVector& data (matrix_list_model->get (list[0]));
              const float lower = node_colour_lower_button->value(), upper = node_colour_upper_button->value();
              for (node_t i = 1; i <= num_nodes(); ++i) {
                if (selected_nodes[i]) {
                  nodes[i].set_colour (node_selection_settings.get_node_selected_colour());
                } else {
                  float min = std::numeric_limits<float>::infinity(), sum = 0.0f, max = -std::numeric_limits<float>::infinity();
                  for (node_t j = 1; j <= num_nodes(); ++j) {
                    if (selected_nodes[j]) {
                      const float value = data[mat2vec (i-1, j-1)];
                      min = std::min (min, value);
                      sum += value;
                      max = std::max (max, value);
                    }
                  }
                  const float mean = sum / float(selected_node_count);
                  float factor = 0.0f;
                  switch (node_colour_matrix_operator) {
                    case node_property_matrix_operator_t::MIN:  factor = min;  break;
                    case node_property_matrix_operator_t::MEAN: factor = mean; break;
                    case node_property_matrix_operator_t::SUM:  factor = sum;  break;
                    case node_property_matrix_operator_t::MAX:  factor = max;  break;
                  }
                  factor = (factor - lower) / (upper - lower);
                  factor = std::min (1.0f, std::max (factor, 0.0f));
                  factor = node_colourmap_invert ? 1.0f-factor : factor;
                  if (ColourMap::maps[node_colourmap_index].is_colour)
                    nodes[i].set_colour (factor * node_fixed_colour);
                  else
                    nodes[i].set_colour (ColourMap::maps[node_colourmap_index].basic_mapping (factor));
                }
              }

            } else {
              for (node_t i = 1; i <= num_nodes(); ++i)
                nodes[i].set_colour (node_fixed_colour);
            }

          } else if (node_colour == node_colour_t::VECTOR_FILE) {

            assert (node_values_from_file_colour.size() == num_nodes());
            const float lower = node_colour_lower_button->value(), upper = node_colour_upper_button->value();
            for (node_t i = 1; i <= num_nodes(); ++i) {
              float factor = (node_values_from_file_colour[i-1]-lower) / (upper - lower);
              factor = std::min (1.0f, std::max (factor, 0.0f));
              factor = node_colourmap_invert ? 1.0f-factor : factor;
              if (ColourMap::maps[node_colourmap_index].is_colour)
                nodes[i].set_colour (factor * node_fixed_colour);
              else
                nodes[i].set_colour (ColourMap::maps[node_colourmap_index].basic_mapping (factor));
            }

          } else if (node_colour == node_colour_t::MATRIX_FILE) {

            assert (size_t(node_values_from_file_colour.size()) == num_edges());
            if (selected_node_count) {
              const float lower = node_colour_lower_button->value(), upper = node_colour_upper_button->value();
              for (node_t i = 1; i <= num_nodes(); ++i) {
                if (selected_nodes[i]) {
                  nodes[i].set_colour (node_selection_settings.get_node_selected_colour());
                } else {
                  float min = std::numeric_limits<float>::infinity(), sum = 0.0f, max = -std::numeric_limits<float>::infinity();
                  for (node_t j = 1; j <= num_nodes(); ++j) {
                    if (selected_nodes[j]) {
                      const float value = node_values_from_file_colour[mat2vec (i-1, j-1)];
                      min = std::min (min, value);
                      sum += value;
                      max = std::max (max, value);
                    }
                  }
                  const float mean = sum / float(selected_node_count);
                  float factor = 0.0f;
                  switch (node_colour_matrix_operator) {
                    case node_property_matrix_operator_t::MIN:  factor = min;  break;
                    case node_property_matrix_operator_t::MEAN: factor = mean; break;
                    case node_property_matrix_operator_t::SUM:  factor = sum;  break;
                    case node_property_matrix_operator_t::MAX:  factor = max;  break;
                  }
                  factor = (factor - lower) / (upper - lower);
                  factor = std::min (1.0f, std::max (factor, 0.0f));
                  factor = node_colourmap_invert ? 1.0f-factor : factor;
                  if (ColourMap::maps[node_colourmap_index].is_colour)
                    nodes[i].set_colour (factor * node_fixed_colour);
                  else
                    nodes[i].set_colour (ColourMap::maps[node_colourmap_index].basic_mapping (factor));
                }
              }
            } else {
              for (node_t i = 1; i <= num_nodes(); ++i)
                nodes[i].set_colour (node_fixed_colour);
            }

          }
          update_node_overlay();
          // Need to indicate to the node list view that data have changed (specifically the node colour pixmaps)
          dynamic_cast<Node_list*>(node_list->tool)->colours_changed();
        }

        void Connectome::calculate_node_sizes()
        {
          if (node_size == node_size_t::FIXED) {

            for (auto i = nodes.begin(); i != nodes.end(); ++i)
              i->set_size (1.0f);

          } else if (node_size == node_size_t::NODE_VOLUME) {

            for (auto i = nodes.begin(); i != nodes.end(); ++i)
              i->set_size (voxel_volume * std::cbrt (i->get_volume() / (4.0 * Math::pi)));

          } else if (node_size == node_size_t::CONNECTOME) {

            QModelIndexList list = matrix_list_view->selectionModel()->selectedRows();
            if (list.size() && selected_node_count) {

              const FileDataVector& data (matrix_list_model->get (list[0]));
              const float lower = node_size_lower_button->value(), upper = node_size_upper_button->value();
              const bool invert = node_size_invert_checkbox->isChecked();
              for (node_t i = 1; i <= num_nodes(); ++i) {
                if (selected_nodes[i]) {
                  nodes[i].set_size (1.0f);
                } else {
                  float min = std::numeric_limits<float>::infinity(), sum = 0.0f, max = -std::numeric_limits<float>::infinity();
                  for (node_t j = 1; j <= num_nodes(); ++j) {
                    if (selected_nodes[j]) {
                      const float value = data[mat2vec (i-1, j-1)];
                      min = std::min (min, value);
                      sum += value;
                      max = std::max (max, value);
                    }
                  }
                  const float mean = sum / float(selected_node_count);
                  float factor = 0.0f;
                  switch (node_size_matrix_operator) {
                    case node_property_matrix_operator_t::MIN:  factor = min;  break;
                    case node_property_matrix_operator_t::MEAN: factor = mean; break;
                    case node_property_matrix_operator_t::SUM:  factor = sum;  break;
                    case node_property_matrix_operator_t::MAX:  factor = max;  break;
                  }
                  factor = (factor - lower) / (upper - lower);
                  factor = std::min (1.0f, std::max (factor, 0.0f));
                  factor = invert ? 1.0f-factor : factor;
                  nodes[i].set_size (factor);
                }
              }

            } else {
              for (node_t i = 1; i <= num_nodes(); ++i)
                nodes[i].set_size (1.0f);
            }

          } else if (node_size == node_size_t::VECTOR_FILE) {

            assert (node_values_from_file_size.size() == num_nodes());
            const float lower = node_size_lower_button->value(), upper = node_size_upper_button->value();
            const bool invert = node_size_invert_checkbox->isChecked();
            for (node_t i = 1; i <= num_nodes(); ++i) {
              float factor = (node_values_from_file_size[i-1]-lower) / (upper - lower);
              factor = std::min (1.0f, std::max (factor, 0.0f));
              factor = invert ? 1.0f-factor : factor;
              nodes[i].set_size (factor);
            }

          } else if (node_size == node_size_t::MATRIX_FILE) {

            assert (size_t(node_values_from_file_size.size()) == num_edges());
            if (selected_node_count) {
              const float lower = node_size_lower_button->value(), upper = node_size_upper_button->value();
              const bool invert = node_size_invert_checkbox->isChecked();
              for (node_t i = 1; i <= num_nodes(); ++i) {
                // Unfortunately there's no real sensible way to deal with the case where
                //   node sizes are scaled by a matrix file and you need to choose a size
                //   for a selected node...
                if (selected_nodes[i]) {
                  nodes[i].set_size (1.0f);
                } else {
                  float min = std::numeric_limits<float>::infinity(), sum = 0.0f, max = -std::numeric_limits<float>::infinity();
                  for (node_t j = 1; j <= num_nodes(); ++j) {
                    if (selected_nodes[j]) {
                      const float value = node_values_from_file_size[mat2vec (i-1, j-1)];
                      min = std::min (min, value);
                      sum += value;
                      max = std::max (max, value);
                    }
                  }
                  const float mean = sum / float(selected_node_count);
                  float factor = 0.0f;
                  switch (node_size_matrix_operator) {
                    case node_property_matrix_operator_t::MIN:  factor = min;  break;
                    case node_property_matrix_operator_t::MEAN: factor = mean; break;
                    case node_property_matrix_operator_t::SUM:  factor = sum;  break;
                    case node_property_matrix_operator_t::MAX:  factor = max;  break;
                  }
                  factor = (factor - lower) / (upper - lower);
                  factor = std::min (1.0f, std::max (factor, 0.0f));
                  factor = invert ? 1.0f-factor : factor;
                  nodes[i].set_size (factor);
                }
              }
            } else {
              for (node_t i = 1; i <= num_nodes(); ++i)
                nodes[i].set_size (1.0f);
            }

          }
        }

        void Connectome::calculate_node_alphas()
        {
          if (node_alpha == node_alpha_t::FIXED) {

            for (auto i = nodes.begin(); i != nodes.end(); ++i)
              i->set_alpha (1.0f);

          } else if (node_alpha == node_alpha_t::CONNECTOME) {

            QModelIndexList list = matrix_list_view->selectionModel()->selectedRows();
            if (list.size() && selected_node_count) {

              const FileDataVector& data (matrix_list_model->get (list[0]));
              const float lower = node_alpha_lower_button->value(), upper = node_alpha_upper_button->value();
              const bool invert = node_alpha_invert_checkbox->isChecked();
              for (node_t i = 1; i <= num_nodes(); ++i) {
                if (selected_nodes[i]) {
                  nodes[i].set_alpha (1.0f);
                } else {
                  float min = std::numeric_limits<float>::infinity(), sum = 0.0f, max = -std::numeric_limits<float>::infinity();
                  for (node_t j = 1; j <= num_nodes(); ++j) {
                    if (selected_nodes[j]) {
                      const float value = data[mat2vec (i-1, j-1)];
                      min = std::min (min, value);
                      sum += value;
                      max = std::max (max, value);
                    }
                  }
                  const float mean = sum / float(selected_node_count);
                  float factor = 0.0f;
                  switch (node_alpha_matrix_operator) {
                    case node_property_matrix_operator_t::MIN:  factor = min;  break;
                    case node_property_matrix_operator_t::MEAN: factor = mean; break;
                    case node_property_matrix_operator_t::SUM:  factor = sum;  break;
                    case node_property_matrix_operator_t::MAX:  factor = max;  break;
                  }
                  factor = (factor - lower) / (upper - lower);
                  factor = std::min (1.0f, std::max (factor, 0.0f));
                  factor = invert ? 1.0f-factor : factor;
                  nodes[i].set_alpha (factor);
                }
              }

            } else {
              for (node_t i = 1; i <= num_nodes(); ++i)
                nodes[i].set_alpha (1.0f);
            }

          } else if (node_alpha == node_alpha_t::FROM_LUT) {

            if (lut.size()) {
              for (node_t node_index = 1; node_index <= num_nodes(); ++node_index) {
                const LUT::const_iterator i = lut.find (node_index);
                if (i == lut.end())
                  nodes[node_index].set_alpha (node_fixed_alpha);
                else
                  nodes[node_index].set_alpha (i->second.get_alpha() / 255.0f);
              }
            } else {
              for (auto i = nodes.begin(); i != nodes.end(); ++i)
                i->set_alpha (node_fixed_alpha);
            }

          } else if (node_alpha == node_alpha_t::VECTOR_FILE) {

            assert (node_values_from_file_alpha.size() == num_nodes());
            const float lower = node_alpha_lower_button->value(), upper = node_alpha_upper_button->value();
            const bool invert = node_alpha_invert_checkbox->isChecked();
            for (node_t i = 1; i <= num_nodes(); ++i) {
              float factor = (node_values_from_file_alpha[i-1]-lower) / (upper - lower);
              factor = std::min (1.0f, std::max (factor, 0.0f));
              factor = invert ? 1.0f-factor : factor;
              nodes[i].set_alpha (factor);
            }

          } else if (node_alpha == node_alpha_t::MATRIX_FILE) {

            assert (size_t(node_values_from_file_alpha.size()) == num_edges());
            if (selected_node_count) {
              const float lower = node_alpha_lower_button->value(), upper = node_alpha_upper_button->value();
              const bool invert = node_alpha_invert_checkbox->isChecked();
              for (node_t i = 1; i <= num_nodes(); ++i) {
                if (selected_nodes[i]) {
                  nodes[i].set_alpha (1.0f);
                } else {
                  float min = std::numeric_limits<float>::infinity(), sum = 0.0f, max = -std::numeric_limits<float>::infinity();
                  for (node_t j = 1; j <= num_nodes(); ++j) {
                    if (selected_nodes[j]) {
                      const float value = node_values_from_file_alpha[mat2vec (i-1, j-1)];
                      min = std::min (min, value);
                      sum += value;
                      max = std::max (max, value);
                    }
                  }
                  const float mean = sum / float(selected_node_count);
                  float factor = 0.0f;
                  switch (node_alpha_matrix_operator) {
                    case node_property_matrix_operator_t::MIN:  factor = min;  break;
                    case node_property_matrix_operator_t::MEAN: factor = mean; break;
                    case node_property_matrix_operator_t::SUM:  factor = sum;  break;
                    case node_property_matrix_operator_t::MAX:  factor = max;  break;
                  }
                  factor = (factor - lower) / (upper - lower);
                  factor = std::min (1.0f, std::max (factor, 0.0f));
                  factor = invert ? 1.0f-factor : factor;
                  nodes[i].set_alpha (factor);
                }
              }
            } else {
              for (node_t i = 1; i <= num_nodes(); ++i)
                nodes[i].set_alpha (1.0f);
            }

          }
          update_node_overlay();
        }










        void Connectome::update_node_overlay()
        {
          if (node_geometry == node_geometry_t::OVERLAY) {
            assert (buffer);
            assert (node_overlay);

            auto functor = [&] (decltype(*buffer)& in, decltype(node_overlay->data)& out)
            {
              const node_t node_index = in.value();
              if (node_index) {
                assert (node_index <= num_nodes());
                const Eigen::Array3f& colour (nodes[node_index].get_colour());
                for (out.index(3) = 0; out.index(3) != 3; ++out.index(3))
                  out.value() = colour[int(out.index(3))];
                out.value() = nodes[node_index].get_alpha();
              } else {
                for (out.index(3) = 0; out.index(3) != 4; ++out.index(3))
                  out.value() = 0.0f;
              }
            };

            MR::ThreadedLoop (*buffer).run (functor, *buffer, node_overlay->data);
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
              i->set_visible (!i->is_diagonal() && nodes[i->get_node_index(0)].to_draw() && nodes[i->get_node_index(1)].to_draw());

          } else if (edge_visibility == edge_visibility_t::CONNECTOME) {

            QModelIndexList list = matrix_list_view->selectionModel()->selectedRows();
            if (list.size()) {
              const FileDataVector& data (matrix_list_model->get (list[0]));
              const bool invert = edge_visibility_threshold_invert_checkbox->isChecked();
              const float threshold = edge_visibility_threshold_button->value();
              for (size_t i = 0; i != num_edges(); ++i) {
                if (edges[i].is_diagonal()) {
                  edges[i].set_visible (false);
                } else {
                  const bool above_threshold = (data[i] >= threshold);
                  edges[i].set_visible (above_threshold != invert);
                }
              }
            } else {
              for (auto i = edges.begin(); i != edges.end(); ++i)
                i->set_visible (false);
            }

          } else if (edge_visibility == edge_visibility_t::MATRIX_FILE) {

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
              i->set_colour (Eigen::Array3f { std::abs (i->get_dir()[0]), std::abs (i->get_dir()[1]), std::abs (i->get_dir()[2]) } );

          } else if (edge_colour == edge_colour_t::CONNECTOME) {

            QModelIndexList list = matrix_list_view->selectionModel()->selectedRows();
            if (list.size()) {
              const FileDataVector& data (matrix_list_model->get (list[0]));
              const float lower = edge_colour_lower_button->value(), upper = edge_colour_upper_button->value();
              for (size_t i = 0; i != num_edges(); ++i) {
                float factor = (data[i]-lower) / (upper - lower);
                factor = std::min (1.0f, std::max (factor, 0.0f));
                factor = edge_colourmap_invert ? 1.0f-factor : factor;
                if (ColourMap::maps[edge_colourmap_index].is_colour)
                  edges[i].set_colour (factor * edge_fixed_colour);
                else
                  edges[i].set_colour (ColourMap::maps[edge_colourmap_index].basic_mapping (factor));
              }
            } else {
              for (auto i = edges.begin(); i != edges.end(); ++i)
                i->set_colour (edge_fixed_colour);
            }

          } else if (edge_colour == edge_colour_t::MATRIX_FILE) {

            assert (edge_values_from_file_colour.size());
            const float lower = edge_colour_lower_button->value(), upper = edge_colour_upper_button->value();
            for (size_t i = 0; i != num_edges(); ++i) {
              float factor = (edge_values_from_file_colour[i]-lower) / (upper - lower);
              factor = std::min (1.0f, std::max (factor, 0.0f));
              factor = edge_colourmap_invert ? 1.0f-factor : factor;
              if (ColourMap::maps[edge_colourmap_index].is_colour)
                edges[i].set_colour (factor * edge_fixed_colour);
              else
                edges[i].set_colour (ColourMap::maps[edge_colourmap_index].basic_mapping (factor));
            }

          }
        }

        void Connectome::calculate_edge_sizes()
        {
          if (edge_size == edge_size_t::FIXED) {

            for (auto i = edges.begin(); i != edges.end(); ++i)
              i->set_size (1.0f);

          } else if (edge_size == edge_size_t::CONNECTOME) {

            QModelIndexList list = matrix_list_view->selectionModel()->selectedRows();
            if (list.size()) {
              const FileDataVector& data (matrix_list_model->get (list[0]));
              const float lower = edge_size_lower_button->value(), upper = edge_size_upper_button->value();
              const bool invert = edge_size_invert_checkbox->isChecked();
              for (size_t i = 0; i != num_edges(); ++i) {
                float factor = (data[i]-lower) / (upper - lower);
                factor = std::min (1.0f, std::max (factor, 0.0f));
                factor = invert ? 1.0f-factor : factor;
                edges[i].set_size (factor);
              }
            } else {
              for (auto i = edges.begin(); i != edges.end(); ++i)
                i->set_size (1.0f);
            }

          } else if (edge_size == edge_size_t::MATRIX_FILE) {

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

          } else if (edge_alpha == edge_alpha_t::CONNECTOME) {

            QModelIndexList list = matrix_list_view->selectionModel()->selectedRows();
            if (list.size()) {
              const FileDataVector& data (matrix_list_model->get (list[0]));
              const float lower = edge_alpha_lower_button->value(), upper = edge_alpha_upper_button->value();
              const bool invert = edge_alpha_invert_checkbox->isChecked();
              for (size_t i = 0; i != num_edges(); ++i) {
                float factor = (data[i]-lower) / (upper - lower);
                factor = std::min (1.0f, std::max (factor, 0.0f));
                factor = invert ? 1.0f-factor : factor;
                edges[i].set_alpha (factor);
              }
            } else {
              for (auto i = edges.begin(); i != edges.end(); ++i)
                i->set_alpha (1.0f);
            }

          } else if (edge_alpha == edge_alpha_t::MATRIX_FILE) {

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






        void Connectome::node_selection_changed (const std::vector<node_t>& list)
        {
          selected_nodes.clear();
          selected_node_count = list.size();
          for (std::vector<node_t>::const_iterator n = list.begin(); n != list.end(); ++n)
            selected_nodes[*n] = true;
          if (node_visibility == node_visibility_t::CONNECTOME || node_visibility == node_visibility_t::MATRIX_FILE) {
            if (selected_node_count >= 2) {
              node_visibility_matrix_operator_combobox->removeItem (2);
              switch (node_visibility_matrix_operator) {
                case node_visibility_matrix_operator_t::ANY: node_visibility_matrix_operator_combobox->setCurrentIndex (0); break;
                case node_visibility_matrix_operator_t::ALL: node_visibility_matrix_operator_combobox->setCurrentIndex (1); break;
              }
              node_visibility_matrix_operator_combobox->setEnabled (true);
            } else {
              if (node_visibility_matrix_operator_combobox->count() == 2)
                node_visibility_matrix_operator_combobox->addItem ("N/A");
              node_visibility_matrix_operator_combobox->setCurrentIndex (2);
              node_visibility_matrix_operator_combobox->setEnabled (false);
            }
            calculate_node_visibility();
          }
          if (node_colour == node_colour_t::CONNECTOME || node_colour == node_colour_t::MATRIX_FILE) {
            if (selected_node_count >= 2) {
              node_colour_matrix_operator_combobox->removeItem (4);
              switch (node_colour_matrix_operator) {
                case node_property_matrix_operator_t::MIN:  node_colour_matrix_operator_combobox->setCurrentIndex (0); break;
                case node_property_matrix_operator_t::MEAN: node_colour_matrix_operator_combobox->setCurrentIndex (1); break;
                case node_property_matrix_operator_t::SUM:  node_colour_matrix_operator_combobox->setCurrentIndex (2); break;
                case node_property_matrix_operator_t::MAX:  node_colour_matrix_operator_combobox->setCurrentIndex (3); break;
              }
              node_colour_matrix_operator_combobox->setEnabled (true);
            } else {
              if (node_colour_matrix_operator_combobox->count() == 4)
                node_colour_matrix_operator_combobox->addItem ("N/A");
              node_colour_matrix_operator_combobox->setCurrentIndex (4);
              node_colour_matrix_operator_combobox->setEnabled (false);
            }
            calculate_node_colours();
          }
          if (node_size == node_size_t::CONNECTOME || node_size == node_size_t::MATRIX_FILE) {
            if (selected_node_count >= 2) {
              node_size_matrix_operator_combobox->removeItem (4);
              switch (node_size_matrix_operator) {
                case node_property_matrix_operator_t::MIN:  node_size_matrix_operator_combobox->setCurrentIndex (0); break;
                case node_property_matrix_operator_t::MEAN: node_size_matrix_operator_combobox->setCurrentIndex (1); break;
                case node_property_matrix_operator_t::SUM:  node_size_matrix_operator_combobox->setCurrentIndex (2); break;
                case node_property_matrix_operator_t::MAX:  node_size_matrix_operator_combobox->setCurrentIndex (3); break;
              }
              node_size_matrix_operator_combobox->setEnabled (true);
            } else {
              if (node_size_matrix_operator_combobox->count() == 4)
                node_size_matrix_operator_combobox->addItem ("N/A");
              node_size_matrix_operator_combobox->setCurrentIndex (4);
              node_size_matrix_operator_combobox->setEnabled (false);
            }
            calculate_node_sizes();
          }
          if (node_alpha == node_alpha_t::CONNECTOME || node_alpha == node_alpha_t::MATRIX_FILE) {
            if (selected_node_count >= 2) {
              node_alpha_matrix_operator_combobox->removeItem (4);
              switch (node_alpha_matrix_operator) {
                case node_property_matrix_operator_t::MIN:  node_alpha_matrix_operator_combobox->setCurrentIndex (0); break;
                case node_property_matrix_operator_t::MEAN: node_alpha_matrix_operator_combobox->setCurrentIndex (1); break;
                case node_property_matrix_operator_t::SUM:  node_alpha_matrix_operator_combobox->setCurrentIndex (2); break;
                case node_property_matrix_operator_t::MAX:  node_alpha_matrix_operator_combobox->setCurrentIndex (3); break;
              }
              node_alpha_matrix_operator_combobox->setEnabled (true);
            } else {
              if (node_alpha_matrix_operator_combobox->count() == 4)
                node_alpha_matrix_operator_combobox->addItem ("N/A");
              node_alpha_matrix_operator_combobox->setCurrentIndex (4);
              node_alpha_matrix_operator_combobox->setEnabled (false);
            }
            calculate_node_alphas();
          }
          window().updateGL();
        }






        bool Connectome::node_visibility_given_selection (const node_t index)
        {
          if (!selected_node_count)
            return nodes[index].is_visible();
          if (node_selection_settings.get_node_selected_visibility_override() && selected_nodes[index])
            return true;
          if (!nodes[index].is_visible())
            return false;
          if (node_selection_settings.get_node_other_visibility_override()) {
            // Only override here if there are no connected selected nodes
            for (auto e = edges.begin(); e != edges.end(); ++e) {
              if (e->is_visible() && (e->get_node_index (0) == index || e->get_node_index (1) == index)
                                  && (selected_nodes[e->get_node_index (0)] || selected_nodes[e->get_node_index (1)])) {
                  return true;
              }
            }
            return false;
          }
          return true;
        }
        Eigen::Array3f Connectome::node_colour_given_selection (const node_t index)
        {
          if (selected_nodes[index]) {
            const float fade = node_selection_settings.get_node_selected_colour_fade();
            return ((fade * node_selection_settings.get_node_selected_colour()) + ((1.0f - fade) * nodes[index].get_colour()));
          } else if (selected_node_count) {
            // Need to find out whether or not there is a visible connection to a selected node
            // TODO Needs to be a more efficient way of calculating this...
            for (auto e = edges.begin(); e != edges.end(); ++e) {
              if (e->is_visible() && (e->get_node_index (0) == index || e->get_node_index (1) == index)
                                  && (selected_nodes[e->get_node_index (0)] || selected_nodes[e->get_node_index (1)])) {
                const float fade = node_selection_settings.get_node_associated_colour_fade();
                return ((fade * node_selection_settings.get_node_associated_colour()) + ((1.0f - fade) * nodes[index].get_colour()));
              }
            }
            const float fade = node_selection_settings.get_node_other_colour_fade();
            return ((fade * node_selection_settings.get_node_other_colour()) + ((1.0f - fade) * nodes[index].get_colour()));
          } else {
            return nodes[index].get_colour();
          }
        }
        float Connectome::node_size_given_selection (const node_t index)
        {
          if (selected_nodes[index]) {
            return (node_selection_settings.get_node_selected_size_multiplier() * nodes[index].get_size());
          } else if (selected_node_count) {
            for (auto e = edges.begin(); e != edges.end(); ++e) {
              if (e->is_visible() && (e->get_node_index (0) == index || e->get_node_index (1) == index)
                                  && (selected_nodes[e->get_node_index (0)] || selected_nodes[e->get_node_index (1)])) {
                return (node_selection_settings.get_node_associated_size_multiplier() * nodes[index].get_size());
              }
            }
            return (node_selection_settings.get_node_other_size_multiplier() * nodes[index].get_size());
          } else {
            return nodes[index].get_size();
          }
        }
        float Connectome::node_alpha_given_selection (const node_t index)
        {
          if (selected_nodes[index]) {
            return (node_selection_settings.get_node_selected_alpha_multiplier() * nodes[index].get_alpha());
          } else if (selected_node_count) {
            for (auto e = edges.begin(); e != edges.end(); ++e) {
              if (e->is_visible() && (e->get_node_index (0) == index || e->get_node_index (1) == index)
                                  && (selected_nodes[e->get_node_index (0)] || selected_nodes[e->get_node_index (1)])) {
                return (node_selection_settings.get_node_associated_alpha_multiplier() * nodes[index].get_alpha());
              }
            }
            return (node_selection_settings.get_node_other_alpha_multiplier() * nodes[index].get_alpha());
          } else {
            return nodes[index].get_alpha();
          }
        }
        bool Connectome::edge_visibility_given_selection (const Edge& edge)
        {
          if (!selected_node_count)
            return edge.is_visible();
          if (!edge.is_visible())
            return false;
          if (node_selection_settings.get_edge_other_visibility_override()
              && !(selected_nodes[edge.get_node_index(0)] || selected_nodes[edge.get_node_index(1)]))
            return false;
          return true;
        }
        Eigen::Array3f Connectome::edge_colour_given_selection (const Edge& edge)
        {
          if (!selected_node_count)
            return edge.get_colour();
          float fade = node_selection_settings.get_edge_other_colour_fade();
          Eigen::Array3f colour = node_selection_settings.get_edge_other_colour();
          if (selected_nodes[edge.get_node_index (0)] || selected_nodes[edge.get_node_index (1)]) {
            fade = node_selection_settings.get_edge_associated_colour_fade();
            colour = node_selection_settings.get_edge_associated_colour();
          }
          if (selected_nodes[edge.get_node_index (0)] & selected_nodes[edge.get_node_index (1)]) {
            fade = node_selection_settings.get_edge_selected_colour_fade();
            colour = node_selection_settings.get_edge_selected_colour();
          }
          return ((fade * colour) + ((1.0f - fade) * edge.get_colour()));
        }
        float Connectome::edge_size_given_selection (const Edge& edge)
        {
          if (!selected_node_count)
            return edge.get_size();
          float multiplier = node_selection_settings.get_edge_other_size_multiplier();
          if (selected_nodes[edge.get_node_index (0)] || selected_nodes[edge.get_node_index (1)])
            multiplier = node_selection_settings.get_edge_associated_size_multiplier();
          if (selected_nodes[edge.get_node_index (0)] & selected_nodes[edge.get_node_index (1)])
            multiplier = node_selection_settings.get_edge_selected_size_multiplier();
          return (multiplier * edge.get_size());
        }
        float Connectome::edge_alpha_given_selection (const Edge& edge)
        {
          if (!selected_node_count)
            return edge.get_alpha();
          float multiplier = node_selection_settings.get_edge_other_alpha_multiplier();
          if (selected_nodes[edge.get_node_index (0)] || selected_nodes[edge.get_node_index (1)])
            multiplier = node_selection_settings.get_edge_associated_alpha_multiplier();
          if (selected_nodes[edge.get_node_index (0)] & selected_nodes[edge.get_node_index (1)])
            multiplier = node_selection_settings.get_edge_selected_alpha_multiplier();
          return (multiplier * edge.get_alpha());
        }









        void Connectome::update_controls_node_visibility (const float min, const float mean, const float max)
        {
          node_visibility_threshold_button->setRate (0.001 * (max - min));
          node_visibility_threshold_button->setMin (min);
          node_visibility_threshold_button->setMax (max);
          node_visibility_threshold_button->setValue (mean);
        }
        void Connectome::update_controls_node_colour     (const float min, const float mean, const float max)
        {
          node_colour_lower_button->setValue (min);
          node_colour_upper_button->setValue (max);
          node_colour_lower_button->setMax (max);
          node_colour_upper_button->setMin (min);
          node_colour_lower_button->setRate (0.01f * (mean - min));
          node_colour_upper_button->setRate (0.01f * (max - mean));
        }
        void Connectome::update_controls_node_size       (const float min, const float mean, const float max)
        {
          node_size_lower_button->setValue (min);
          node_size_upper_button->setValue (max);
          node_size_lower_button->setMax (max);
          node_size_upper_button->setMin (min);
          node_size_lower_button->setRate (0.01f * (mean - min));
          node_size_upper_button->setRate (0.01f * (max - mean));
        }
        void Connectome::update_controls_node_alpha      (const float min, const float mean, const float max)
        {
          node_alpha_lower_button->setValue (min);
          node_alpha_upper_button->setValue (max);
          node_alpha_lower_button->setMax (max);
          node_alpha_upper_button->setMin (min);
          node_alpha_lower_button->setRate (0.01f * (mean - min));
          node_alpha_upper_button->setRate (0.01f * (max - mean));
        }
        void Connectome::update_controls_edge_visibility (const float min, const float mean, const float max)
        {
          edge_visibility_threshold_button->setRate (0.001 * (max - min));
          edge_visibility_threshold_button->setMin (min);
          edge_visibility_threshold_button->setMax (max);
          edge_visibility_threshold_button->setValue (mean);
        }
        void Connectome::update_controls_edge_colour     (const float min, const float mean, const float max)
        {
          edge_colour_lower_button->setValue (min);
          edge_colour_upper_button->setValue (max);
          edge_colour_lower_button->setMax (max);
          edge_colour_upper_button->setMin (min);
          edge_colour_lower_button->setRate (0.01f * (mean - min));
          edge_colour_upper_button->setRate (0.01f * (max - mean));
        }
        void Connectome::update_controls_edge_size       (const float min, const float mean, const float max)
        {
          edge_size_lower_button->setValue (min);
          edge_size_upper_button->setValue (max);
          edge_size_lower_button->setMax (max);
          edge_size_upper_button->setMin (min);
          edge_size_lower_button->setRate (0.01f * (mean - min));
          edge_size_upper_button->setRate (0.01f * (max - mean));
        }
        void Connectome::update_controls_edge_alpha      (const float min, const float mean, const float max)
        {
          edge_alpha_lower_button->setValue (min);
          edge_alpha_upper_button->setValue (max);
          edge_alpha_lower_button->setMax (max);
          edge_alpha_upper_button->setMin (min);
          edge_alpha_lower_button->setRate (0.01f * (mean - min));
          edge_alpha_upper_button->setRate (0.01f * (max - mean));
        }











        void Connectome::get_meshes()
        {
          // Request exemplar track file path from user
          const std::string path = GUI::Dialog::File::get_file (this, "Select file containing mesh for each node", "OBJ mesh files (*.obj)");
          if (!path.size()) return;
          Mesh::MeshMulti meshes;
          meshes.load (path);
          if (meshes.size() != nodes.size())
            throw Exception ("Mesh file contains " + str(meshes.size()) + " objects; expected " + str(nodes.size()));
          have_meshes = false;
          MRView::GrabContext context;
          for (node_t i = 1; i <= num_nodes(); ++i)
            nodes[i].assign_mesh (meshes[i]);
          have_meshes = true;
        }



        void Connectome::get_exemplars()
        {
          // Request exemplar track file path from user
          const std::string path = GUI::Dialog::File::get_file (this, "Select track file resulting from running connectome2tck -exemplars", "Track files (*.tck)");
          if (!path.size()) return;
          MR::DWI::Tractography::Properties properties;
          MR::DWI::Tractography::Reader<float> reader (path, properties);
          const size_t num_tracks = to<size_t>(properties["count"]);
          if (num_tracks != num_edges())
            throw Exception ("Track file " + Path::basename (path) + " contains " + str(num_tracks) + " streamlines; connectome expects " + str(num_edges()) + " exemplars");
          ProgressBar progress ("Importing connection exemplars", num_edges());
          MR::DWI::Tractography::Streamline<float> tck;
          while (reader (tck)) {
            edges[tck.index].load_exemplar (tck);
            edges[tck.index].create_streamline();
            ++progress;
          }
          have_exemplars = true;
        }



        void Connectome::get_streamtubes()
        {
          if (!have_exemplars) {
            get_exemplars();
            if (!have_exemplars) return;
          }
          ProgressBar progress ("Generating connection streamtubes", num_edges());
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

        bool Connectome::use_alpha_nodes() const
        {
          bool alpha = !(node_alpha == node_alpha_t::FIXED && node_fixed_alpha == 1.0f);
          if (selected_node_count && (node_selection_settings.get_node_selected_alpha_multiplier() < 1.0f || node_selection_settings.get_node_associated_alpha_multiplier() < 1.0f || node_selection_settings.get_node_other_alpha_multiplier() < 1.0f))
            alpha = true;
          return alpha;
        }

        bool Connectome::use_alpha_edges() const
        {
          bool alpha = !(edge_alpha == edge_alpha_t::FIXED && edge_fixed_alpha == 1.0f);
          if (selected_node_count && (node_selection_settings.get_edge_selected_alpha_multiplier() < 1.0f || node_selection_settings.get_edge_associated_alpha_multiplier() < 1.0f || node_selection_settings.get_edge_other_alpha_multiplier() < 1.0f))
            alpha = true;
          return alpha;
        }






        float Connectome::calc_line_width (const float desired_width, const bool is_smooth) const
        {
          if (is_smooth) {
            if (line_thickness_range_smooth[0] && std::round (desired_width) < line_thickness_range_smooth[0])
              return line_thickness_range_smooth[0];
            if (line_thickness_range_smooth[1] && std::round (desired_width) > line_thickness_range_smooth[1])
              return line_thickness_range_smooth[1];
            return desired_width;
          } else {
            if (line_thickness_range_aliased[0] && std::round (desired_width) < line_thickness_range_aliased[0])
              return line_thickness_range_aliased[0];
            if (line_thickness_range_aliased[1] && std::round (desired_width) > line_thickness_range_aliased[1])
              return line_thickness_range_aliased[1];
            return desired_width;
          }
        }





      }
    }
  }
}





