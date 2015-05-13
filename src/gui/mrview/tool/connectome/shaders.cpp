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

#include "gui/mrview/tool/connectome/shaders.h"

#include "gui/mrview/tool/connectome/connectome.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {




        void ShaderBase::recompile (const Connectome& parent)
        {
          if (*this != 0)
            clear();
          update (parent);
          GL::Shader::Vertex vertex_shader (vertex_shader_source);
          GL::Shader::Geometry geometry_shader (geometry_shader_source);
          GL::Shader::Fragment fragment_shader (fragment_shader_source);
          attach (vertex_shader);
          if ((GLuint) geometry_shader)
            attach (geometry_shader);
          attach (fragment_shader);
          link();
        }







        bool NodeShader::need_update (const Connectome& parent) const
        {
          if (geometry != parent.node_geometry) return true;
          if (colour != parent.node_colour) return true;
          if (colour == NODE_COLOUR_FILE && colourmap_index != parent.node_colourmap_index) return true;
          const bool need_alpha = !(parent.node_alpha == NODE_ALPHA_FIXED && parent.node_fixed_alpha == 1.0f);
          if (use_alpha != need_alpha) return true;
          return false;
        }

        void NodeShader::update (const Connectome& parent)
        {
          geometry = parent.node_geometry;
          colour = parent.node_colour;
          colourmap_index = parent.node_colourmap_index;
          use_alpha = !(parent.node_alpha == NODE_ALPHA_FIXED && parent.node_fixed_alpha == 1.0f);

          vertex_shader_source =
              "layout (location = 0) in vec3 vertexPosition_modelspace;\n";

          if (geometry == NODE_GEOM_CUBE || geometry == NODE_GEOM_MESH || geometry == NODE_GEOM_SMOOTH_MESH) {
            vertex_shader_source +=
              "layout (location = 1) in vec3 vertexNormal_modelspace;\n";
          }

          vertex_shader_source +=
              "uniform mat4 MVP;\n";

          if (geometry != NODE_GEOM_OVERLAY) {
            vertex_shader_source +=
              "uniform vec3 node_centre;\n"
              "uniform float node_size;\n";
          }

          if (geometry == NODE_GEOM_SPHERE) {
            vertex_shader_source +=
              "uniform int reverse;\n";
          }

          if (geometry == NODE_GEOM_SPHERE || geometry == NODE_GEOM_MESH || geometry == NODE_GEOM_SMOOTH_MESH) {
            vertex_shader_source +=
              "out vec3 normal;\n";
          } else if (geometry == NODE_GEOM_CUBE) {
            vertex_shader_source +=
              "flat out vec3 normal;\n";
          }

          vertex_shader_source +=
              "void main() {\n";

          switch (geometry) {
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
              "  vec3 pos = vertexPosition_modelspace * node_size;\n"
              "  gl_Position = (MVP * vec4 (node_centre + pos, 1));\n"
              "  normal = vertexNormal_modelspace;\n";
              break;
            case NODE_GEOM_OVERLAY:
              break;
            case NODE_GEOM_MESH:
            case NODE_GEOM_SMOOTH_MESH:
              vertex_shader_source +=
              "  normal = vertexNormal_modelspace;\n"
              "  vec3 pos = (node_size * (vertexPosition_modelspace - node_centre));\n"
              "  gl_Position = MVP * vec4 (node_centre + pos, 1);\n";
              break;
          }

          vertex_shader_source +=
              "}\n";

          // =================================================================

          geometry_shader_source = std::string("");

          // =================================================================

          fragment_shader_source =
              "uniform vec3 node_colour;\n";

          if (use_alpha) {
            fragment_shader_source +=
              "uniform float node_alpha;\n"
              "out vec4 color;\n";
          } else {
            fragment_shader_source +=
              "out vec3 color;\n";
          }

          if (geometry != NODE_GEOM_OVERLAY) {
            fragment_shader_source +=
              "uniform float ambient, diffuse, specular, shine;\n"
              "uniform vec3 light_pos;\n"
              "uniform vec3 screen_normal;\n";
          }
          if (geometry == NODE_GEOM_SPHERE || geometry == NODE_GEOM_MESH || geometry == NODE_GEOM_SMOOTH_MESH) {
            fragment_shader_source +=
              "in vec3 normal;\n";
          } else if (geometry == NODE_GEOM_CUBE) {
            fragment_shader_source +=
              "flat in vec3 normal;\n";
          }

          if (geometry != NODE_GEOM_OVERLAY) {
            fragment_shader_source +=
              "in vec3 position;\n";
          }

          if (colour == NODE_COLOUR_FILE && ColourMap::maps[colourmap_index].is_colour) {
            fragment_shader_source +=
              "in vec3 colourmap_colour;\n";
          }

          fragment_shader_source +=
              "void main() {\n";

          if (colour == NODE_COLOUR_FILE) {

            // Red component of node_colour is the position within the range [0, 1] based on the current settings;
            //   use this to derive the actual colour based on the selected mapping
            fragment_shader_source +=
              "  float amplitude = node_colour.r;\n";
            fragment_shader_source += std::string("  ") + ColourMap::maps[colourmap_index].mapping;

          } else {

            if (use_alpha) {
              fragment_shader_source +=
                  "  color.rgb = node_colour;\n";
            } else {
              fragment_shader_source +=
                  "  color = node_colour;\n";
            }

          }

          if (geometry != NODE_GEOM_OVERLAY) {
            fragment_shader_source +=
              "  color *= ambient + diffuse * clamp (dot (normal, light_pos), 0, 1);\n"
              "  color += specular * pow (clamp (dot (reflect (light_pos, normal), screen_normal), 0, 1), shine);\n";
          }

          if (use_alpha) {
            fragment_shader_source +=
              "  color.a = node_alpha;\n";
          }

          fragment_shader_source +=
              "}\n";
        }






        bool EdgeShader::need_update (const Connectome& parent) const
        {
          if (geometry != parent.edge_geometry) return true;
          if (colour != parent.edge_colour) return true;
          if (colour == EDGE_COLOUR_FILE && colourmap_index != parent.edge_colourmap_index) return true;
          const bool need_alpha = !(parent.edge_alpha == EDGE_ALPHA_FIXED && parent.edge_fixed_alpha == 1.0f);
          if (use_alpha != need_alpha) return true;
          return false;
        }

        void EdgeShader::update (const Connectome& parent)
        {
          geometry = parent.edge_geometry;
          colour = parent.edge_colour;
          colourmap_index = parent.edge_colourmap_index;
          use_alpha = !(parent.edge_alpha == EDGE_ALPHA_FIXED && parent.edge_fixed_alpha == 1.0f);

          vertex_shader_source =
              "layout (location = 0) in vec3 vertexPosition_modelspace;\n"
              "uniform mat4 MVP;\n";

          if (geometry == EDGE_GEOM_CYLINDER) {
            vertex_shader_source +=
              "layout (location = 1) in vec3 vertexNormal_modelspace;\n"
              "uniform vec3 centre_one, centre_two;\n"
              "uniform mat3 rot_matrix;\n"
              "uniform float radius;\n"
              "out vec3 normal;\n";
          }

          vertex_shader_source +=
              "void main() {\n";

          switch (geometry) {
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
              "  normal = vertexNormal_modelspace * rot_matrix;\n"
              "  gl_Position = MVP * vec4 (centre + (radius * offset), 1);\n";
              break;
          }

          vertex_shader_source +=
              "}\n";

          // =================================================================

          geometry_shader_source = std::string("");

          // =================================================================

          fragment_shader_source =
              "uniform vec3 edge_colour;\n";

          if (use_alpha) {
            fragment_shader_source +=
              "uniform float edge_alpha;\n"
              "out vec4 color;\n";
          } else {
            fragment_shader_source +=
              "out vec3 color;\n";
          }

          if (geometry == EDGE_GEOM_CYLINDER) {
            fragment_shader_source +=
              "in vec3 normal;\n"
              "uniform float ambient, diffuse, specular, shine;\n"
              "uniform vec3 light_pos;\n"
              "uniform vec3 screen_normal;\n";
          }

          if (colour == EDGE_COLOUR_FILE && ColourMap::maps[colourmap_index].is_colour) {
            fragment_shader_source +=
              "in vec3 colourmap_colour;\n";
          }

          fragment_shader_source +=
              "void main() {\n";

          if (colour == EDGE_COLOUR_FILE) {

            fragment_shader_source +=
              "  float amplitude = edge_colour.r;\n";
            fragment_shader_source += std::string("  ") + ColourMap::maps[colourmap_index].mapping;

          } else {

            if (use_alpha) {
              fragment_shader_source +=
                  "  color.xyz = edge_colour;\n";
            } else {
              fragment_shader_source +=
                  "  color = edge_colour;\n";
            }

          }

          if (geometry == EDGE_GEOM_CYLINDER) {
            fragment_shader_source +=
              "  color *= ambient + diffuse * clamp (dot (normal, light_pos), 0, 1);\n"
              "  color += specular * pow (clamp (dot (reflect (light_pos, normal), screen_normal), 0, 1), shine);\n";
          }

          if (use_alpha) {
            fragment_shader_source +=
              "  color.a = edge_alpha;\n";
          }

          fragment_shader_source +=
              "}\n";

        }




      }
    }
  }
}





