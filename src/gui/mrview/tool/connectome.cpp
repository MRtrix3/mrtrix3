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
#include "image/buffer.h"
#include "image/buffer_scratch.h"
#include "image/header.h"
#include "image/loop.h"
#include "image/nav.h"
#include "image/transform.h"

#include "math/math.h"
#include "math/rng.h"

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
              "layout (location = 0) in vec3 vertexPosition_modelspace;\n"
              "uniform mat4 MVP;\n";

          if (parent.node_geometry == NODE_GEOM_SPHERE) {
            vertex_shader_source +=
              "uniform vec3 node_centre;\n"
              "uniform float node_size;\n"
              "uniform int reverse;\n";
          }

          if (parent.node_geometry == NODE_GEOM_SPHERE) {
            vertex_shader_source +=
              "out vec3 normal;\n";
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
            case NODE_GEOM_OVERLAY:
              break;
            case NODE_GEOM_MESH:
              vertex_shader_source +=
              "  gl_Position = MVP * vec4 (vertexPosition_modelspace, 1);\n";
              break;
          }

          vertex_shader_source += "}\n";

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

          if (parent.node_geometry == NODE_GEOM_SPHERE) {
            fragment_shader_source +=
              "uniform float ambient, diffuse, specular, shine;\n"
              "uniform vec3 light_pos;\n"
              "in vec3 normal;\n";
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

          if (parent.node_geometry == NODE_GEOM_SPHERE) {
            fragment_shader_source +=
              "  color *= ambient + diffuse * clamp (dot (normal, light_pos), 0, 1);\n"
              "  color += specular * pow (clamp (dot (reflect (-light_pos, normal), normal), 0, 1), shine);\n";
          }

          fragment_shader_source += "}\n";
        }








        void Connectome::EdgeShader::update (const Connectome& parent)
        {
          vertex_shader_source =
              "layout (location = 0) in vec3 vertexPosition_modelspace;\n"
              "uniform mat4 MVP;\n";

          vertex_shader_source +=
              "void main() {\n";

          vertex_shader_source +=
              "  gl_Position = MVP * vec4 (vertexPosition_modelspace, 1);\n";

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

          fragment_shader_source += "}\n";

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
          node_colour_fixedcolour_button = new QColorButton;
          connect (node_colour_fixedcolour_button, SIGNAL (clicked()), this, SLOT (node_colour_change_slot()));
          gridlayout->addWidget (node_colour_fixedcolour_button, 1, 2);
          node_colour_colourmap_button = new ColourMapButton (this, *this, false, false, true);
          node_colour_colourmap_button->setVisible (false);
          gridlayout->addWidget (node_colour_colourmap_button, 1, 3, 1, 1);

          label = new QLabel ("Size scaling: ");
          gridlayout->addWidget (label, 2, 0);
          node_size_combobox = new QComboBox (this);
          node_size_combobox->setToolTip (tr ("Scale the size of each node"));
          node_size_combobox->addItem ("Fixed");
          node_size_combobox->addItem ("Node volume");
          node_size_combobox->addItem ("From vector file");
          connect (node_size_combobox, SIGNAL (activated(int)), this, SLOT (node_size_selection_slot (int)));
          gridlayout->addWidget (node_size_combobox, 2, 1);
          node_size_button = new AdjustButton (this, 0.01);
          node_size_button->setValue (node_size_scale_factor);
          node_size_button->setMin (0.0f);
          connect (node_size_button, SIGNAL (valueChanged()), this, SLOT (node_size_value_slot()));
          gridlayout->addWidget (node_size_button, 2, 2, 1, 1);

          label = new QLabel ("Visibility: ");
          gridlayout->addWidget (label, 3, 0);
          node_visibility_combobox = new QComboBox (this);
          node_visibility_combobox->setToolTip (tr ("Set which nodes are visible"));
          node_visibility_combobox->addItem ("All");
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
          node_alpha_slider = new QSlider (Qt::Horizontal);
          node_alpha_slider->setRange (0,1000);
          node_alpha_slider->setSliderPosition (1000);
          connect (node_alpha_slider, SIGNAL (valueChanged (int)), this, SLOT (node_alpha_value_slot (int)));
          gridlayout->addWidget (node_alpha_slider, 4, 2, 1, 2);

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
          edge_colour_fixedcolour_button = new QColorButton;
          connect (edge_colour_fixedcolour_button, SIGNAL (clicked()), this, SLOT (edge_colour_change_slot()));
          gridlayout->addWidget (edge_colour_fixedcolour_button, 1, 2);
          edge_colour_colourmap_button = new ColourMapButton (this, *this, false, false, true);
          edge_colour_colourmap_button->setVisible (false);
          gridlayout->addWidget (edge_colour_colourmap_button, 1, 3, 1, 1);

          label = new QLabel ("Size scaling: ");
          gridlayout->addWidget (label, 2, 0);
          edge_size_combobox = new QComboBox (this);
          edge_size_combobox->setToolTip (tr ("Scale the width of each edge"));
          edge_size_combobox->addItem ("Fixed");
          edge_size_combobox->addItem ("From matrix file");
          connect (edge_size_combobox, SIGNAL (activated(int)), this, SLOT (edge_size_selection_slot (int)));
          gridlayout->addWidget (edge_size_combobox, 2, 1);
          edge_size_button = new AdjustButton (this, 0.01);
          edge_size_button->setValue (edge_size_scale_factor);
          edge_size_button->setMin (0.0f);
          connect (edge_size_button, SIGNAL (valueChanged()), this, SLOT (edge_size_value_slot()));
          gridlayout->addWidget (edge_size_button, 2, 2, 1, 1);

          label = new QLabel ("Visibility: ");
          gridlayout->addWidget (label, 3, 0);
          edge_visibility_combobox = new QComboBox (this);
          edge_visibility_combobox->setToolTip (tr ("Set which edges are visible"));
          edge_visibility_combobox->addItem ("All");
          edge_visibility_combobox->addItem ("None");
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
          edge_alpha_slider = new QSlider (Qt::Horizontal);
          edge_alpha_slider->setRange (0,1000);
          edge_alpha_slider->setSliderPosition (1000);
          connect (edge_alpha_slider, SIGNAL (valueChanged (int)), this, SLOT (edge_alpha_value_slot (int)));
          gridlayout->addWidget (edge_alpha_slider, 4, 2, 1, 2);

          main_box->addStretch ();
          setMinimumSize (main_box->minimumSize());

          sphere.LOD (4);
          sphere_VAO.gen();
          sphere_VAO.bind();
          sphere.vertex_buffer.bind (gl::ARRAY_BUFFER);
          gl::EnableVertexAttribArray (0);
          gl::VertexAttribPointer (0, 3, gl::FLOAT, gl::FALSE_, 0, (void*)(0));

          image_open_slot();
        }


        Connectome::~Connectome () {}


        void Connectome::draw (const Projection& projection, bool /*is_3D*/, int /*axis*/, int /*slice*/)
        {
          if (hide_all_button->isChecked()) return;

          node_shader.start (*this);
          projection.set (node_shader);

          bool use_alpha = !(node_alpha == NODE_ALPHA_FIXED && node_fixed_alpha == 1.0f);

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
          if (node_geometry == NODE_GEOM_SPHERE) {
            sphere.vertex_buffer.bind (gl::ARRAY_BUFFER);
            sphere_VAO.bind();
            sphere.index_buffer.bind();
            node_centre_ID = gl::GetUniformLocation (node_shader, "node_centre");
            node_size_ID = gl::GetUniformLocation (node_shader, "node_size");
            reverse_ID = gl::GetUniformLocation (node_shader, "reverse");
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
              switch (node_geometry) {
                case NODE_GEOM_SPHERE:
                  gl::Uniform3fv (node_centre_ID, 1, &node.get_com()[0]);
                  gl::Uniform1f (node_size_ID, node.get_size() * node_size_scale_factor);
                  gl::Uniform1i (reverse_ID, 0);
                  gl::DrawElements (gl::TRIANGLES, sphere.num_indices, gl::UNSIGNED_INT, (void*)0);
                  gl::Uniform1i (reverse_ID, 1);
                  gl::DrawElements (gl::TRIANGLES, sphere.num_indices, gl::UNSIGNED_INT, (void*)0);
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

          node_shader.stop();

          // =================================================================
          if (edge_visibility != EDGE_VIS_NONE) {

            edge_shader.start (*this);
            projection.set (edge_shader);

            use_alpha = !(edge_alpha == EDGE_ALPHA_FIXED && edge_fixed_alpha == 1.0f);

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

            const GLuint edge_colour_ID = gl::GetUniformLocation (edge_shader, "edge_colour");

            GLuint edge_alpha_ID = 0;
            if (edge_alpha != EDGE_ALPHA_FIXED)
              edge_alpha_ID = gl::GetUniformLocation (edge_shader, "edge_alpha");

            std::map<float, size_t> edge_ordering;
            for (size_t i = 0; i <= num_edges(); ++i)
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
                    // TODO
                    break;
                }
              }
            }

            // Reset to defaults if we've been doing transparency
            if (use_alpha) {
              gl::Disable (gl::BLEND);
              gl::DepthMask (gl::TRUE_);
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
              node_geometry_sphere_lod_label->setVisible (true);
              node_geometry_sphere_lod_spinbox->setVisible (true);
              break;
            case 1:
              if (node_geometry == NODE_GEOM_OVERLAY) return;
              node_geometry = NODE_GEOM_OVERLAY;
              node_size_combobox->setCurrentIndex (0);
              node_size_combobox->setEnabled (false);
              node_size_button->setVisible (false);
              node_geometry_sphere_lod_label->setVisible (false);
              node_geometry_sphere_lod_spinbox->setVisible (false);
              break;
            case 2:
              if (node_geometry == NODE_GEOM_MESH) return;
              node_geometry = NODE_GEOM_MESH;
              node_size_combobox->setCurrentIndex (0);
              node_size_combobox->setEnabled (false);
              node_size_button->setVisible (false);
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
              break;
            case 1:
              //if (node_colour == NODE_COLOUR_RANDOM) return; // Keep this; regenerate random colours on repeat selection
              node_colour = NODE_COLOUR_RANDOM;
              node_colour_colourmap_button->setVisible (false);
              node_colour_fixedcolour_button->setVisible (false);
              break;
            case 2:
              if (node_colour == NODE_COLOUR_LUT) return;
              // TODO Pointless selection if no LUT is loaded... need to detect; or better, disable
              if (lut.size()) {
                node_colour = NODE_COLOUR_LUT;
                node_colour_colourmap_button->setVisible (false);
                node_colour_fixedcolour_button->setVisible (false);
              } else {
                node_colour_combobox->setCurrentIndex (0);
                node_colour = NODE_COLOUR_FIXED;
                node_colour_colourmap_button->setVisible (false);
                node_colour_fixedcolour_button->setVisible (true);
              }
              break;
            case 3:
              //if (node_colour == NODE_COLOUR_FILE) return; // Keep this; may want to select a new file
              try {
                import_file_for_node_property (node_values_from_file_colour, "colours");
              } catch (...) { }
              if (node_values_from_file_colour.size()) {
                node_colour = NODE_COLOUR_FILE;
                // TODO Make other relevant GUI elements visible: lower & upper thresholds, colour map selection & invert option, ...
                node_colour_colourmap_button->setVisible (true);
                node_colour_fixedcolour_button->setVisible (false);
              } else {
                node_colour_combobox->setCurrentIndex (0);
                node_colour = NODE_COLOUR_FIXED;
                node_colour_colourmap_button->setVisible (false);
                node_colour_fixedcolour_button->setVisible (true);
              }
              break;
          }
          calculate_node_colours();
          window.updateGL();
        }

        void Connectome::node_size_selection_slot (int index)
        {
          assert (node_geometry == NODE_GEOM_SPHERE);
          switch (index) {
            case 0:
              node_size = NODE_SIZE_FIXED;
              break;
            case 1:
              node_size = NODE_SIZE_VOLUME;
              break;
            case 2:
              try {
                import_file_for_node_property (node_values_from_file_size, "size");
              } catch (...) { }
              if (node_values_from_file_size.size()) {
                node_size = NODE_SIZE_FILE;
              } else {
                node_size_combobox->setCurrentIndex (0);
                node_size = NODE_SIZE_FIXED;
              }
              break;
          }
          calculate_node_sizes();
          window.updateGL();
        }

        void Connectome::node_visibility_selection_slot (int index)
        {
          switch (index) {
            case 0:
              node_visibility = NODE_VIS_ALL;
              break;
            case 1:
              try {
                import_file_for_node_property (node_values_from_file_visibility, "visibility");
              } catch (...) { }
              if (node_values_from_file_visibility.size()) {
                node_visibility = NODE_VIS_FILE;
              } else {
                node_visibility_combobox->setCurrentIndex (0);
                node_visibility = NODE_VIS_ALL;
              }
              break;
            case 2:
              node_visibility = NODE_VIS_DEGREE;
              break;
            case 3:
              node_visibility = NODE_VIS_MANUAL;
              // TODO Here is where the corresponding list view should be made visible
              // Ideally the current node colours would also be presented within this list...
              break;
          }
          calculate_node_visibility();
          window.updateGL();
        }

        void Connectome::node_alpha_selection_slot (int index)
        {
          switch (index) {
            case 0:
              node_alpha = NODE_ALPHA_FIXED;
              node_alpha_slider->setVisible (true);
              break;
            case 1:
              node_alpha = NODE_ALPHA_LUT;
              node_alpha_slider->setVisible (false);
              break;
            case 2:
              try {
                import_file_for_node_property (node_values_from_file_alpha, "transparency");
              } catch (...) { }
              if (node_values_from_file_alpha.size()) {
                node_alpha = NODE_ALPHA_FILE;
                node_alpha_slider->setVisible (false);
              } else {
                node_alpha_combobox->setCurrentIndex (0);
                node_alpha = NODE_ALPHA_FIXED;
                node_alpha_slider->setVisible (true);
              }
              break;
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
          calculate_node_alphas();
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
              break;
            case 1:
              if (edge_colour == EDGE_COLOUR_DIR) return;
              edge_colour = EDGE_COLOUR_DIR;
              edge_colour_colourmap_button->setVisible (false);
              edge_colour_fixedcolour_button->setVisible (false);
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
              } else {
                edge_colour_combobox->setCurrentIndex (0);
                edge_colour = EDGE_COLOUR_FIXED;
                edge_colour_colourmap_button->setVisible (false);
                edge_colour_fixedcolour_button->setVisible (true);
              }
              break;
          }
          calculate_edge_colours();
          window.updateGL();
        }

        void Connectome::edge_size_selection_slot (int index)
        {
          switch (index) {
            case 0:
              edge_size = EDGE_SIZE_FIXED;
              break;
            case 1:
              try {
                import_file_for_edge_property (edge_values_from_file_size, "size");
              } catch (...) { }
              if (edge_values_from_file_size.size()) {
                edge_size = EDGE_SIZE_FILE;
              } else {
                edge_size_combobox->setCurrentIndex (0);
                edge_size = EDGE_SIZE_FIXED;
              }
              break;
          }
          calculate_edge_sizes();
          window.updateGL();
        }

        void Connectome::edge_visibility_selection_slot (int index)
        {
          switch (index) {
            case 0:
              edge_visibility = EDGE_VIS_ALL;
              break;
            case 1:
              edge_visibility = EDGE_VIS_NONE;
              break;
            case 2:
              try {
                import_file_for_edge_property (edge_values_from_file_visibility, "visibility");
              } catch (...) { }
              if (edge_values_from_file_visibility.size()) {
                edge_visibility = EDGE_VIS_FILE;
              } else {
                edge_visibility_combobox->setCurrentIndex (0);
                edge_visibility = EDGE_VIS_ALL;
              }
              break;
          }
          calculate_edge_visibility();
          window.updateGL();
        }

        void Connectome::edge_alpha_selection_slot (int index)
        {
          switch (index) {
            case 0:
              edge_alpha = EDGE_ALPHA_FIXED;
              edge_alpha_slider->setVisible (true);
              break;
            case 1:
              try {
                import_file_for_edge_property (edge_values_from_file_alpha, "transparency");
              } catch (...) { }
              if (edge_values_from_file_alpha.size()) {
                edge_alpha = EDGE_ALPHA_FILE;
                edge_alpha_slider->setVisible (false);
              } else {
                edge_alpha_combobox->setCurrentIndex (0);
                edge_alpha = EDGE_ALPHA_FIXED;
                edge_alpha_slider->setVisible (true);
              }
              break;
          }
          calculate_edge_alphas();
          window.updateGL();
        }

        void Connectome::cylinder_lod_slot (int /*index*/)
        {
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
            size (1.0f),
            colour (0.5f, 0.5f, 0.5f),
            alpha (1.0f),
            visible (one != two) { }

        Connectome::Edge::Edge () :
            node_indices { 0, 0 },
            node_centres { Point<float>(), Point<float>() },
            dir (Point<float>()),
            size (0.0f),
            colour (0.0f, 0.0f, 0.0f),
            alpha (0.0f),
            visible (false) { }

        void Connectome::Edge::render_line() const
        {
          glColor3f (colour[0], colour[1], colour[2]);
          glBegin (GL_LINES);
          glVertex3f (node_centres[0][0], node_centres[0][1], node_centres[0][2]);
          glVertex3f (node_centres[1][0], node_centres[1][1], node_centres[1][2]);
          glEnd();
        }










        void Connectome::clear_all()
        {
          image_button ->setText ("");
          emit lut_open_slot (0);
          config_button->setText ("");
          nodes.clear();
          edges.clear();
          lut.clear();
        }

        void Connectome::initialise (const std::string& path)
        {

          // TODO This could be made faster by constructing the meshes on-the-fly
          // This would also allow calculation of vertex normals

          MR::Image::Header H (path);
          if (!H.datatype().is_integer())
            throw Exception ("Input parcellation image must have an integer datatype");
          voxel_volume = H.vox(0) * H.vox(1) * H.vox(2);
          MR::Image::Buffer<node_t> buffer (path);
          auto voxel = buffer.voxel();
          MR::Image::Transform transform (H);
          std::vector< Point<float> > node_coms;
          std::vector<size_t> node_volumes;
          VecPtr< MR::Image::BufferScratch<bool> > node_masks (1);
          VecPtr< MR::Image::BufferScratch<bool>::voxel_type > node_mask_voxels (1);
          size_t max_index = 0;

          {
            MR::Image::LoopInOrder loop (voxel, "Importing parcellation image... ");
            for (loop.start (voxel); loop.ok(); loop.next (voxel)) {
              const node_t node_index = voxel.value();
              if (node_index) {

                if (node_index >= max_index) {
                  node_coms       .resize (node_index+1, Point<float> (0.0f, 0.0f, 0.0f));
                  node_volumes    .resize (node_index+1, 0);
                  node_masks      .resize (node_index+1);
                  node_mask_voxels.resize (node_index+1);
                  for (size_t i = max_index+1; i <= node_index; ++i) {
                    node_masks[i] = new MR::Image::BufferScratch<bool> (H, "Node " + str(i));
                    node_mask_voxels[i] = new MR::Image::BufferScratch<bool>::voxel_type (*node_masks[i]);
                  }
                  max_index = node_index;
                }

                MR::Image::Nav::set_pos (*node_mask_voxels[node_index], voxel);
                node_mask_voxels[node_index]->value() = true;

                node_coms   [node_index] += transform.voxel2scanner (voxel);
                node_volumes[node_index]++;
              }
            }
          }
          for (node_t n = 1; n <= max_index; ++n)
            node_coms[n] *= (1.0f / float(node_volumes[n]));

          nodes.clear();

          // TODO In its current state, this could be multi-threaded
          {
            MR::ProgressBar progress ("Triangulating nodes...", max_index);
            nodes.push_back (Node());
            for (size_t i = 1; i <= max_index; ++i) {
              nodes.push_back (Node (node_coms[i], node_volumes[i], *node_masks[i]));
              ++progress;
            }
          }

          mat2vec = MR::Connectome::Mat2Vec (num_nodes());

          edges.clear();
          edges.reserve (mat2vec.vec_size());
          for (size_t edge_index = 0; edge_index != mat2vec.vec_size(); ++edge_index)
            edges.push_back (Edge (*this, mat2vec(edge_index).first + 1, mat2vec(edge_index).second + 1));

        }




        void Connectome::import_file_for_node_property (Math::Vector<float>& data, const std::string& attribute)
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
        }


        void Connectome::import_file_for_edge_property (Math::Vector<float>& data, const std::string& attribute)
        {
          data.clear();
          const std::string path = Dialog::File::get_file (this, "Select matrix file to determine edge " + attribute);
          if (path.empty())
            return;
          Math::Matrix<float> temp (path);
          MR::Connectome::verify_matrix (temp, num_nodes());
          mat2vec (temp, data);
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

          } else if (node_visibility == NODE_VIS_FILE) {

            assert (node_values_from_file_visibility.size());
            for (size_t i = 1; i <= num_nodes(); ++i)
              nodes[i].set_visible (node_values_from_file_visibility[i-1]);

          } else if (node_visibility == NODE_VIS_DEGREE) {

            // TODO Need full connectome matrix, as well as current edge visualisation
            //   thresholds, in order to calculate this
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





