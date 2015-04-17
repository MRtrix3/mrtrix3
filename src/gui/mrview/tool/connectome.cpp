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

      // For now, assume that all nodes are being drawn based on the mesh;
      //   branches can be added later
        void Connectome::NodeShader::update (const Connectome& /*parent*/)
        {
          vertex_shader_source =
              "layout (location = 0) in vec3 vertexPosition_modelspace;\n"
              "uniform mat4 MVP;\n";

          vertex_shader_source +=
              "void main() {\n"
              "  gl_Position = MVP * vec4 (vertexPosition_modelspace, 1);\n";

          vertex_shader_source += "}\n";

          fragment_shader_source =
              "out vec3 color;\n"
              "in vec3 fragmentColor;\n";

          fragment_shader_source +=
              "void main(){\n";

          fragment_shader_source +=
              //"  color = fragmentColor;\n";
              "  color = vec3(1,0,0);\n";

          fragment_shader_source += "}\n";
        }

        void Connectome::EdgeShader::update (const Connectome& /*parent*/) { }













        Connectome::Connectome (Window& main_window, Dock* parent) :
            Base (main_window, parent),
            node_geometry (NODE_GEOM_SPHERE),
            node_colour (NODE_COLOUR_FIXED),
            node_size (NODE_SIZE_FIXED),
            node_visibility (NODE_VIS_ALL),
            node_fixed_colour (0.5f, 0.5f, 0.5f),
            node_size_scale_factor (0.0f)
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
          image_button->setToolTip (tr ("Open parcellation image"));
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
          vlayout = new VBoxLayout;
          group_box->setLayout (vlayout);

          hlayout = new HBoxLayout;
          hlayout->setContentsMargins (0, 0, 0, 0);
          hlayout->setSpacing (0);

          QLabel* label = new QLabel ("Geometry: ");
          hlayout->addWidget (label);
          node_geometry_combobox = new QComboBox (this);
          node_geometry_combobox->setToolTip (tr ("The 3D geometrical shape used to draw each node"));
          node_geometry_combobox->addItem ("Sphere");
          node_geometry_combobox->addItem ("Overlay");
          node_geometry_combobox->addItem ("Mesh");
          connect (node_geometry_combobox, SIGNAL (activated(int)), this, SLOT (node_geometry_slot (int)));
          hlayout->addWidget (node_geometry_combobox, 1);
          vlayout->addLayout(hlayout);

          hlayout = new HBoxLayout;
          hlayout->setContentsMargins (0, 0, 0, 0);
          hlayout->setSpacing (0);

          label = new QLabel ("Colour: ");
          hlayout->addWidget (label);
          node_colour_combobox = new QComboBox (this);
          node_colour_combobox->setToolTip (tr ("Set the colour of each node"));
          node_colour_combobox->addItem ("Fixed");
          node_colour_combobox->addItem ("Random");
          node_colour_combobox->addItem ("Lookup table");
          node_colour_combobox->addItem ("From scalar file");
          connect (node_colour_combobox, SIGNAL (activated(int)), this, SLOT (node_colour_slot (int)));
          hlayout->addWidget (node_colour_combobox, 1);
          vlayout->addLayout (hlayout);

          hlayout = new HBoxLayout;
          hlayout->setContentsMargins (0, 0, 0, 0);
          hlayout->setSpacing (0);

          label = new QLabel ("Size scaling: ");
          hlayout->addWidget (label);
          node_size_combobox = new QComboBox (this);
          node_size_combobox->setToolTip (tr ("Scale the size of each node"));
          node_size_combobox->addItem ("Fixed");
          node_size_combobox->addItem ("Volume");
          node_size_combobox->addItem ("From scalar file");
          connect (node_size_combobox, SIGNAL (activated(int)), this, SLOT (node_size_slot (int)));
          hlayout->addWidget (node_size_combobox, 1);
          vlayout->addLayout (hlayout);

          hlayout = new HBoxLayout;
          hlayout->setContentsMargins (0, 0, 0, 0);
          hlayout->setSpacing (0);

          label = new QLabel ("Visibility: ");
          hlayout->addWidget (label);
          node_visibility_combobox = new QComboBox (this);
          node_visibility_combobox->setToolTip (tr ("Set which nodes are visible"));
          node_visibility_combobox->addItem ("All");
          node_visibility_combobox->addItem ("From scalar file");
          node_visibility_combobox->addItem ("Degree >= 1");
          node_visibility_combobox->addItem ("Manual");
          connect (node_visibility_combobox, SIGNAL (activated(int)), this, SLOT (node_visibility_slot (int)));
          hlayout->addWidget (node_visibility_combobox, 1);
          vlayout->addLayout (hlayout);

          main_box->addStretch ();
          setMinimumSize (main_box->minimumSize());

          image_open_slot();
        }


        Connectome::~Connectome () {}


        void Connectome::draw (const Projection& projection, bool /*is_3D*/, int /*axis*/, int /*slice*/)
        {
          if (hide_all_button->isChecked()) return;

          node_shader.start (*this);
          projection.set (node_shader);

          gl::Disable (gl::BLEND);
          gl::Enable (gl::DEPTH_TEST);
          gl::DepthMask (gl::TRUE_);

          //for (size_t i = 1; i != num_nodes(); ++i)
          //  nodes[i].render_mesh();
          nodes[44].render_mesh();

          node_shader.stop();
        }


        //void Connectome::drawOverlays (const Projection& transform)
        void Connectome::drawOverlays (const Projection&)
        {
          if (hide_all_button->isChecked()) return;
        }


        //bool Connectome::process_batch_command (const std::string& cmd, const std::string& args)
        bool Connectome::process_batch_command (const std::string&, const std::string&)
        {
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
        }


        void Connectome::lut_open_slot (int index)
        {
          if (!index) {
            lut.clear();
            lut_lookup.clear();
            //lut_namebox->setText (QString::fromStdString ("(none)"));
            lut_combobox->removeItem (5);
            load_node_properties();
            return;
          }
          if (index == 5)
            return; // Selected currently-open LUT; nothing to do

          const std::string path = Dialog::File::get_file (this, std::string("Select lookup table file (in ") + MR::DWI::Tractography::Connectomics::lut_format_strings[index] + " format)");
          if (path.empty())
            return;

          lut.clear();
          lut_lookup.clear();
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

          load_node_properties();
        }


        void Connectome::config_open_slot()
        {
          const std::string path = Dialog::File::get_file (this, "Select connectome configuration file");
          if (path.empty())
            return;
          config.clear();
          lut_lookup.clear();
          config_button->setText ("");
          MR::DWI::Tractography::Connectomics::load_config (path, config);
          config_button->setText (QString::fromStdString (Path::basename (path)));
          load_node_properties();
        }


        void Connectome::hide_all_slot()
        {
          window.updateGL();
        }






        void Connectome::node_geometry_slot (int index)
        {
          switch (index) {
            case 0:
              if (node_geometry == NODE_GEOM_SPHERE) return;
              node_geometry = NODE_GEOM_SPHERE;
              node_size_combobox->setEnabled (true);
              break;
            case 1:
              if (node_geometry == NODE_GEOM_OVERLAY) return;
              node_geometry = NODE_GEOM_OVERLAY;
              node_size_combobox->setCurrentIndex (0);
              node_size_combobox->setEnabled (false);
              break;
            case 2:
              if (node_geometry == NODE_GEOM_MESH) return;
              node_geometry = NODE_GEOM_MESH;
              node_size_combobox->setCurrentIndex (0);
              node_size_combobox->setEnabled (false);
              break;
          }
        }

        void Connectome::node_colour_slot (int index)
        {
          switch (index) {
            case 0:
              // if (node_colour == NODE_COLOUR_FIXED) return; // TODO Should this prompt a new colour selection? Means no need for a button...
              node_colour = NODE_COLOUR_FIXED;
              break;
            case 1:
              //if (node_colour == NODE_COLOUR_RANDOM) return; // Keep this; regenerate random colours on repeat selection
              node_colour = NODE_COLOUR_RANDOM;
              break;
            case 2:
              if (node_colour == NODE_COLOUR_LUT) return;
              // TODO Pointless selection if no LUT is loaded...
              node_colour = NODE_COLOUR_LUT;
              break;
            case 3:
              //if (node_colour == NODE_COLOUR_FILE) return; // Keep this; may want to select a new file
              node_colour = NODE_COLOUR_FILE;
              // TODO Prompt the user to import a file
              // TODO Make the relevant GUI elements visible: lower & upper thresholds, colour map selection & invert option, ...
              break;
          }
          calculate_node_colours();
          // Probably need to updateGL() as well...
        }

        void Connectome::node_size_slot (int index)
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
              node_size = NODE_SIZE_FILE;
              break;
          }
          calculate_node_sizes();
        }

        void Connectome::node_visibility_slot (int index)
        {
          switch (index) {
            case 0:
              node_visibility = NODE_VIS_ALL;
              break;
            case 1:
              node_visibility = NODE_VIS_FILE;
              break;
            case 2:
              node_visibility = NODE_VIS_DEGREE;
              break;
            case 3:
              node_visibility = NODE_VIS_MANUAL;
              // TODO Here is where the corresponding list view should be made visible
              break;
          }
          calculate_node_visibility();
        }

        void Connectome::node_alpha_slot (int index)
        {
          switch (index) {
            case 0:
              node_alpha = NODE_ALPHA_FIXED;
              break;
            case 1:
              node_alpha = NODE_ALPHA_LUT;
              break;
            case 2:
              node_alpha = NODE_ALPHA_FILE;
              break;
          }
          calculate_node_alphas();
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
            count (in.num_triangles())
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
          vertex_array_object.bind();
          index_buffer.bind();
          gl::DrawArrays (gl::TRIANGLES, 0, count);
        }







        void Connectome::clear_all()
        {
          image_button ->setText ("");
          emit lut_open_slot (0);
          config_button->setText ("");
          nodes.clear();
          lut.clear();
        }

        void Connectome::initialise (const std::string& path)
        {

          // TODO This could be made faster by constructing the meshes on-the-fly

          MR::Image::Header H (path);
          if (!H.datatype().is_integer())
            throw Exception ("Input parcellation image must have an integer datatype");
          MR::Image::Buffer<node_t> buffer (path);
          auto voxel = buffer.voxel();
          MR::Image::Transform transform (H);
          std::vector< Point<float> > node_coms;
          std::vector<size_t> node_volumes;
          VecPtr< MR::Image::BufferScratch<bool> > node_masks (1);
          VecPtr< MR::Image::BufferScratch<bool>::voxel_type > node_mask_voxels (1);
          size_t node_count = 0;

          {
            MR::Image::LoopInOrder loop (voxel, "Importing parcellation image... ");
            for (loop.start (voxel); loop.ok(); loop.next (voxel)) {
              const node_t node_index = voxel.value();
              if (node_index) {

                if (node_index >= node_count) {
                  node_coms       .resize (node_index+1, Point<float> (0.0f, 0.0f, 0.0f));
                  node_volumes    .resize (node_index+1, 0);
                  node_masks      .resize (node_index+1);
                  node_mask_voxels.resize (node_index+1);
                  for (size_t i = node_count+1; i <= node_index; ++i) {
                    node_masks[i] = new MR::Image::BufferScratch<bool> (H, "Node " + str(i));
                    node_mask_voxels[i] = new MR::Image::BufferScratch<bool>::voxel_type (*node_masks[i]);
                  }
                  node_count = node_index;
                }

                MR::Image::Nav::set_pos (*node_mask_voxels[node_index], voxel);
                node_mask_voxels[node_index]->value() = true;

                node_coms   [node_index] += transform.voxel2scanner (voxel);
                node_volumes[node_index]++;
              }
            }
          }
          for (node_t n = 1; n != node_count; ++n)
            node_coms[n] *= (1.0f / float(node_volumes[n]));

          // TODO In its current state, this could be multi-threaded
          {
            MR::ProgressBar progress ("Triangulating nodes...", node_count);
            nodes.push_back (Node());
            for (size_t i = 1; i != node_count; ++i) {
              nodes.push_back (Node (node_coms[i], node_volumes[i], *node_masks[i]));
              ++progress;
            }
          }

        }





        void Connectome::load_node_properties()
        {
          lut_lookup.clear();
          if (lut.size()) {

            lut_lookup.push_back (lut.end());
            for (size_t node_index = 1; node_index != num_nodes()+1; ++node_index) {

              if (config.size()) {
                const std::string name = config[node_index];
                nodes[node_index].set_name (name);
                Node_map::const_iterator it;
                for (it = lut.begin(); it != lut.end(); ++it) {
                  if (it->second.get_name() == name) {
                    lut_lookup.push_back (it);
                    continue;
                  }
                }
                if (it == lut.end())
                  lut_lookup.push_back (it);

              } else { // LUT, but no config file

                const auto it = lut.find (node_index);
                if (it == lut.end())
                  nodes[node_index].set_name ("Node " + str(node_index));
                else
                  nodes[node_index].set_name (it->second.get_name());
                lut_lookup.push_back (it);

              }

            } // End looping over all nodes when LUT is present

          } else { // No LUT; just name nodes according to their indices

            lut_lookup.assign (num_nodes()+1, lut.end());
            for (size_t node_index = 1; node_index != num_nodes()+1; ++node_index)
              nodes[node_index].set_name ("Node " + str(node_index));

          }

          calculate_node_colours();
          calculate_node_sizes();
          calculate_node_visibility();
          calculate_node_alphas();

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
            for (size_t node_index = 1; node_index != num_nodes()+1; ++node_index) {
              if (lut_lookup[node_index] == lut.end())
                nodes[node_index].set_colour (node_fixed_colour);
              else
                nodes[node_index].set_colour (Point<float> (lut_lookup[node_index]->second.get_colour()) / 255.0f);
            }

          } else if (node_colour == NODE_COLOUR_FILE) {

            // TODO Probably actually nothing to do here;
            //   shader will branch in order to feed the raw value from the imported file into a colour
            //   (will need to send the shader a scalar rather than a vec3)
            // This will then enable use of all possible colour maps

          }
        }



        void Connectome::calculate_node_sizes()
        {
          if (node_size == NODE_SIZE_FIXED) {

            for (auto i = nodes.begin(); i != nodes.end(); ++i)
              i->set_size (1.0f);

          } else if (node_size == NODE_SIZE_VOLUME) {

            // TODO Improve this heuristic
            // For example: Could take into consideration the voxel size of the image, then
            //   scale so that the sphere takes up approximately the same volume as the node itself
            for (auto i = nodes.begin(); i != nodes.end(); ++i)
              i->set_size (i->get_volume());

          } else if (node_size == NODE_SIZE_FILE) {

            assert (node_values_from_file_size.size());
            for (size_t i = 1; i != num_nodes()+1; ++i)
              nodes[i].set_size (node_values_from_file_size[i-1]);

          }
        }



        void Connectome::calculate_node_visibility()
        {

          if (node_visibility == NODE_VIS_ALL) {

            for (auto i = nodes.begin(); i != nodes.end(); ++i)
              i->set_visible (true);

          } else if (node_visibility == NODE_VIS_FILE) {

            assert (node_values_from_file_visibility.size());
            for (size_t i = 1; i != num_nodes()+1; ++i)
              nodes[i].set_visible (node_values_from_file_visibility[i-1]);

          } else if (node_visibility == NODE_VIS_DEGREE) {

            // TODO Need full connectome matrix, as well as current edge visualisation
            //   thresholds, in order to calculate this

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
            for (size_t node_index = 1; node_index != num_nodes()+1; ++node_index) {
              if (lut_lookup[node_index] == lut.end())
                nodes[node_index].set_alpha (node_fixed_alpha);
              else
                nodes[node_index].set_alpha (lut_lookup[node_index]->second.get_alpha() / 255.0f);
            }

          } else if (node_alpha == NODE_ALPHA_FILE) {

            assert (node_values_from_file_alpha.size());
            for (size_t i = 1; i != num_nodes()+1; ++i)
              nodes[i].set_visible (node_values_from_file_alpha[i-1]);

          }
        }




      }
    }
  }
}





