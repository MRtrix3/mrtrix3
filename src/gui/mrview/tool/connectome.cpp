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

#include "gui/mrview/tool/connectome.h"

#include "file/path.h"
#include "gui/dialog/file.h"
#include "image/adapter/extract.h"
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







        bool Connectome::Shader::need_update (const Connectome&) const { return true; }

        void Connectome::Shader::recompile (const Connectome& parent)
        {
          if (*this != 0)
            clear();
          update (parent);
          GL::Shader::Vertex vertex_shader (vertex_shader_source);
          GL::Shader::Fragment fragment_shader (fragment_shader_source);
          attach (vertex_shader);
          attach (fragment_shader);
          link();
        }

        void Connectome::NodeShader::update (const Connectome& parent)
        {
          vertex_shader_source =
              "layout (location = 0) in vec3 vertexPosition_modelspace;\n";

          if (parent.node_geometry == NODE_GEOM_CUBE) {
            vertex_shader_source +=
              "layout (location = 1) in vec3 vertexNormal_modelspace;\n";
          }

          vertex_shader_source +=
              "uniform mat4 MVP;\n";

          if (parent.node_geometry == NODE_GEOM_SPHERE || parent.node_geometry == NODE_GEOM_CUBE || (parent.node_geometry == NODE_GEOM_MESH && parent.node_size_scale_factor != 1.0f)) {
            vertex_shader_source +=
              "uniform vec3 node_centre;\n"
              "uniform float node_size;\n";
          }

          if (parent.node_geometry == NODE_GEOM_SPHERE) {
            vertex_shader_source +=
              "uniform int reverse;\n";
          }

          if (parent.node_geometry == NODE_GEOM_SPHERE) {
            vertex_shader_source +=
              "out vec3 normal;\n";
          } else if (parent.node_geometry == NODE_GEOM_CUBE) {
            vertex_shader_source +=
              "flat out vec3 normal;\n";
          }

          vertex_shader_source +=
              "void main() {\n";

          switch (parent.node_geometry) {
            case NODE_GEOM_SPHERE:
              vertex_shader_source +=
              "  vec3 pos = vertexPosition_modelspace * node_size;\n"
              "  normal = vertexPosition_modelspace;\n"
              "  if (reverse != 0) {\n"
              "    pos = -pos;\n"
              "    normal = -normal;\n"
              "  }\n"
              "  gl_Position = (MVP * vec4 (node_centre + pos, 1));\n";
              break;
            case NODE_GEOM_CUBE:
              vertex_shader_source +=
              "  gl_Position = (MVP * vec4 (node_centre + (vertexPosition_modelspace * node_size), 1));\n"
              "  normal = vertexNormal_modelspace;\n";
              break;
            case NODE_GEOM_OVERLAY:
              break;
            case NODE_GEOM_MESH:
              if (parent.node_size_scale_factor != 1.0f) {
                vertex_shader_source +=
                    "  gl_Position = MVP * vec4 (node_centre + (node_size * (vertexPosition_modelspace - node_centre)), 1);\n";
              } else {
                vertex_shader_source +=
                    "  gl_Position = MVP * vec4 (vertexPosition_modelspace, 1);\n";
              }

              break;
          }

          vertex_shader_source +=
              "}\n";

          // =================================================================

          const bool per_node_alpha = (parent.node_alpha != NODE_ALPHA_FIXED);

          fragment_shader_source =
              "uniform vec3 node_colour;\n";

          if (per_node_alpha) {
            fragment_shader_source +=
              "uniform float node_alpha;\n"
              "out vec4 color;\n";
          } else {
            fragment_shader_source +=
              "out vec3 color;\n";
          }

          if (parent.node_geometry == NODE_GEOM_SPHERE || parent.node_geometry == NODE_GEOM_CUBE) {
            fragment_shader_source +=
              "uniform float ambient, diffuse, specular, shine;\n"
              "uniform vec3 light_pos;\n";
          }
          if (parent.node_geometry == NODE_GEOM_SPHERE) {
            fragment_shader_source +=
              "in vec3 normal;\n";
          } else if (parent.node_geometry == NODE_GEOM_CUBE) {
            fragment_shader_source +=
              "flat in vec3 normal;\n";
          }

          fragment_shader_source +=
              "void main() {\n";

          if (per_node_alpha) {
            fragment_shader_source +=
              "  color.xyz = node_colour;\n"
              "  color.a = node_alpha;\n";
          } else {
            fragment_shader_source +=
              "  color = node_colour;\n";
          }

          if (parent.node_geometry == NODE_GEOM_SPHERE || parent.node_geometry == NODE_GEOM_CUBE) {
            fragment_shader_source +=
              "  color *= ambient + diffuse * clamp (dot (normal, light_pos), 0, 1);\n"
              "  color += specular * pow (clamp (dot (reflect (-light_pos, normal), normal), 0, 1), shine);\n";
          }

          fragment_shader_source +=
              "}\n";
        }








        void Connectome::EdgeShader::update (const Connectome& parent)
        {
          vertex_shader_source =
              "layout (location = 0) in vec3 vertexPosition_modelspace;\n"
              "uniform mat4 MVP;\n";

          if (parent.edge_geometry == EDGE_GEOM_CYLINDER) {
            vertex_shader_source +=
              "uniform vec3 centre_one, centre_two;\n"
              "uniform mat3 rot_matrix;\n"
              "uniform float radius;\n";
          }

          vertex_shader_source +=
              "void main() {\n";

          switch (parent.edge_geometry) {
            case EDGE_GEOM_LINE:
              vertex_shader_source +=
              "  gl_Position = MVP * vec4 (vertexPosition_modelspace, 1);\n";
              break;
            case EDGE_GEOM_CYLINDER:
              vertex_shader_source +=
              "  vec3 centre = centre_one;\n"
              "  vec3 offset = vertexPosition_modelspace;\n"
              "  if (offset[2] != 0.0) {\n"
              "    centre = centre_two;\n"
              "    offset[2] = 0.0;\n"
              "  }\n"
              "  offset = offset * rot_matrix;\n"
              "  gl_Position = MVP * vec4 (centre + (radius * offset), 1);\n";
              break;
          }

          vertex_shader_source +=
              "}\n";

          // =================================================================

          const bool per_edge_alpha = (parent.edge_alpha != EDGE_ALPHA_FIXED);

          fragment_shader_source =
              "uniform vec3 edge_colour;\n";

          if (per_edge_alpha) {
            fragment_shader_source +=
              "uniform float edge_alpha;\n"
              "out vec4 color;\n";
          } else {
            fragment_shader_source +=
              "out vec3 color;\n";
          }

          fragment_shader_source +=
              "void main() {\n";

          if (per_edge_alpha) {
            fragment_shader_source +=
              "  color.xyz = edge_colour;\n"
              "  color.a = edge_alpha;\n";
          } else {
            fragment_shader_source +=
              "  color = edge_colour;\n";
          }

          fragment_shader_source +=
              "}\n";

        }













        Connectome::Connectome (Window& main_window, Dock* parent) :
            Base (main_window, parent),
            mat2vec (0),
            lighting (window.lighting()),
            node_geometry (NODE_GEOM_SPHERE),
            node_colour (NODE_COLOUR_FIXED),
            node_size (NODE_SIZE_FIXED),
            node_visibility (NODE_VIS_ALL),
            node_alpha (NODE_ALPHA_FIXED),
            node_fixed_colour (0.5f, 0.5f, 0.5f),
            node_fixed_alpha (1.0f),
            node_size_scale_factor (1.0f),
            voxel_volume (0.0f),
            edge_geometry (EDGE_GEOM_LINE),
            edge_colour (EDGE_COLOUR_FIXED),
            edge_size (EDGE_SIZE_FIXED),
            edge_visibility (EDGE_VIS_NONE),
            edge_alpha (EDGE_ALPHA_FIXED),
            edge_fixed_colour (0.5f, 0.5f, 0.5f),
            edge_fixed_alpha (1.0f),
            edge_size_scale_factor (1.0f)
        {
          VBoxLayout* main_box = new VBoxLayout (this);

          HBoxLayout* hlayout = new HBoxLayout;
          hlayout->setContentsMargins (0, 0, 0, 0);
          hlayout->setSpacing (0);

          QGroupBox* group_box = new QGroupBox ("Basic setup");
          main_box->addWidget (group_box);
          VBoxLayout* vlayout = new VBoxLayout;
          group_box->setLayout (vlayout);

          image_button = new QPushButton (this);
          image_button->setToolTip (tr ("Change primary parcellation image"));
          // TODO New icons
          // TODO Have the icons always there, but add the opened file name as text
          //image_button->setIcon (QIcon (":/open.svg"));
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
          lut_combobox->setToolTip (tr ("Open lookup table file (must select appropriate format)"));
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
          config_button->setToolTip (tr ("Open connectome config file"));
          //config_button->setIcon (QIcon (":/close.svg"));
          config_button->setText (tr ("(none)"));
          connect (config_button, SIGNAL (clicked()), this, SLOT (config_open_slot ()));
          hlayout->addWidget (config_button, 1);
          vlayout->addLayout (hlayout);

          group_box = new QGroupBox ("Node visualisation");
          main_box->addWidget (group_box);
          GridLayout* gridlayout = new GridLayout();
          group_box->setLayout (gridlayout);

          QLabel* label = new QLabel ("Geometry: ");
          gridlayout->addWidget (label, 0, 0);
          node_geometry_combobox = new QComboBox (this);
          node_geometry_combobox->setToolTip (tr ("The 3D geometrical shape used to draw each node"));
          node_geometry_combobox->addItem ("Sphere");
          node_geometry_combobox->addItem ("Cube");
          node_geometry_combobox->addItem ("Overlay");
          node_geometry_combobox->addItem ("Mesh");
          connect (node_geometry_combobox, SIGNAL (activated(int)), this, SLOT (node_geometry_selection_slot (int)));
          gridlayout->addWidget (node_geometry_combobox, 0, 1);
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
          gridlayout->addLayout (hlayout, 0, 2, 1, 2);

          label = new QLabel ("Colour: ");
          gridlayout->addWidget (label, 1, 0);
          node_colour_combobox = new QComboBox (this);
          node_colour_combobox->setToolTip (tr ("Set how the colour of each node is determined"));
          node_colour_combobox->addItem ("Fixed");
          node_colour_combobox->addItem ("Random");
          node_colour_combobox->addItem ("Lookup table");
          node_colour_combobox->addItem ("From vector file");
          connect (node_colour_combobox, SIGNAL (activated(int)), this, SLOT (node_colour_selection_slot (int)));
          gridlayout->addWidget (node_colour_combobox, 1, 1);
          hlayout = new HBoxLayout;
          hlayout->setContentsMargins (0, 0, 0, 0);
          hlayout->setSpacing (0);
          node_colour_fixedcolour_button = new QColorButton;
          connect (node_colour_fixedcolour_button, SIGNAL (clicked()), this, SLOT (node_colour_change_slot()));
          hlayout->addWidget (node_colour_fixedcolour_button, 1);
          node_colour_colourmap_button = new ColourMapButton (this, *this, false, false, true);
          node_colour_colourmap_button->setVisible (false);
          hlayout->addWidget (node_colour_colourmap_button, 1);
          gridlayout->addLayout (hlayout, 1, 2, 1, 2);

          label = new QLabel ("Size scaling: ");
          gridlayout->addWidget (label, 2, 0);
          node_size_combobox = new QComboBox (this);
          node_size_combobox->setToolTip (tr ("Scale the size of each node"));
          node_size_combobox->addItem ("Fixed");
          node_size_combobox->addItem ("Node volume");
          node_size_combobox->addItem ("From vector file");
          connect (node_size_combobox, SIGNAL (activated(int)), this, SLOT (node_size_selection_slot (int)));
          gridlayout->addWidget (node_size_combobox, 2, 1);
          hlayout = new HBoxLayout;
          hlayout->setContentsMargins (0, 0, 0, 0);
          hlayout->setSpacing (0);
          node_size_button = new AdjustButton (this, 0.01);
          node_size_button->setValue (node_size_scale_factor);
          node_size_button->setMin (0.0f);
          connect (node_size_button, SIGNAL (valueChanged()), this, SLOT (node_size_value_slot()));
          hlayout->addWidget (node_size_button, 1);
          gridlayout->addLayout (hlayout, 2, 2, 1, 2);

          label = new QLabel ("Visibility: ");
          gridlayout->addWidget (label, 3, 0);
          node_visibility_combobox = new QComboBox (this);
          node_visibility_combobox->setToolTip (tr ("Set which nodes are visible"));
          node_visibility_combobox->addItem ("All");
          node_visibility_combobox->addItem ("None");
          node_visibility_combobox->addItem ("From vector file");
          node_visibility_combobox->addItem ("Degree >= 1");
          node_visibility_combobox->addItem ("Manual");
          connect (node_visibility_combobox, SIGNAL (activated(int)), this, SLOT (node_visibility_selection_slot (int)));
          gridlayout->addWidget (node_visibility_combobox, 3, 1);

          label = new QLabel ("Transparency: ");
          gridlayout->addWidget (label, 4, 0);
          node_alpha_combobox = new QComboBox (this);
          node_alpha_combobox->setToolTip (tr ("Set how node transparency is determined"));
          node_alpha_combobox->addItem ("Fixed");
          node_alpha_combobox->addItem ("Lookup table");
          node_alpha_combobox->addItem ("From vector file");
          connect (node_alpha_combobox, SIGNAL (activated(int)), this, SLOT (node_alpha_selection_slot (int)));
          gridlayout->addWidget (node_alpha_combobox, 4, 1);
          hlayout = new HBoxLayout;
          hlayout->setContentsMargins (0, 0, 0, 0);
          hlayout->setSpacing (0);
          node_alpha_slider = new QSlider (Qt::Horizontal);
          node_alpha_slider->setRange (0,1000);
          node_alpha_slider->setSliderPosition (1000);
          connect (node_alpha_slider, SIGNAL (valueChanged (int)), this, SLOT (node_alpha_value_slot (int)));
          hlayout->addWidget (node_alpha_slider, 1);
          gridlayout->addLayout (hlayout, 4, 2, 1, 2);

          group_box = new QGroupBox ("Edge visualisation");
          main_box->addWidget (group_box);
          gridlayout = new GridLayout();
          group_box->setLayout (gridlayout);

          label = new QLabel ("Geometry: ");
          gridlayout->addWidget (label, 0, 0);
          edge_geometry_combobox = new QComboBox (this);
          edge_geometry_combobox->setToolTip (tr ("The geometry used to draw each edge"));
          edge_geometry_combobox->addItem ("Line");
          edge_geometry_combobox->addItem ("Cylinder");
          connect (edge_geometry_combobox, SIGNAL (activated(int)), this, SLOT (edge_geometry_selection_slot (int)));
          gridlayout->addWidget (edge_geometry_combobox, 0, 1);
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
          gridlayout->addLayout (hlayout, 0, 2, 1, 2);

          label = new QLabel ("Colour: ");
          gridlayout->addWidget (label, 1, 0);
          edge_colour_combobox = new QComboBox (this);
          edge_colour_combobox->setToolTip (tr ("Set how the colour of each edge is determined"));
          edge_colour_combobox->addItem ("Fixed");
          edge_colour_combobox->addItem ("By direction");
          edge_colour_combobox->addItem ("From matrix file");
          connect (edge_colour_combobox, SIGNAL (activated(int)), this, SLOT (edge_colour_selection_slot (int)));
          gridlayout->addWidget (edge_colour_combobox, 1, 1);
          hlayout = new HBoxLayout;
          hlayout->setContentsMargins (0, 0, 0, 0);
          hlayout->setSpacing (0);
          edge_colour_fixedcolour_button = new QColorButton;
          connect (edge_colour_fixedcolour_button, SIGNAL (clicked()), this, SLOT (edge_colour_change_slot()));
          hlayout->addWidget (edge_colour_fixedcolour_button, 1);
          edge_colour_colourmap_button = new ColourMapButton (this, *this, false, false, true);
          edge_colour_colourmap_button->setVisible (false);
          hlayout->addWidget (edge_colour_colourmap_button, 1);
          gridlayout->addLayout (hlayout, 1, 2, 1, 2);

          label = new QLabel ("Size scaling: ");
          gridlayout->addWidget (label, 2, 0);
          edge_size_combobox = new QComboBox (this);
          edge_size_combobox->setToolTip (tr ("Scale the width of each edge"));
          edge_size_combobox->addItem ("Fixed");
          edge_size_combobox->addItem ("From matrix file");
          connect (edge_size_combobox, SIGNAL (activated(int)), this, SLOT (edge_size_selection_slot (int)));
          gridlayout->addWidget (edge_size_combobox, 2, 1);
          hlayout = new HBoxLayout;
          hlayout->setContentsMargins (0, 0, 0, 0);
          hlayout->setSpacing (0);
          edge_size_button = new AdjustButton (this, 0.01);
          edge_size_button->setValue (edge_size_scale_factor);
          edge_size_button->setMin (0.0f);
          connect (edge_size_button, SIGNAL (valueChanged()), this, SLOT (edge_size_value_slot()));
          hlayout->addWidget (edge_size_button, 1);
          gridlayout->addLayout (hlayout, 2, 2, 1, 2);

          label = new QLabel ("Visibility: ");
          gridlayout->addWidget (label, 3, 0);
          edge_visibility_combobox = new QComboBox (this);
          edge_visibility_combobox->setToolTip (tr ("Set which edges are visible"));
          edge_visibility_combobox->addItem ("All");
          edge_visibility_combobox->addItem ("None");
          edge_visibility_combobox->addItem ("By nodes");
          edge_visibility_combobox->addItem ("From matrix file");
          edge_visibility_combobox->setCurrentIndex (1);
          connect (edge_visibility_combobox, SIGNAL (activated(int)), this, SLOT (edge_visibility_selection_slot (int)));
          gridlayout->addWidget (edge_visibility_combobox, 3, 1);

          label = new QLabel ("Transparency: ");
          gridlayout->addWidget (label, 4, 0);
          edge_alpha_combobox = new QComboBox (this);
          edge_alpha_combobox->setToolTip (tr ("Set how node transparency is determined"));
          edge_alpha_combobox->addItem ("Fixed");
          edge_alpha_combobox->addItem ("From matrix file");
          connect (edge_alpha_combobox, SIGNAL (activated(int)), this, SLOT (edge_alpha_selection_slot (int)));
          gridlayout->addWidget (edge_alpha_combobox, 4, 1);
          hlayout = new HBoxLayout;
          hlayout->setContentsMargins (0, 0, 0, 0);
          hlayout->setSpacing (0);
          edge_alpha_slider = new QSlider (Qt::Horizontal);
          edge_alpha_slider->setRange (0,1000);
          edge_alpha_slider->setSliderPosition (1000);
          connect (edge_alpha_slider, SIGNAL (valueChanged (int)), this, SLOT (edge_alpha_value_slot (int)));
          hlayout->addWidget (edge_alpha_slider, 1);
          gridlayout->addLayout (hlayout, 4, 2, 1, 2);

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

          sphere.LOD (4);
          sphere_VAO.gen();
          sphere_VAO.bind();
          sphere.vertex_buffer.bind (gl::ARRAY_BUFFER);
          gl::EnableVertexAttribArray (0);
          gl::VertexAttribPointer (0, 3, gl::FLOAT, gl::FALSE_, 0, (void*)(0));
        }


        Connectome::~Connectome () {}


        void Connectome::draw (const Projection& projection, bool is_3D, int /*axis*/, int /*slice*/)
        {
          if (hide_all_button->isChecked()) return;

          if (node_visibility != NODE_VIS_NONE) {

            if (node_geometry == NODE_GEOM_OVERLAY) {

              if (is_3D) {
                window.get_current_mode()->overlays_for_3D.push_back (node_overlay);
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

              const bool use_alpha = !(node_alpha == NODE_ALPHA_FIXED && node_fixed_alpha == 1.0f);

              gl::Enable (gl::DEPTH_TEST);
              if (use_alpha) {
                gl::Enable (gl::BLEND);
                gl::DepthMask (gl::FALSE_);
                gl::BlendEquation (gl::FUNC_ADD);
                gl::BlendFunc (gl::CONSTANT_ALPHA, gl::ONE_MINUS_CONSTANT_ALPHA);
                gl::BlendColor (1.0, 1.0, 1.0, node_fixed_alpha);
                //gl::Disable (gl::CULL_FACE);
              } else {
                gl::Disable (gl::BLEND);
                gl::DepthMask (gl::TRUE_);
                //gl::Enable (gl::CULL_FACE);
              }

              const GLuint node_colour_ID = gl::GetUniformLocation (node_shader, "node_colour");

              GLuint node_alpha_ID = 0;
              if (node_alpha != NODE_ALPHA_FIXED)
                node_alpha_ID = gl::GetUniformLocation (node_shader, "node_alpha");

              GLuint node_centre_ID = 0, node_size_ID = 0, reverse_ID = 0;
              if (node_geometry == NODE_GEOM_SPHERE || node_geometry == NODE_GEOM_CUBE || (node_geometry == NODE_GEOM_MESH && node_size_scale_factor != 1.0f)) {
                node_centre_ID = gl::GetUniformLocation (node_shader, "node_centre");
                node_size_ID = gl::GetUniformLocation (node_shader, "node_size");
              }

              if (node_geometry == NODE_GEOM_SPHERE) {
                sphere.vertex_buffer.bind (gl::ARRAY_BUFFER);
                sphere_VAO.bind();
                sphere.index_buffer.bind();
                reverse_ID = gl::GetUniformLocation (node_shader, "reverse");
              } else if (node_geometry == NODE_GEOM_CUBE) {
                cube.vertex_buffer.bind (gl::ARRAY_BUFFER);
                cube.normals_buffer.bind (gl::ARRAY_BUFFER);
                cube_VAO.bind();
                cube.index_buffer.bind();
                glShadeModel (GL_FLAT);
                gl::ProvokingVertex (gl::FIRST_VERTEX_CONVENTION);
              }

              if (node_geometry == NODE_GEOM_MESH && node_size_scale_factor != 1.0f) {
                gl::Uniform1f  (node_size_ID, node_size_scale_factor);
              }

              if (node_geometry != NODE_GEOM_OVERLAY) {
                gl::Uniform3fv (gl::GetUniformLocation (node_shader, "light_pos"), 1, lighting.lightpos);
                gl::Uniform1f  (gl::GetUniformLocation (node_shader, "ambient"), lighting.ambient);
                gl::Uniform1f  (gl::GetUniformLocation (node_shader, "diffuse"), lighting.diffuse);
                gl::Uniform1f  (gl::GetUniformLocation (node_shader, "specular"), lighting.specular);
                gl::Uniform1f  (gl::GetUniformLocation (node_shader, "shine"), lighting.shine);
              }

              std::map<float, size_t> node_ordering;
              for (size_t i = 1; i <= num_nodes(); ++i)
                node_ordering.insert (std::make_pair (projection.depth_of (nodes[i].get_com()), i));

              for (auto it = node_ordering.rbegin(); it != node_ordering.rend(); ++it) {
                const Node& node (nodes[it->second]);
                if (node.is_visible()) {
                  gl::Uniform3fv (node_colour_ID, 1, node.get_colour());
                  if (node_alpha != NODE_ALPHA_FIXED)
                    gl::Uniform1f (node_alpha_ID, node.get_alpha());
                  if (node_geometry == NODE_GEOM_SPHERE || node_geometry == NODE_GEOM_CUBE || (node_geometry == NODE_GEOM_MESH && node_size_scale_factor != 1.0f))
                    gl::Uniform3fv (node_centre_ID, 1, &node.get_com()[0]);
                  switch (node_geometry) {
                    case NODE_GEOM_SPHERE:
                      gl::Uniform1f (node_size_ID, node.get_size() * node_size_scale_factor);
                      gl::Uniform1i (reverse_ID, 0);
                      gl::DrawElements (gl::TRIANGLES, sphere.num_indices, gl::UNSIGNED_INT, (void*)0);
                      gl::Uniform1i (reverse_ID, 1);
                      gl::DrawElements (gl::TRIANGLES, sphere.num_indices, gl::UNSIGNED_INT, (void*)0);
                      break;
                    case NODE_GEOM_CUBE:
                      gl::Uniform1f (node_size_ID, node.get_size() * node_size_scale_factor);
                      gl::DrawElements (gl::TRIANGLES, cube.num_indices, gl::UNSIGNED_INT, (void*)0);
                      break;
                    case NODE_GEOM_OVERLAY:
                      break;
                    case NODE_GEOM_MESH:
                      node.render_mesh();
                      break;
                  }
                }
              }

              // Reset to defaults if we've been doing transparency
              if (use_alpha) {
                gl::Disable (gl::BLEND);
                gl::DepthMask (gl::TRUE_);
              }

              if (node_geometry == NODE_GEOM_CUBE)
                glShadeModel (GL_SMOOTH);

              node_shader.stop();

            }

          }

          // =================================================================

          if (edge_visibility != EDGE_VIS_NONE) {

            edge_shader.start (*this);
            projection.set (edge_shader);

            const bool use_alpha = !(edge_alpha == EDGE_ALPHA_FIXED && edge_fixed_alpha == 1.0f);

            gl::Enable (gl::DEPTH_TEST);
            if (use_alpha) {
              gl::Enable (gl::BLEND);
              gl::DepthMask (gl::FALSE_);
              gl::BlendEquation (gl::FUNC_ADD);
              gl::BlendFunc (gl::CONSTANT_ALPHA, gl::ONE_MINUS_CONSTANT_ALPHA);
              gl::BlendColor (1.0, 1.0, 1.0, node_fixed_alpha);
            } else {
              gl::Disable (gl::BLEND);
              gl::DepthMask (gl::TRUE_);
            }

            GLuint node_centre_one_ID = 0, node_centre_two_ID = 0, rot_matrix_ID = 0, radius_ID = 0;
            if (edge_geometry == EDGE_GEOM_CYLINDER) {
              cylinder.vertex_buffer.bind (gl::ARRAY_BUFFER);
              cylinder_VAO.bind();
              cylinder.index_buffer.bind();
              node_centre_one_ID = gl::GetUniformLocation (edge_shader, "centre_one");
              node_centre_two_ID = gl::GetUniformLocation (edge_shader, "centre_two");
              rot_matrix_ID      = gl::GetUniformLocation (edge_shader, "rot_matrix");
              radius_ID          = gl::GetUniformLocation (edge_shader, "radius");
            }

            const GLuint edge_colour_ID = gl::GetUniformLocation (edge_shader, "edge_colour");

            GLuint edge_alpha_ID = 0;
            if (edge_alpha != EDGE_ALPHA_FIXED)
              edge_alpha_ID = gl::GetUniformLocation (edge_shader, "edge_alpha");

            std::map<float, size_t> edge_ordering;
            for (size_t i = 0; i != num_edges(); ++i)
              edge_ordering.insert (std::make_pair (projection.depth_of (edges[i].get_com()), i));

            for (auto it = edge_ordering.rbegin(); it != edge_ordering.rend(); ++it) {
              const Edge& edge (edges[it->second]);
              if (edge.is_visible()) {
                gl::Uniform3fv (edge_colour_ID, 1, edge.get_colour());
                if (edge_alpha != EDGE_ALPHA_FIXED)
                  gl::Uniform1f (edge_alpha_ID, edge.get_alpha());
                switch (edge_geometry) {
                  case EDGE_GEOM_LINE:
                    glLineWidth (edge.get_size() * edge_size_scale_factor);
                    edge.render_line();
                    break;
                  case EDGE_GEOM_CYLINDER:
                    gl::Uniform3fv       (node_centre_one_ID, 1,        edge.get_node_centre (0));
                    gl::Uniform3fv       (node_centre_two_ID, 1,        edge.get_node_centre (1));
                    gl::UniformMatrix3fv (rot_matrix_ID,      1, false, edge.get_rot_matrix());
                    gl::Uniform1f        (radius_ID,                    edge.get_size() * edge_size_scale_factor);
                    gl::DrawElements     (gl::TRIANGLES, cylinder.num_indices, gl::UNSIGNED_INT, (void*)0);
                    break;
                }
              }
            }

            // Reset to defaults if we've been doing transparency
            if (use_alpha) {
              gl::Disable (gl::BLEND);
              gl::DepthMask (gl::TRUE_);
            }

            if (edge_geometry == EDGE_GEOM_LINE)
              glLineWidth (1.0f);

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
          window.updateGL();
        }


        void Connectome::lut_open_slot (int index)
        {
          if (!index) {
            lut.clear();
            lut_mapping.clear();
            //lut_namebox->setText (QString::fromStdString ("(none)"));
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
          } catch (...) { return; }

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
          config_button->setText ("");
          MR::DWI::Tractography::Connectomics::load_config (path, config);
          config_button->setText (QString::fromStdString (Path::basename (path)));
          load_properties();
          window.updateGL();
        }


        void Connectome::hide_all_slot()
        {
          window.updateGL();
        }






        void Connectome::node_geometry_selection_slot (int index)
        {
          switch (index) {
            case 0:
              if (node_geometry == NODE_GEOM_SPHERE) return;
              node_geometry = NODE_GEOM_SPHERE;
              node_size_combobox->setEnabled (true);
              node_size_button->setVisible (true);
              node_size_button->setMax (std::numeric_limits<float>::max());
              node_geometry_sphere_lod_label->setVisible (true);
              node_geometry_sphere_lod_spinbox->setVisible (true);
              break;
            case 1:
              if (node_geometry == NODE_GEOM_CUBE) return;
              node_geometry = NODE_GEOM_CUBE;
              node_size_combobox->setEnabled (true);
              node_size_button->setVisible (true);
              node_size_button->setMax (std::numeric_limits<float>::max());
              node_geometry_sphere_lod_label->setVisible (false);
              node_geometry_sphere_lod_spinbox->setVisible (false);
              break;
            case 2:
              if (node_geometry == NODE_GEOM_OVERLAY) return;
              node_geometry = NODE_GEOM_OVERLAY;
              node_size_combobox->setCurrentIndex (0);
              node_size_combobox->setEnabled (false);
              node_size_button->setVisible (false);
              node_geometry_sphere_lod_label->setVisible (false);
              node_geometry_sphere_lod_spinbox->setVisible (false);
              update_node_overlay();
              break;
            case 3:
              if (node_geometry == NODE_GEOM_MESH) return;
              node_geometry = NODE_GEOM_MESH;
              node_size_combobox->setCurrentIndex (0);
              node_size_combobox->setEnabled (false);
              node_size_button->setVisible (true);
              if (node_size_scale_factor > 1.0f) {
                node_size_scale_factor = 1.0f;
                node_size_button->setValue (node_size_scale_factor);
              }
              node_size_button->setMax (1.0f);
              node_geometry_sphere_lod_label->setVisible (false);
              node_geometry_sphere_lod_spinbox->setVisible (false);
              break;
          }
          window.updateGL();
        }

        void Connectome::node_colour_selection_slot (int index)
        {
          switch (index) {
            case 0:
              if (node_colour == NODE_COLOUR_FIXED) return;
              node_colour = NODE_COLOUR_FIXED;
              node_colour_colourmap_button->setVisible (false);
              node_colour_fixedcolour_button->setVisible (true);
              node_colour_combobox->removeItem (4);
              break;
            case 1:
              // Regenerate random colours on repeat selection
              node_colour = NODE_COLOUR_RANDOM;
              node_colour_colourmap_button->setVisible (false);
              node_colour_fixedcolour_button->setVisible (false);
              node_colour_combobox->removeItem (4);
              break;
            case 2:
              if (node_colour == NODE_COLOUR_LUT) return;
              // TODO Pointless selection if no LUT is loaded... need to detect; or better, disable
              if (lut.size()) {
                node_colour = NODE_COLOUR_LUT;
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
                node_colour = NODE_COLOUR_FIXED;
                node_colour_fixedcolour_button->setVisible (true);
              }
              node_colour_colourmap_button->setVisible (false);
              node_colour_combobox->removeItem (4);
              break;
            case 3:
              try {
                import_file_for_node_property (node_values_from_file_colour, "colours");
              } catch (...) { }
              if (node_values_from_file_colour.size()) {
                node_colour = NODE_COLOUR_FILE;
                // TODO Make other relevant GUI elements visible: lower & upper thresholds, colour map selection & invert option, ...
                node_colour_colourmap_button->setVisible (true);
                node_colour_fixedcolour_button->setVisible (false);
                if (node_colour_combobox->count() == 4)
                  node_colour_combobox->addItem (node_values_from_file_colour.get_name());
                else
                  node_colour_combobox->setItemText (4, node_values_from_file_colour.get_name());
                node_colour_combobox->setCurrentIndex (4);
              } else {
                node_colour_combobox->setCurrentIndex (0);
                node_colour = NODE_COLOUR_FIXED;
                node_colour_colourmap_button->setVisible (false);
                node_colour_fixedcolour_button->setVisible (true);
                node_colour_combobox->removeItem (4);
              }
              break;
            case 4:
              return;
          }
          calculate_node_colours();
          window.updateGL();
        }

        void Connectome::node_size_selection_slot (int index)
        {
          assert (node_geometry == NODE_GEOM_SPHERE || node_geometry == NODE_GEOM_CUBE);
          switch (index) {
            case 0:
              if (node_size == NODE_SIZE_FIXED) return;
              node_size = NODE_SIZE_FIXED;
              node_size_combobox->removeItem (3);
              break;
            case 1:
              if (node_size == NODE_SIZE_VOLUME) return;
              node_size = NODE_SIZE_VOLUME;
              node_size_combobox->removeItem (3);
              break;
            case 2:
              try {
                import_file_for_node_property (node_values_from_file_size, "size");
              } catch (...) { }
              if (node_values_from_file_size.size()) {
                node_size = NODE_SIZE_FILE;
                if (node_size_combobox->count() == 3)
                  node_size_combobox->addItem (node_values_from_file_size.get_name());
                else
                  node_size_combobox->setItemText (3, node_values_from_file_size.get_name());
                node_size_combobox->setCurrentIndex (3);
              } else {
                node_size_combobox->setCurrentIndex (0);
                node_size = NODE_SIZE_FIXED;
                node_size_combobox->removeItem (3);
              }
              break;
            case 3:
              return;
          }
          calculate_node_sizes();
          window.updateGL();
        }

        void Connectome::node_visibility_selection_slot (int index)
        {
          switch (index) {
            case 0:
              if (node_visibility == NODE_VIS_ALL) return;
              node_visibility = NODE_VIS_ALL;
              node_visibility_combobox->removeItem (5);
              break;
            case 1:
              if (node_visibility == NODE_VIS_NONE) return;
              node_visibility = NODE_VIS_NONE;
              node_visibility_combobox->removeItem (5);
              break;
            case 2:
              try {
                import_file_for_node_property (node_values_from_file_visibility, "visibility");
              } catch (...) { }
              if (node_values_from_file_visibility.size()) {
                node_visibility = NODE_VIS_FILE;
                if (node_visibility_combobox->count() == 5)
                  node_visibility_combobox->addItem (node_values_from_file_visibility.get_name());
                else
                  node_visibility_combobox->setItemText (5, node_values_from_file_visibility.get_name());
                node_visibility_combobox->setCurrentIndex (5);
              } else {
                node_visibility_combobox->setCurrentIndex (0);
                node_visibility = NODE_VIS_ALL;
                node_visibility_combobox->removeItem (5);
              }
              break;
            case 3:
              if (node_visibility == NODE_VIS_DEGREE) return;
              if (edge_visibility == EDGE_VIS_NODES) {
                QMessageBox::warning (QApplication::activeWindow(),
                                      tr ("Visualisation error"),
                                      tr ("Cannot have node visibility based on edges; edge visibility is based on nodes!"),
                                      QMessageBox::Ok,
                                      QMessageBox::Ok);
                node_visibility_combobox->setCurrentIndex (0);
                node_visibility = NODE_VIS_ALL;
              } else {
                node_visibility = NODE_VIS_DEGREE;
              }
              node_visibility_combobox->removeItem (5);
              break;
            case 4:
              node_visibility = NODE_VIS_MANUAL;
              node_visibility_combobox->removeItem (5);
              // TODO Here is where the corresponding list view should be made visible
              // Ideally the current node colours would also be presented within this list...
              break;
            case 5:
              return;
          }
          calculate_node_visibility();
          window.updateGL();
        }

        void Connectome::node_alpha_selection_slot (int index)
        {
          switch (index) {
            case 0:
              if (node_alpha == NODE_ALPHA_FIXED) return;
              node_alpha = NODE_ALPHA_FIXED;
              //node_alpha_slider->setVisible (true);
              node_alpha_combobox->removeItem (3);
              break;
            case 1:
              if (node_alpha == NODE_ALPHA_LUT) return;
              node_alpha = NODE_ALPHA_LUT;
              //node_alpha_slider->setVisible (false);
              node_alpha_combobox->removeItem (3);
              break;
            case 2:
              try {
                import_file_for_node_property (node_values_from_file_alpha, "transparency");
              } catch (...) { }
              if (node_values_from_file_alpha.size()) {
                node_alpha = NODE_ALPHA_FILE;
                //node_alpha_slider->setVisible (false);
                if (node_alpha_combobox->count() == 3)
                  node_alpha_combobox->addItem (node_values_from_file_alpha.get_name());
                else
                  node_alpha_combobox->setItemText (3, node_values_from_file_alpha.get_name());
                node_alpha_combobox->setCurrentIndex (3);
              } else {
                node_alpha_combobox->setCurrentIndex (0);
                node_alpha = NODE_ALPHA_FIXED;
                //node_alpha_slider->setVisible (true);
                node_alpha_combobox->removeItem (3);
              }
              break;
            case 3:
              return;
          }
          calculate_node_alphas();
          window.updateGL();
        }





        void Connectome::sphere_lod_slot (int value)
        {
          sphere.LOD (value);
          window.updateGL();
        }

        void Connectome::node_colour_change_slot()
        {
          QColor c = node_colour_fixedcolour_button->color();
          node_fixed_colour.set (c.red() / 255.0f, c.green() / 255.0f, c.blue() / 255.0f);
          calculate_node_colours();
          window.updateGL();
        }

        void Connectome::node_size_value_slot()
        {
          node_size_scale_factor = node_size_button->value();
          window.updateGL();
        }

        void Connectome::node_alpha_value_slot (int position)
        {
          node_fixed_alpha = position / 1000.0f;
          if (node_overlay)
            node_overlay->alpha = node_fixed_alpha;
          window.updateGL();
        }







        void Connectome::edge_geometry_selection_slot (int index)
        {
          switch (index) {
            case 0:
              if (edge_geometry == EDGE_GEOM_LINE) return;
              edge_geometry = EDGE_GEOM_LINE;
              edge_geometry_cylinder_lod_label->setVisible (false);
              edge_geometry_cylinder_lod_spinbox->setVisible (false);
              break;
            case 1:
              if (edge_geometry == EDGE_GEOM_CYLINDER) return;
              edge_geometry = EDGE_GEOM_CYLINDER;
              edge_geometry_cylinder_lod_label->setVisible (true);
              edge_geometry_cylinder_lod_spinbox->setVisible (true);
              break;
          }
          window.updateGL();
        }

        void Connectome::edge_colour_selection_slot (int index)
        {
          switch (index) {
            case 0:
              if (edge_colour == EDGE_COLOUR_FIXED) return;
              edge_colour = EDGE_COLOUR_FIXED;
              edge_colour_colourmap_button->setVisible (false);
              edge_colour_fixedcolour_button->setVisible (true);
              edge_colour_combobox->removeItem (3);
              break;
            case 1:
              if (edge_colour == EDGE_COLOUR_DIR) return;
              edge_colour = EDGE_COLOUR_DIR;
              edge_colour_colourmap_button->setVisible (false);
              edge_colour_fixedcolour_button->setVisible (false);
              edge_colour_combobox->removeItem (3);
              break;
            case 2:
              try {
                import_file_for_edge_property (edge_values_from_file_colour, "colours");
              } catch (...) { }
              if (edge_values_from_file_colour.size()) {
                edge_colour = EDGE_COLOUR_FILE;
                // TODO Make other relevant GUI elements visible: lower & upper thresholds, colour map selection & invert option, ...
                edge_colour_colourmap_button->setVisible (true);
                edge_colour_fixedcolour_button->setVisible (false);
                if (edge_colour_combobox->count() == 3)
                  edge_colour_combobox->addItem (edge_values_from_file_colour.get_name());
                else
                  edge_colour_combobox->setItemText (3, edge_values_from_file_colour.get_name());
                edge_colour_combobox->setCurrentIndex (3);
              } else {
                edge_colour_combobox->setCurrentIndex (0);
                edge_colour = EDGE_COLOUR_FIXED;
                edge_colour_colourmap_button->setVisible (false);
                edge_colour_fixedcolour_button->setVisible (true);
                edge_colour_combobox->removeItem (3);
              }
              break;
            case 3:
              return;
          }
          calculate_edge_colours();
          window.updateGL();
        }

        void Connectome::edge_size_selection_slot (int index)
        {
          switch (index) {
            case 0:
              if (edge_size == EDGE_SIZE_FIXED) return;
              edge_size = EDGE_SIZE_FIXED;
              edge_size_combobox->removeItem (2);
              break;
            case 1:
              try {
                import_file_for_edge_property (edge_values_from_file_size, "size");
              } catch (...) { }
              if (edge_values_from_file_size.size()) {
                edge_size = EDGE_SIZE_FILE;
                if (edge_size_combobox->count() == 2)
                  edge_size_combobox->addItem (edge_values_from_file_size.get_name());
                else
                  edge_size_combobox->setItemText (2, edge_values_from_file_size.get_name());
                edge_size_combobox->setCurrentIndex (2);
              } else {
                edge_size_combobox->setCurrentIndex (0);
                edge_size = EDGE_SIZE_FIXED;
                edge_size_combobox->removeItem (2);
              }
              break;
            case 2:
              return;
          }
          calculate_edge_sizes();
          window.updateGL();
        }

        void Connectome::edge_visibility_selection_slot (int index)
        {
          switch (index) {
            case 0:
              if (edge_visibility == EDGE_VIS_ALL) return;
              edge_visibility = EDGE_VIS_ALL;
              edge_visibility_combobox->removeItem (4);
              break;
            case 1:
              if (edge_visibility == EDGE_VIS_NONE) return;
              edge_visibility = EDGE_VIS_NONE;
              edge_visibility_combobox->removeItem (4);
              break;
            case 2:
              if (edge_visibility == EDGE_VIS_NODES) return;
              if (node_visibility == NODE_VIS_DEGREE) {
                QMessageBox::warning (QApplication::activeWindow(),
                                      tr ("Visualisation error"),
                                      tr ("Cannot have edge visibility based on nodes; node visibility is based on edges!"),
                                      QMessageBox::Ok,
                                      QMessageBox::Ok);
                edge_visibility_combobox->setCurrentIndex (1);
                edge_visibility = EDGE_VIS_NONE;
              } else {
                edge_visibility = EDGE_VIS_NODES;
              }
              edge_visibility_combobox->removeItem (4);
              break;
            case 3:
              try {
                import_file_for_edge_property (edge_values_from_file_visibility, "visibility");
              } catch (...) { }
              if (edge_values_from_file_visibility.size()) {
                edge_visibility = EDGE_VIS_FILE;
                if (edge_visibility_combobox->count() == 4)
                  edge_visibility_combobox->addItem (edge_values_from_file_visibility.get_name());
                else
                  edge_visibility_combobox->setItemText (4, edge_values_from_file_visibility.get_name());
                edge_visibility_combobox->setCurrentIndex (4);
              } else {
                edge_visibility_combobox->setCurrentIndex (1);
                edge_visibility = EDGE_VIS_NONE;
                edge_visibility_combobox->removeItem (4);
              }
              break;
            case 4:
              return;
          }
          calculate_edge_visibility();
          window.updateGL();
        }

        void Connectome::edge_alpha_selection_slot (int index)
        {
          switch (index) {
            case 0:
              if (edge_alpha == EDGE_ALPHA_FIXED) return;
              edge_alpha = EDGE_ALPHA_FIXED;
              edge_alpha_slider->setVisible (true);
              edge_alpha_combobox->removeItem (2);
              break;
            case 1:
              try {
                import_file_for_edge_property (edge_values_from_file_alpha, "transparency");
              } catch (...) { }
              if (edge_values_from_file_alpha.size()) {
                edge_alpha = EDGE_ALPHA_FILE;
                edge_alpha_slider->setVisible (false);
                if (edge_alpha_combobox->count() == 2)
                  edge_alpha_combobox->addItem (edge_values_from_file_alpha.get_name());
                else
                  edge_alpha_combobox->setItemText (2, edge_values_from_file_alpha.get_name());
                edge_alpha_combobox->setCurrentIndex (2);
              } else {
                edge_alpha_combobox->setCurrentIndex (0);
                edge_alpha = EDGE_ALPHA_FIXED;
                edge_alpha_slider->setVisible (true);
                edge_alpha_combobox->removeItem (2);
              }
              break;
            case 2:
              return;
          }
          calculate_edge_alphas();
          window.updateGL();
        }

        void Connectome::cylinder_lod_slot (int index)
        {
          cylinder.LOD (index);
          window.updateGL();
        }
        void Connectome::edge_colour_change_slot()
        {
          QColor c = edge_colour_fixedcolour_button->color();
          edge_fixed_colour.set (c.red() / 255.0f, c.green() / 255.0f, c.blue() / 255.0f);
          calculate_edge_colours();
          window.updateGL();
        }
        void Connectome::edge_size_value_slot()
        {
          edge_size_scale_factor = edge_size_button->value();
          window.updateGL();
        }

        void Connectome::edge_alpha_value_slot (int position)
        {
          edge_fixed_alpha = position / 1000.0f;
          calculate_edge_alphas();
          window.updateGL();
        }








        Connectome::Node::Node (const Point<float>& com, const size_t vol, MR::Image::BufferScratch<bool>& img) :
            centre_of_mass (com),
            volume (vol),
            size (1.0f),
            colour (0.5f, 0.5f, 0.5f),
            alpha (1.0f),
            visible (true)
        {
          MR::Mesh::Mesh temp;
          auto voxel = img.voxel();
          {
            MR::LogLevelLatch latch (0);
            MR::Mesh::vox2mesh (voxel, temp);
            temp.transform_voxel_to_realspace (img);
          }
          mesh = Node::Mesh (temp);
          name = img.name();
        }

        Connectome::Node::Node () :
            centre_of_mass (),
            volume (0),
            size (0.0f),
            colour (0.0f, 0.0f, 0.0f),
            alpha (0.0f),
            visible (false) { }






        Connectome::Node::Mesh::Mesh (const MR::Mesh::Mesh& in) :
            count (3 * in.num_triangles())
        {
          std::vector<float> vertices;
          vertices.reserve (3 * in.num_vertices());
          for (size_t v = 0; v != in.num_vertices(); ++v) {
            for (size_t axis = 0; axis != 3; ++axis)
              vertices.push_back (in.vert(v)[axis]);
          }
          vertex_buffer.gen();
          vertex_buffer.bind (gl::ARRAY_BUFFER);
          if (vertices.size())
            gl::BufferData (gl::ARRAY_BUFFER, vertices.size() * sizeof (float), &vertices[0], gl::STATIC_DRAW);

          vertex_array_object.gen();
          vertex_array_object.bind();
          gl::EnableVertexAttribArray (0);
          gl::VertexAttribPointer (0, 3, gl::FLOAT, gl::FALSE_, 0, (void*)(0));

          std::vector<unsigned int> indices;
          indices.reserve (3 * in.num_triangles());
          for (size_t i = 0; i != in.num_triangles(); ++i) {
            for (size_t v = 0; v != 3; ++v)
              indices.push_back (in.tri(i)[v]);
          }
          index_buffer.gen();
          index_buffer.bind();
          if (indices.size())
            gl::BufferData (gl::ELEMENT_ARRAY_BUFFER, indices.size() * sizeof (unsigned int), &indices[0], gl::STATIC_DRAW);
        }

        Connectome::Node::Mesh::Mesh (Mesh&& that) :
            count (that.count),
            vertex_buffer (std::move (that.vertex_buffer)),
            vertex_array_object (std::move (that.vertex_array_object)),
            index_buffer (std::move (that.index_buffer))
        {
          that.count = 0;
        }

        Connectome::Node::Mesh::Mesh () :
            count (0) { }

        Connectome::Node::Mesh& Connectome::Node::Mesh::operator= (Connectome::Node::Mesh&& that)
        {
          count = that.count; that.count = 0;
          vertex_buffer = std::move (that.vertex_buffer);
          vertex_array_object = std::move (that.vertex_array_object);
          index_buffer = std::move (that.index_buffer);
          return *this;
        }

        void Connectome::Node::Mesh::render() const
        {
          assert (count);
          vertex_buffer.bind (gl::ARRAY_BUFFER);
          vertex_array_object.bind();
          index_buffer.bind();
          gl::DrawElements (gl::TRIANGLES, count, gl::UNSIGNED_INT, (void*)0);
        }







        Connectome::Edge::Edge (const Connectome& parent, const node_t one, const node_t two) :
            node_indices { one, two },
            node_centres { parent.nodes[one].get_com(), parent.nodes[two].get_com() },
            dir ((node_centres[1] - node_centres[0]).normalise()),
            rot_matrix (new GLfloat[9]),
            size (1.0f),
            colour (0.5f, 0.5f, 0.5f),
            alpha (1.0f),
            visible (one != two)
        {
          static const Point<float> z_axis (0.0f, 0.0f, 1.0f);
          if (is_diagonal()) {

            rot_matrix[0] = 0.0f; rot_matrix[1] = 0.0f; rot_matrix[2] = 0.0f;
            rot_matrix[3] = 0.0f; rot_matrix[4] = 0.0f; rot_matrix[5] = 0.0f;
            rot_matrix[6] = 0.0f; rot_matrix[7] = 0.0f; rot_matrix[8] = 0.0f;

          } else {

            // First, let's get an axis of rotation, s.t. the rotation angle is positive
            const Point<float> rot_axis = (z_axis.cross (dir).normalise());
            // Now, a rotation angle
            const float rot_angle = std::acos (z_axis.dot (dir));
            // Convert to versor representation
            const Math::Versor<float> versor (rot_angle, rot_axis);
            // Convert to a matrix
            Math::Matrix<float> matrix;
            versor.to_matrix (matrix);
            // Put into the GLfloat array
            rot_matrix[0] = matrix(0,0); rot_matrix[1] = matrix(0,1); rot_matrix[2] = matrix(0,2);
            rot_matrix[3] = matrix(1,0); rot_matrix[4] = matrix(1,1); rot_matrix[5] = matrix(1,2);
            rot_matrix[6] = matrix(2,0); rot_matrix[7] = matrix(2,1); rot_matrix[8] = matrix(2,2);

          }
        }

        Connectome::Edge::Edge (Edge&& that) :
            node_indices { that.node_indices[0], that.node_indices[1] },
            node_centres { that.node_centres[0], that.node_centres[1] },
            dir (that.dir),
            rot_matrix (that.rot_matrix),
            size (that.size),
            colour (that.colour),
            alpha (that.alpha),
            visible (that.visible)
        {
          that.rot_matrix = nullptr;
        }

        Connectome::Edge::Edge () :
            node_indices { 0, 0 },
            node_centres { Point<float>(), Point<float>() },
            dir (Point<float>()),
            rot_matrix (nullptr),
            size (0.0f),
            colour (0.0f, 0.0f, 0.0f),
            alpha (0.0f),
            visible (false) { }

        Connectome::Edge::~Edge()
        {
          if (rot_matrix) {
            delete[] rot_matrix;
            rot_matrix = nullptr;
          }
        }

        void Connectome::Edge::render_line() const
        {
          glColor3f (colour[0], colour[1], colour[2]);
          glBegin (GL_LINES);
          glVertex3f (node_centres[0][0], node_centres[0][1], node_centres[0][2]);
          glVertex3f (node_centres[1][0], node_centres[1][1], node_centres[1][2]);
          glEnd();
        }








        Connectome::FileDataVector& Connectome::FileDataVector::load (const std::string& filename) {
          Math::Vector<float>::load (filename);
          name = Path::basename (filename).c_str();
          calc_minmax();
          return *this;
        }

        Connectome::FileDataVector& Connectome::FileDataVector::clear ()
        {
          Math::Vector<float>::clear();
          name.clear();
          min = max = NAN;
          return *this;
        }

        void Connectome::FileDataVector::calc_minmax()
        {
          min = std::numeric_limits<float>::max();
          max = -std::numeric_limits<float>::max();
          for (size_t i = 0; i != size(); ++i) {
            min = std::min (min, operator[] (i));
            max = std::max (max, operator[] (i));
          }
        }










        Connectome::NodeOverlay::NodeOverlay (const MR::Image::Info& info) :
            MR::GUI::MRView::ImageBase (info),
            data (info),
            need_update (true)
        {
          position.assign (3, -1);
          set_interpolate (false);
          set_colourmap (5);
          set_min_max (0.0f, 1.0f);
          set_allowed_features  (false, true, true);
          set_use_discard_lower (false);
          set_use_discard_upper (false);
          set_use_transparency  (true);
          set_invert_scale      (false);
          alpha = 1.0f;
          type = gl::FLOAT;
          format = gl::RGBA;
          internal_format = gl::RGBA32F;
        }




        void Connectome::NodeOverlay::update_texture2D (const int plane, const int slice)
        {
          // TESTME Suspect this should never be run...
          assert (0);
          if (!texture2D[plane])
            texture2D[plane].gen (gl::TEXTURE_3D);
          texture2D[plane].bind();
          gl::PixelStorei (gl::UNPACK_ALIGNMENT, 1);
          texture2D[plane].set_interp (interpolation);

          if (position[plane] == slice && !need_update)
            return;
          position[plane] = slice;

          int x, y;
          get_axes (plane, x, y);
          const ssize_t xdim = _info.dim (x), ydim = _info.dim (y);

          std::vector<float> texture_data;
          auto vox = data.voxel();
          texture_data.resize (4*xdim*ydim, 0.0f);
          if (position[plane] >= 0 && position[plane] < _info.dim (plane)) {
            vox[plane] = slice;
            for (vox[y] = 0; vox[y] < ydim; ++vox[y]) {
              for (vox[x] = 0; vox[x] < xdim; ++vox[x]) {
                for (vox[3] = 0; vox[3] != 4; ++vox[3]) {
                  texture_data[4*(vox[x]+vox[y]*xdim) + vox[3]] = vox.value();
            } } }
          }

          gl::TexImage3D (gl::TEXTURE_3D, 0, internal_format, xdim, ydim, 1, 0, format, type, reinterpret_cast<void*> (&texture_data[0]));
          need_update = false;
        }

        void Connectome::NodeOverlay::update_texture3D()
        {
          bind();
          allocate();
          if (!need_update) return;
          value_min = 0.0f; value_max = 1.0f;
          auto V = data.voxel();
          std::vector<float> texture_data (4 * V.dim(0) * V.dim(1));

          ProgressBar progress ("loading parcellation overlay...", V.dim(2));

          for (V[2] = 0; V[2] != V.dim(2); ++V[2]) {
            for (V[1] = 0; V[1] != V.dim(1); ++V[1]) {
              for (V[0] = 0; V[0] != V.dim(0); ++V[0]) {
                for (V[3] = 0; V[3] != 4;        ++V[3]) {
                  texture_data[4*(V[0]+V[1]*V.dim(0)) + V[3]] = V.value();
            } } }
            upload_data ({ { 0, 0, V[2] } }, { { V.dim(0), V.dim(1), 1 } }, reinterpret_cast<void*> (&texture_data[0]));
            ++progress;
          }
          need_update = false;
        }




        std::string Connectome::NodeOverlay::Shader::vertex_shader_source (const Displayable&)
        {
          return
            "layout(location = 0) in vec3 vertpos;\n"
            "layout(location = 1) in vec3 texpos;\n"
            "uniform mat4 MVP;\n"
            "out vec3 texcoord;\n"
            "void main() {\n"
            "  gl_Position =  MVP * vec4 (vertpos,1);\n"
            "  texcoord = texpos;\n"
            "}\n";
        }


        std::string Connectome::NodeOverlay::Shader::fragment_shader_source (const Displayable& object)
        {
          assert (object.colourmap == 5);
          std::string source = object.declare_shader_variables () +
            "uniform sampler3D tex;\n"
            "in vec3 texcoord;\n"
            "out vec4 color;\n"
            "void main() {\n"
            "  if (texcoord.s < 0.0 || texcoord.s > 1.0 ||\n"
            "      texcoord.t < 0.0 || texcoord.t > 1.0 ||\n"
            "      texcoord.p < 0.0 || texcoord.p > 1.0) discard;\n"
            "  color = texture (tex, texcoord.stp);\n"
            "  color.a = color.a * alpha;\n";
          source += "  " + std::string (ColourMap::maps[object.colourmap].mapping);
          source += "}\n";
          return source;
        }










        void Connectome::clear_all()
        {
          image_button ->setText ("");
          lut_combobox->removeItem (5);
          lut_combobox->setCurrentIndex (0);
          config_button->setText ("");
          if (node_colour == NODE_COLOUR_FILE) {
            node_colour_combobox->removeItem (4);
            node_colour_combobox->setCurrentIndex (0);
            node_colour = NODE_COLOUR_FIXED;
          }
          if (node_size == NODE_SIZE_FILE) {
            node_size_combobox->removeItem (3);
            node_size_combobox->setCurrentIndex (0);
            node_size = NODE_SIZE_FIXED;
          }
          if (node_visibility == NODE_VIS_FILE) {
            node_visibility_combobox->removeItem (5);
            node_visibility_combobox->setCurrentIndex (0);
            node_visibility = NODE_VIS_ALL;
          }
          if (node_alpha == NODE_ALPHA_FILE) {
            node_alpha_combobox->removeItem (3);
            node_alpha_combobox->setCurrentIndex (0);
            node_alpha = NODE_ALPHA_FIXED;
          }
          if (edge_colour == EDGE_COLOUR_FILE) {
            edge_colour_combobox->removeItem (3);
            edge_colour_combobox->setCurrentIndex (0);
            edge_colour = EDGE_COLOUR_FIXED;
          }
          if (edge_size == EDGE_SIZE_FILE) {
            edge_size_combobox->removeItem (2);
            edge_size_combobox->setCurrentIndex (0);
            edge_size = EDGE_SIZE_FIXED;
          }
          if (edge_visibility == EDGE_VIS_FILE) {
            edge_visibility_combobox->removeItem (4);
            edge_visibility_combobox->setCurrentIndex (1);
            edge_visibility = EDGE_VIS_NONE;
          }
          if (edge_alpha == EDGE_ALPHA_FILE) {
            edge_alpha_combobox->removeItem (2);
            edge_alpha_combobox->setCurrentIndex (0);
            edge_alpha = EDGE_ALPHA_FIXED;
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
        }

        void Connectome::initialise (const std::string& path)
        {

          MR::Image::Header H (path);
          if (!H.datatype().is_integer())
            throw Exception ("Input parcellation image must have an integer datatype");
          if (H.ndim() != 3)
            throw Exception ("Input parcellation image must be a 3D image");
          voxel_volume = H.vox(0) * H.vox(1) * H.vox(2);
          buffer = new MR::Image::BufferPreload<node_t> (path);
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

                // Determine the sub-volume occupied by this node, and update the image transform appropriately
                MR::Image::Info info (H.info());
                Math::Vector<float> a (4), b (4);
                for (size_t axis = 0; axis != 3; ++axis) {
                  info.dim (axis) = node_upper_corners[node_index][axis] - node_lower_corners[node_index][axis] + 1;
                  a[axis] = node_lower_corners[node_index][axis] * info.vox (axis);
                }
                a[3] = 1.0;
                Math::mult (b, info.transform(), a);
                info.transform().column (3) = b;

                // Construct a scratch buffer into which the volume for this node will be copied
                MR::Image::BufferScratch<bool> scratch_data (info);
                auto scratch = scratch_data.voxel();

                // Use an image adapter to only access the relevant portion of the input image
                std::vector< std::vector<int> > per_axis_indices;
                for (size_t axis = 0; axis != 3; ++axis) {
                  std::vector<int> indices;
                  for (int i = node_lower_corners[node_index][axis]; i <= node_upper_corners[node_index][axis]; ++i)
                    indices.push_back (i);
                  per_axis_indices.push_back (indices);
                }
                MR::Image::Adapter::Extract<decltype(voxel)> extract (voxel, per_axis_indices);

                // Generate the boolean scratch buffer
                MR::Image::Loop loop;
                for (loop.start (extract, scratch); loop.ok(); loop.next (extract, scratch))
                  scratch.value() = (extract.value() == node_index);

                nodes.push_back (Node (node_coms[node_index], node_volumes[node_index], scratch_data));

              } else {
                nodes.push_back (Node());
              }
              ++progress;
            }
          }

          mat2vec = MR::Connectome::Mat2Vec (num_nodes());

          edges.clear();
          edges.reserve (mat2vec.vec_size());
          for (size_t edge_index = 0; edge_index != mat2vec.vec_size(); ++edge_index)
            edges.push_back (Edge (*this, mat2vec(edge_index).first + 1, mat2vec(edge_index).second + 1));

          // Construct the node overlay image
          MR::Image::Info overlay_info (H.info());
          overlay_info.set_ndim (4);
          overlay_info.dim (3) = 4; // RGBA
          overlay_info.stride (3) = 0;
          overlay_info.sanitise();
          node_overlay = new NodeOverlay (overlay_info);
          update_node_overlay();

        }




        void Connectome::import_file_for_node_property (FileDataVector& data, const std::string& attribute)
        {
          data.clear();
          const std::string path = Dialog::File::get_file (this, "Select vector file to determine node " + attribute);
          if (path.empty())
            return;
          data.load (path);
          const size_t numel = data.size();
          if (data.size() != num_nodes()) {
            data.clear();
            throw Exception ("File " + Path::basename (path) + " contains " + str (numel) + " elements, but connectome has " + str(num_nodes()) + " nodes");
          }
          data.set_name (Path::basename (path));
        }


        void Connectome::import_file_for_edge_property (FileDataVector& data, const std::string& attribute)
        {
          data.clear();
          const std::string path = Dialog::File::get_file (this, "Select matrix file to determine edge " + attribute);
          if (path.empty())
            return;
          Math::Matrix<float> temp (path);
          MR::Connectome::verify_matrix (temp, num_nodes());
          mat2vec (temp, data);
          data.set_name (Path::basename (path));
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

          calculate_node_colours();
          calculate_node_sizes();
          calculate_node_visibility();
          calculate_node_alphas();

          calculate_edge_colours();
          calculate_edge_sizes();
          calculate_edge_visibility();
          calculate_edge_alphas();

        }



        void Connectome::calculate_node_colours()
        {
          if (node_colour == NODE_COLOUR_FIXED) {

            for (auto i = nodes.begin(); i != nodes.end(); ++i)
              i->set_colour (node_fixed_colour);

          } else if (node_colour == NODE_COLOUR_RANDOM) {

            Point<float> rgb;
            Math::RNG rng;
            for (auto i = nodes.begin(); i != nodes.end(); ++i) {
              do {
                rgb.set (rng.uniform(), rng.uniform(), rng.uniform());
              } while (rgb[0] < 0.5 && rgb[1] < 0.5 && rgb[2] < 0.5);
              i->set_colour (rgb);
            }

          } else if (node_colour == NODE_COLOUR_LUT) {

            assert (lut.size());
            for (size_t node_index = 1; node_index <= num_nodes(); ++node_index) {
              if (lut_mapping[node_index] == lut.end())
                nodes[node_index].set_colour (node_fixed_colour);
              else
                nodes[node_index].set_colour (Point<float> (lut_mapping[node_index]->second.get_colour()) / 255.0f);
            }

          } else if (node_colour == NODE_COLOUR_FILE) {

            // TODO Probably actually nothing to do here;
            //   shader will branch in order to feed the raw value from the imported file into a colour
            //   (will need to send the shader a scalar rather than a vec3)
            // This will then enable use of all possible colour maps
            for (auto i = nodes.begin(); i != nodes.end(); ++i)
              i->set_colour (Point<float> (0.0f, 0.0f, 0.0f));

          }
          update_node_overlay();
        }



        void Connectome::calculate_node_sizes()
        {
          if (node_size == NODE_SIZE_FIXED) {

            for (auto i = nodes.begin(); i != nodes.end(); ++i)
              i->set_size (1.0f);

          } else if (node_size == NODE_SIZE_VOLUME) {

            for (auto i = nodes.begin(); i != nodes.end(); ++i)
              i->set_size (voxel_volume * std::cbrt (i->get_volume() / (4.0 * Math::pi)));

          } else if (node_size == NODE_SIZE_FILE) {

            assert (node_values_from_file_size.size());
            for (size_t i = 1; i <= num_nodes(); ++i)
              nodes[i].set_size (std::cbrt (node_values_from_file_size[i-1] / (4.0 * Math::pi)));

          }
        }



        void Connectome::calculate_node_visibility()
        {
          if (node_visibility == NODE_VIS_ALL) {

            for (auto i = nodes.begin(); i != nodes.end(); ++i)
              i->set_visible (true);

          } else if (node_visibility == NODE_VIS_NONE) {

            for (auto i = nodes.begin(); i != nodes.end(); ++i)
              i->set_visible (false);

          } else if (node_visibility == NODE_VIS_FILE) {

            assert (node_values_from_file_visibility.size());
            for (size_t i = 1; i <= num_nodes(); ++i)
              nodes[i].set_visible (node_values_from_file_visibility[i-1]);

          } else if (node_visibility == NODE_VIS_DEGREE) {

            for (auto i = nodes.begin(); i != nodes.end(); ++i)
              i->set_visible (false);
            for (auto i = edges.begin(); i != edges.end(); ++i) {
              if (i->is_visible()) {
                nodes[i->get_node_index(0)].set_visible (true);
                nodes[i->get_node_index(1)].set_visible (true);
              }
            }

          } else if (node_visibility == NODE_VIS_MANUAL) {

            // TODO This needs to read from the corresponding list view (which doesn't exist yet),
            //   and set the visibilities accordingly

          }
          update_node_overlay();
          if (edge_visibility == EDGE_VIS_NODES)
            calculate_edge_visibility();
        }



        void Connectome::calculate_node_alphas()
        {
          if (node_alpha == NODE_ALPHA_FIXED) {

            for (auto i = nodes.begin(); i != nodes.end(); ++i)
              i->set_alpha (1.0f);

          } else if (node_alpha == NODE_ALPHA_LUT) {

            assert (lut.size());
            for (size_t node_index = 1; node_index <= num_nodes(); ++node_index) {
              if (lut_mapping[node_index] == lut.end())
                nodes[node_index].set_alpha (node_fixed_alpha);
              else
                nodes[node_index].set_alpha (lut_mapping[node_index]->second.get_alpha() / 255.0f);
            }

          } else if (node_alpha == NODE_ALPHA_FILE) {

            assert (node_values_from_file_alpha.size());
            for (size_t i = 1; i <= num_nodes(); ++i)
              nodes[i].set_alpha (node_values_from_file_alpha[i-1]);

          }
          update_node_overlay();
        }










        void Connectome::update_node_overlay()
        {
          assert (buffer);
          assert (node_overlay);
          auto in = buffer->voxel();
          auto out = node_overlay->voxel();
          if (node_geometry == NODE_GEOM_OVERLAY) {
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














        void Connectome::calculate_edge_colours()
        {
          if (edge_colour == EDGE_COLOUR_FIXED) {

            for (auto i = edges.begin(); i != edges.end(); ++i)
              i->set_colour (edge_fixed_colour);

          } else if (edge_colour == EDGE_COLOUR_DIR) {

            for (auto i = edges.begin(); i != edges.end(); ++i)
              i->set_colour (Point<float> (std::abs (i->get_dir()[0]), std::abs (i->get_dir()[1]), std::abs (i->get_dir()[2])));

          } else if (edge_colour == EDGE_COLOUR_FILE) {

            for (auto i = edges.begin(); i != edges.end(); ++i)
              i->set_colour (Point<float> (0.0f, 0.0f, 0.0f));

          }
        }



        void Connectome::calculate_edge_sizes()
        {
          if (edge_size == EDGE_SIZE_FIXED) {

            for (auto i = edges.begin(); i != edges.end(); ++i)
              i->set_size (1.0f);

          } else if (edge_size == EDGE_SIZE_FILE) {

            assert (edge_values_from_file_size.size());
            for (size_t i = 0; i != edges.size(); ++i)
              edges[i].set_size (std::sqrt (edge_values_from_file_size[i] / Math::pi));

          }
        }



        void Connectome::calculate_edge_visibility()
        {
          if (edge_visibility == EDGE_VIS_ALL) {

            for (auto i = edges.begin(); i != edges.end(); ++i)
              i->set_visible (!i->is_diagonal());

          } else if (edge_visibility == EDGE_VIS_NONE) {

            for (auto i = edges.begin(); i != edges.end(); ++i)
              i->set_visible (false);

          } else if (edge_visibility == EDGE_VIS_NODES) {

            for (auto i = edges.begin(); i != edges.end(); ++i)
              i->set_visible (nodes[i->get_node_index(0)].is_visible() && nodes[i->get_node_index(1)].is_visible());

          } else if (edge_visibility == EDGE_VIS_FILE) {

            assert (edge_values_from_file_visibility.size());
            for (size_t i = 0; i != edges.size(); ++i)
              edges[i].set_visible (edge_values_from_file_visibility[i] && !edges[i].is_diagonal());

          }
          if (node_visibility == NODE_VIS_DEGREE)
            calculate_node_visibility();
        }



        void Connectome::calculate_edge_alphas()
        {
          if (edge_alpha == EDGE_ALPHA_FIXED) {

            for (auto i = edges.begin(); i != edges.end(); ++i)
              i->set_alpha (1.0f);

          } else if (edge_alpha == EDGE_ALPHA_FILE) {

            assert (edge_values_from_file_alpha.size());
            for (size_t i = 0; i != edges.size(); ++i)
              edges[i].set_alpha (edge_values_from_file_alpha[i]);

          }
        }




      }
    }
  }
}





