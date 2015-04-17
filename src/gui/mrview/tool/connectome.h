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
#include "gui/mrview/tool/base.h"
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
        class Connectome : public Base
        {
            Q_OBJECT

            typedef MR::DWI::Tractography::Connectomics::node_t    node_t;
            typedef MR::DWI::Tractography::Connectomics::Node_info Node_info;
            typedef MR::DWI::Tractography::Connectomics::Node_map  Node_map;

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

            size_t num_nodes() const { return nodes.size(); }

          private slots:
            void image_open_slot ();
            void lut_open_slot (int);
            void config_open_slot ();
            void hide_all_slot ();

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
            // * Size by: file
            // * Visibility: file, degree!=0
            //
            // Edge display options:
            // * Geometry: Line, cylinder, ...
            // * Colour by: file w. colour map, fixed colour, ...
            // * Size by: file
            // * Visibility: file

            //
            // Questions:
            // * Should connectomes and parcellation images be paired? Not convinced that this makes sense,
            //     as ideally the underlying image would need to change between subjects also.

            QPushButton *image_button, *hide_all_button;;
            QComboBox *lut_combobox;
            QPushButton *config_button;




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

                // TODO Might as well store display options in here
                void set_name (const std::string& i) { name = i; }
                const std::string& get_name() const { return name; }
                void set_size (const float i) { size = i; }
                float get_size() const { return size; }
                void set_colour (const Point<float>& i) { colour = i; }
                const Point<float>& get_colour() const { return colour; }
                void set_visible (const bool i) { visible = i; }
                bool is_visible() const { return visible; }


              private:
                const Point<float> centre_of_mass;
                const size_t volume;

                std::string name;
                float size;
                Point<float> colour;
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
            MR::DWI::Tractography::Connectomics::ConfigInvLookup config;



            // TODO Helper functions
            void clear_all();
            void initialise (const std::string&);
            void load_node_properties();

        };








      }
    }
  }
}

#endif




