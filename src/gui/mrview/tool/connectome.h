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

#include "gui/mrview/tool/base.h"
#include "gui/projection.h"

#include "mesh/mesh.h"

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

          public:
            class Model;

            Connectome (Window& main_window, Dock* parent);

            virtual ~Connectome ();

            void draw (const Projection& transform, bool is_3D, int axis, int slice);
            void drawOverlays (const Projection& transform) override;
            bool process_batch_command (const std::string& cmd, const std::string& args);


          private slots:
            void image_open_slot ();
            void lut_open_slot ();
            void config_open_slot ();
            void hide_all_slot ();

          protected:

            // TODO Toolbar format for connectome:
            // Basic configuration options:
            // * Button to select template parcellation image
            // * Button to import colour lookup table
            // * Button to import connectome config file
            //
            // Node display options:
            // * Geometry: Mesh, overlay, sphere, ...
            // * Colour by: LUT (if available), file w. colour map, fixed colour, ...
            // * Size by: file
            // * Visibility: file
            //
            // Edge display options:
            // * Geometry: Line, cylinder, ...
            // * Colour by: file w. colour map, fixed colour, ...
            // * Size by: file
            // * Visibility: file

            QGroupBox* basic_option_group;
            QPushButton *image_button, *lut_button, *config_button, *hide_all_button;


          private:

            // TODO Helper class to manage the storage and display of the mesh for each node
            class Node_mesh
            {

              public:
                // This should transfer all mesh data onto the GPU, and just store the
                //   necessary references
                Node_mesh (const Mesh::Mesh&);
                ~Node_mesh();

                // TODO This should also handle the rendering; allows for a fixed colour per node
                //   without having to create a redundant colour vertex array
                // Actually, may be able to do this in the calling draw() function by resetting
                //   the colour using gl::GetUniformLocation()
                void render();

              private:
                const GLsizei count;
                GLuint vertex_buffer, vertex_array_object, index_buffer;

            };



            // Keep the number of nodes here handy & ready to go
            node_t num_nodes;
            // TODO Store the centre of mass for each node
            std::vector< Point<float> > centres_of_mass;
            // TODO Store a mesh representation of each node
            // Since this should not need to be manipulated in any way, it should be
            //   possible to store them in whatever format OpenGL wants them in
            std::vector<Node_mesh> node_meshes;

            // If a connectome config file is provided, this will map from the
            //   value in the image to the appropriate index of the lookup table
            std::map<node_t, node_t> lookup;
            // If a lookup table is provided, this container will store the
            //   properties of each node (e.g. name & colour)
            Node_map node_map;


            // TODO Helper functions
            void clear_all();
            void initialise (const std::string&);






        };








      }
    }
  }
}

#endif




