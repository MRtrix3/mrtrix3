/*
   Copyright 2014 Brain Research Institute, Melbourne, Australia

   Written by David Raffelt 2014.

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

#ifndef __gui_mrview_tool_connectome_h__
#define __gui_mrview_tool_connectome_h__

#include <map>
#include <vector>

#include "point.h"

#include "gui/opengl/shader.h"
#include "gui/mrview/adjust_button.h"
#include "gui/mrview/colourmap_button.h"
#include "gui/mrview/tool/base.h"
#include "gui/color_button.h"
#include "gui/projection.h"

#include "mesh/mesh.h"

#include "dwi/tractography/connectomics/config.h"
#include "dwi/tractography/connectomics/connectomics.h"
#include "dwi/tractography/connectomics/lut.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {
        class Connectome : public Base, public ColourMapButtonObserver
        {
            Q_OBJECT

            typedef MR::DWI::Tractography::Connectomics::node_t    node_t;
            typedef MR::DWI::Tractography::Connectomics::Node_info Node_info;
            typedef MR::DWI::Tractography::Connectomics::Node_map  Node_map;

            enum node_geometry_t   { NODE_GEOM_SPHERE, NODE_GEOM_OVERLAY, NODE_GEOM_MESH };
            enum node_colour_t     { NODE_COLOUR_FIXED, NODE_COLOUR_RANDOM, NODE_COLOUR_LUT, NODE_COLOUR_FILE };
            enum node_size_t       { NODE_SIZE_FIXED, NODE_SIZE_VOLUME, NODE_SIZE_FILE };
            enum node_visibility_t { NODE_VIS_ALL, NODE_VIS_FILE, NODE_VIS_DEGREE, NODE_VIS_MANUAL };
            enum node_alpha_t      { NODE_ALPHA_FIXED, NODE_ALPHA_LUT, NODE_ALPHA_FILE };

          private:

            class Shader : public GL::Shader::Program {
              public:
                Shader() : GL::Shader::Program () { }
                virtual ~Shader() { }

                bool need_update (const Connectome&) const;
                virtual void update (const Connectome&) = 0;

                void start (const Connectome& parent) {
                  if (*this == 0 || need_update (parent))
                    recompile (parent);
                  GL::Shader::Program::start();
                }

              protected:
                std::string vertex_shader_source, fragment_shader_source;
                // TODO edge shader using lines will make a lot more sense with a geometry shader:
                //   set up the relevant parameters, then emit two vertices at two locations

              private:
                void recompile (const Connectome& parent);
            };

            class NodeShader : public Shader
            {
              public:
                NodeShader() : Shader () { }
                ~NodeShader() { }
                void update (const Connectome&) override;
            } node_shader;

            class EdgeShader : public Shader
            {
              public:
                EdgeShader() : Shader () { }
                ~EdgeShader() { }
                void update (const Connectome&) override;
            } edge_shader;


          public:

            Connectome (Window& main_window, Dock* parent);

            virtual ~Connectome ();

            void draw (const Projection& transform, bool is_3D, int axis, int slice);
            void drawOverlays (const Projection& transform) override;
            bool process_batch_command (const std::string& cmd, const std::string& args);

            size_t num_nodes() const { return nodes.size() - 1; }

          private slots:
            void image_open_slot ();
            void lut_open_slot (int);
            void config_open_slot ();
            void hide_all_slot ();

            void node_geometry_selection_slot (int);
            void node_colour_selection_slot (int);
            void node_size_selection_slot (int);
            void node_visibility_selection_slot (int);
            void node_alpha_selection_slot (int);

            void sphere_lod_slot (int);
            void node_colour_change_slot();
            void node_size_value_slot();
            void node_alpha_value_slot (int);

          protected:

            // TODO Toolbar format for connectome:
            // Basic configuration options:
            // * Button to select template parcellation image
            // * Button to import colour lookup table
            // * Button to import connectome config file
            //
            // * Connectome files (list)
            //
            // Node display options:
            // * Geometry: Mesh, overlay, sphere, ...
            // * Colour by: LUT (if available), file w. colour map, fixed colour, random, ...
            // * Size by: fixed, volume, file
            // * Visibility: all, file, degree!=0, manual (list view appears to toggle nodes)
            // * Transparency: fixed, by file
            //
            // Edge display options:
            // * Geometry: Line, cylinder, ...
            // * Colour by: file w. colour map, fixed colour, ... direction?
            // * Size by: fixed, file
            // * Visibility: all, file
            // * Transparency: fixed, weights, file

            //
            // Questions:
            // * Should connectomes and parcellation images be paired? Not convinced that this makes sense,
            //     as ideally the underlying image would need to change between subjects also.

            // TODO When node geometry is an overlay image, suspect it will be faster to store a coloured
            //   texture buffer, which can be used to overlay in either 2D or 3D, but will need to be
            //   updated whenever a relevant setting (e.g. node visibility, colours) changes

            // TODO Change layout here: Should probably be a grid covering all lines
            // Also: Colour settings may need more space; preferably its own row directly below

            QPushButton *image_button, *hide_all_button;
            QComboBox *lut_combobox;
            QPushButton *config_button;

            QComboBox *node_geometry_combobox, *node_colour_combobox, *node_size_combobox, *node_visibility_combobox, *node_alpha_combobox;

            // TODO Elements that need to appear / change based on these selections:
            // - If node size is based on a file, should automatically give a dialog to select the file;
            //     then also need to print the base name of the file next to the combo box, and will need
            //     additional scaling controls, e.g. upper and lower thesholds
            //     (existing slider can be used to control a global scaling factor, but need a way to map the
            //     range of values within the file to additional scaling factors)
            QLabel *node_geometry_sphere_lod_label;
            QSpinBox *node_geometry_sphere_lod_spinbox;

            QColorButton *node_colour_fixedcolour_button;
            ColourMapButton *node_colour_colourmap_button;

            AdjustButton *node_size_button;

            QSlider *node_alpha_slider;




          private:

            // Stores all information relating to the drawing of individual nodes, both fixed and variable
            class Node
            {
              public:
                Node (const Point<float>&, const size_t, MR::Image::BufferScratch<bool>&);
                Node ();

                void render_mesh() const { mesh.render(); }

                const Point<float>& get_com() const { return centre_of_mass; }
                size_t get_volume() const { return volume; }

                // TODO Might as well store display options in here, rather than individual vectors;
                //   these will be set by the main thread before data is applied to the shaders anyways
                void set_name (const std::string& i) { name = i; }
                const std::string& get_name() const { return name; }
                void set_size (const float i) { size = i; }
                float get_size() const { return size; }
                void set_colour (const Point<float>& i) { colour = i; }
                const Point<float>& get_colour() const { return colour; }
                void set_alpha (const float i) { alpha = i; }
                float get_alpha() const { return alpha; }
                void set_visible (const bool i) { visible = i; }
                bool is_visible() const { return visible; }


              private:
                const Point<float> centre_of_mass;
                const size_t volume;

                std::string name;
                float size;
                Point<float> colour;
                float alpha;
                bool visible;

                // Helper class to manage the storage and display of the mesh for each node
                class Mesh {
                  public:
                    Mesh (const MR::Mesh::Mesh&);
                    Mesh (const Mesh&) = delete;
                    Mesh (Mesh&&);
                    Mesh ();
                    ~Mesh() { }
                    Mesh& operator= (Mesh&&);
                    void render() const;
                  private:
                    GLsizei count;
                    GL::VertexBuffer vertex_buffer;
                    GL::VertexArrayObject vertex_array_object;
                    GL::IndexBuffer index_buffer;
                } mesh;

                // TODO Helper class to manage the storage and display of the mask volume for each node

            };
            std::vector<Node> nodes;


            // If a lookup table is provided, this container will store the
            //   properties of each node as provided in that file (e.g. name & colour)
            Node_map lut;

            // If a connectome configuration file is provided, this will map
            //   each structure name to an index in the parcellation image;
            //   this can then be used to produce the lookup table
            std::vector<std::string> config;

            // TODO If both a LUT and a config file have been provided, it would be
            //   nice to have a direct vector mapping from image node index to
            //   a position in the lookup table, pre-generated
            std::vector<Node_map::const_iterator> lut_mapping;


            // Current node visualisation settings
            node_geometry_t node_geometry;
            node_colour_t node_colour;
            node_size_t node_size;
            node_visibility_t node_visibility;
            node_alpha_t node_alpha;

            // Other values that need to be stored w.r.t. node visualisation
            Point<float> node_fixed_colour;
            float node_fixed_alpha;
            float node_size_scale_factor;
            Math::Vector<float> node_values_from_file_colour;
            Math::Vector<float> node_values_from_file_size;
            Math::Vector<float> node_values_from_file_visibility;
            Math::Vector<float> node_values_from_file_alpha;




            // TODO Helper functions
            void clear_all();
            void initialise (const std::string&);

            void import_file_for_node_property (Math::Vector<float>&, const std::string&);

            void load_node_properties();
            void calculate_node_colours();
            void calculate_node_sizes();
            void calculate_node_visibility();
            void calculate_node_alphas();

        };








      }
    }
  }
}

#endif




