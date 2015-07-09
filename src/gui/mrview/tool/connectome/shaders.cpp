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
          if (crop_to_slab != parent.crop_to_slab) return true;
          if (is_3D != parent.is_3D) return true;
          if (use_lighting != parent.use_lighting()) return true;
          if (geometry != parent.node_geometry) return true;
          if (colour != parent.node_colour) return true;
          if ((colour == node_colour_t::VECTOR_FILE || colour == node_colour_t::MATRIX_FILE) && colourmap_index != parent.node_colourmap_index) return true;
          const bool need_alpha = parent.use_alpha_nodes();
          if (use_alpha != need_alpha) return true;
          return false;
        }

        void NodeShader::update (const Connectome& parent)
        {
          crop_to_slab = parent.crop_to_slab;
          is_3D = parent.is_3D;
          use_lighting = parent.use_lighting();
          geometry = parent.node_geometry;
          assert (geometry != node_geometry_t::OVERLAY);
          colour = parent.node_colour;
          colourmap_index = parent.node_colourmap_index;
          use_alpha = parent.use_alpha_nodes();

          const std::string GS_in  = is_3D ? "" : "_GSin";
          const std::string GS_out = is_3D ? "" : "_GSout";

          vertex_shader_source =
              "layout (location = 0) in vec3 vertexPosition_modelspace;\n";

          if (geometry == node_geometry_t::CUBE || geometry == node_geometry_t::MESH) {
            vertex_shader_source +=
              "layout (location = 1) in vec3 vertexNormal_modelspace;\n";
          }

          vertex_shader_source +=
              "uniform mat4 MVP;\n"
              "uniform mat4 MV;\n"
              "uniform vec3 node_centre;\n"
              "uniform float node_size;\n";

          if (geometry == node_geometry_t::SPHERE || geometry == node_geometry_t::CUBE || geometry == node_geometry_t::MESH) {
            vertex_shader_source +=
              "out vec3 normal" + GS_in + ";\n";
          }

          if (crop_to_slab) {
            vertex_shader_source +=
              "uniform vec3 screen_normal;\n";
            if (is_3D) {
              vertex_shader_source +=
              "out float include;\n"
              "uniform float crop_var;\n"
              "uniform float slab_thickness;\n";
            } else {
              vertex_shader_source +=
              "out float depth;\n"
              "uniform float depth_offset;\n";
            }
          }

          vertex_shader_source +=
              "void main() {\n";

          switch (geometry) {
            case node_geometry_t::SPHERE:
              vertex_shader_source +=
              "  vec3 pos = node_centre + (vertexPosition_modelspace * node_size);\n"
              "  normal" + GS_in + " = normalize (mat3(MV) * vertexPosition_modelspace);\n";
              break;
            case node_geometry_t::CUBE:
              vertex_shader_source +=
              "  vec3 pos = node_centre + (vertexPosition_modelspace * node_size);\n"
              "  normal" + GS_in + " = normalize (mat3(MV) * vertexNormal_modelspace);\n";
              break;
            case node_geometry_t::POINT:
              vertex_shader_source +=
              "  vec3 pos = node_centre;\n"
              "  gl_PointSize = node_size;\n";
              break;
            case node_geometry_t::OVERLAY:
              break;
            case node_geometry_t::MESH:
              vertex_shader_source +=
              "  vec3 pos = node_centre + (node_size * (vertexPosition_modelspace - node_centre));"
              "  normal" + GS_in + " = normalize (mat3(MV) * vertexNormal_modelspace);\n";
              break;
          }

          vertex_shader_source +=
              "  gl_Position = MVP * vec4 (pos, 1);\n";

          if (crop_to_slab) {
            if (is_3D) {
              vertex_shader_source +=
              "  include = (dot (pos, screen_normal) - crop_var) / slab_thickness;\n";
            } else {
              vertex_shader_source +=
              "  depth = dot (pos, screen_normal) - depth_offset;\n";
            }
          }

          vertex_shader_source +=
              "}\n";

          // =================================================================

          geometry_shader_source = std::string("");
          if (!is_3D) {

            if (geometry == node_geometry_t::POINT) {

              geometry_shader_source +=
                  "layout(points) in;\n"
                  "layout(points, max_vertices=1) out;\n"
                  "in float depth[1];\n"
                  "void main() {\n"
                  "  float depth = abs(depth[0]);\n"
                  "  float radius = gl_in[0].gl_PointSize;\n"
                  "  if (depth < radius) {\n"
                  // Calculate the radius of the sphere subtended by the plane
                  // FIXME This is wrong: radius is in pixels, depth is in mm
                  //"    gl_PointSize = sqrt(radius*radius - depth*depth);\n"
                  // Resort to not modifying radii of points
                  "    gl_PointSize = radius;\n"
                  "    gl_Position = gl_in[0].gl_Position;\n"
                  "    gl_Position.z = 0.0;\n"
                  "    EmitVertex();\n"
                  "    EndPrimitive();\n"
                  "  }\n"
                  "}\n";

            } else {

              geometry_shader_source +=
                  "layout(triangles) in;\n"
                  "layout(line_strip, max_vertices=2) out;\n"
                  "in float depth[3];\n";

              if (geometry == node_geometry_t::SPHERE || geometry == node_geometry_t::CUBE || geometry == node_geometry_t::MESH) {
                geometry_shader_source +=
                  "in vec3 normal" + GS_in + "[3];\n"
                  "out vec3 normal" + GS_out + ";\n";
              }

              // Need to detect whether or not this triangle intersects the viewing plane
              // If it does, need to emit two vertices; one for each of the two interpolated
              //   points that intersect the viewing plane
              // Following MVP multiplication, the viewing plane should be at a Z-coordinate of zero...
              // If one edge intersects the viewing plane, it is guaranteed that a second does also
              geometry_shader_source +=
                  "void main() {\n"
                  "  for (int v1 = 0; v1 != 3; ++v1) {\n"
                  "    int v2 = (v1 == 2) ? 0 : v1+1;\n"
                  "    float mu = depth[v1] / (depth[v1] - depth[v2]);\n"
                  "    if (mu >= 0.0 && mu <= 1.0) {\n"
                  "      gl_Position = gl_in[v1].gl_Position + (mu * (gl_in[v2].gl_Position - gl_in[v1].gl_Position));\n"
                  "      normal" + GS_out + " = normalize(((1.0 - mu) * normal" + GS_in + "[v1]) + (mu * normal" + GS_in + "[v2]));\n"
                  "      EmitVertex();\n"
                  "    }\n"
                  "  }\n"
                  "  EndPrimitive();\n"
                  "}\n";

            }

          }

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

          if (use_lighting && geometry != node_geometry_t::POINT) {

            fragment_shader_source +=
              "uniform float ambient, diffuse, specular, shine;\n"
              "uniform vec3 light_pos;\n"
              "uniform vec3 screen_normal;\n";

            if (geometry == node_geometry_t::SPHERE || geometry == node_geometry_t::CUBE || geometry == node_geometry_t::MESH) {
              fragment_shader_source +=
                "in vec3 normal" + GS_out + ";\n";
            }
          }

          if (crop_to_slab && is_3D) {
            fragment_shader_source +=
              "in float include;\n";
          }

          fragment_shader_source +=
              "void main() {\n";

          if (crop_to_slab && is_3D) {
            fragment_shader_source +=
              "  if (include < 0 || include > 1) discard;\n";
          }

          if (use_alpha) {
            fragment_shader_source +=
                "  color.rgb = node_colour;\n";
          } else {
            fragment_shader_source +=
                "  color = node_colour;\n";
          }

          if (use_lighting && geometry != node_geometry_t::POINT) {
            fragment_shader_source +=
              "  color *= ambient + diffuse * clamp (dot (normal" + GS_out + ", light_pos), 0, 1);\n"
              "  color += specular * pow (clamp (dot (reflect (-light_pos, normal" + GS_out + "), vec3(0.0,0.0,1.0)), 0, 1), shine);\n";
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
          if (crop_to_slab != parent.crop_to_slab) return true;
          if (is_3D != parent.is_3D) return true;
          if (use_lighting != parent.use_lighting()) return true;
          if (geometry != parent.edge_geometry) return true;
          if (colour != parent.edge_colour) return true;
          if (colour == edge_colour_t::MATRIX_FILE && colourmap_index != parent.edge_colourmap_index) return true;
          const bool need_alpha = parent.use_alpha_edges();
          if (use_alpha != need_alpha) return true;
          return false;
        }

        void EdgeShader::update (const Connectome& parent)
        {
          crop_to_slab = parent.crop_to_slab;
          is_3D = parent.is_3D;
          use_lighting = parent.use_lighting();
          geometry = parent.edge_geometry;
          colour = parent.edge_colour;
          colourmap_index = parent.edge_colourmap_index;
          use_alpha = parent.use_alpha_edges();

          const std::string GS_in  = is_3D ? "" : "_GSin";
          const std::string GS_out = is_3D ? "" : "_GSout";

          vertex_shader_source =
              "layout (location = 0) in vec3 vertexPosition_modelspace;\n"
              "uniform mat4 MVP;\n"
              "uniform mat4 MV;\n";

          if (geometry == edge_geometry_t::CYLINDER) {
            vertex_shader_source +=
              "layout (location = 1) in vec3 vertexNormal_modelspace;\n"
              "uniform vec3 centre_one, centre_two;\n"
              "uniform mat3 rot_matrix;\n"
              "uniform float radius;\n"
              "out vec3 normal" + GS_in + ";\n";
          }

          if (geometry == edge_geometry_t::STREAMLINE) {
            vertex_shader_source +=
              "layout (location = 1) in vec3 vertexTangent_modelspace;\n"
              "out vec3 tangent" + GS_in + ";\n";
          }

          if (geometry == edge_geometry_t::STREAMTUBE) {
            vertex_shader_source +=
              "layout (location = 1) in vec3 vertexTangent_modelspace;\n"
              "layout (location = 2) in vec3 vertexNormal_modelspace;\n"
              "uniform float radius;\n"
              "out vec3 tangent" + GS_in + ";\n"
              "out vec3 normal" + GS_in + ";\n";
          }

          if (crop_to_slab) {
            vertex_shader_source +=
              "uniform vec3 screen_normal;\n";
            if (is_3D) {
              vertex_shader_source +=
              "out float include;\n"
              "uniform float crop_var;\n"
              "uniform float slab_thickness;\n";
            } else {
              vertex_shader_source +=
              "out float depth;\n"
              "uniform float depth_offset;\n";
            }
          }

          vertex_shader_source +=
              "void main() {\n";

          switch (geometry) {
            case edge_geometry_t::LINE:
              vertex_shader_source +=
              "  vec3 pos = vertexPosition_modelspace;\n";
              break;
            case edge_geometry_t::CYLINDER:
              vertex_shader_source +=
              "  vec3 centre = centre_one;\n"
              "  vec3 offset = vertexPosition_modelspace;\n"
              "  if (offset[2] != 0.0) {\n"
              "    centre = centre_two;\n"
              "    offset[2] = 0.0;\n"
              "  }\n"
              "  offset = offset * rot_matrix;\n"
              "  normal" + GS_in + " = normalize (mat3(MV) * (vertexNormal_modelspace * rot_matrix));\n"
              "  vec3 pos = centre + (radius * offset);\n";
              break;
            case edge_geometry_t::STREAMLINE:
              vertex_shader_source +=
              "  vec3 pos = vertexPosition_modelspace;\n"
              "  tangent" + GS_in + " = vertexTangent_modelspace;\n";
              break;
            case edge_geometry_t::STREAMTUBE:
              vertex_shader_source +=
              "  vec3 pos = vertexPosition_modelspace + (radius * vertexNormal_modelspace);\n"
              "  tangent" + GS_in + " = vertexTangent_modelspace;\n"
              "  normal" + GS_in + " = normalize (mat3(MV) * vertexNormal_modelspace);\n";
              break;
          }

          vertex_shader_source +=
              "  gl_Position = MVP * vec4 (pos, 1);\n";

          if (crop_to_slab) {
            if (is_3D) {
              vertex_shader_source +=
              "  include = (dot (pos, screen_normal) - crop_var) / slab_thickness;\n";
            } else {
              vertex_shader_source +=
              "  depth = dot (pos, screen_normal) - depth_offset;\n";
            }
          }

          vertex_shader_source +=
              "}\n";

          // =================================================================

          geometry_shader_source = std::string("");
          if (!is_3D) {

            switch (geometry) {
              case edge_geometry_t::LINE:
              case edge_geometry_t::STREAMLINE:
                geometry_shader_source +=
                "layout(lines) in;\n"
                "layout(points, max_vertices=1) out;\n"
                "in float depth[2];\n";
                break;
              case edge_geometry_t::CYLINDER:
              case edge_geometry_t::STREAMTUBE:
                geometry_shader_source +=
                "layout(triangles) in;\n"
                "layout(line_strip, max_vertices=2) out;\n"
                "in vec3 normal" + GS_in + "[3];\n"
                "in vec3 tangent" + GS_in + "[3];\n"
                "out vec3 normal" + GS_out + ";\n"
                "out vec3 tangent" + GS_out + ";\n"
                "in float depth[3];\n";
                break;
            }

            geometry_shader_source +=
                "void main() {\n";

            switch (geometry) {
              case edge_geometry_t::LINE:
              case edge_geometry_t::STREAMLINE:
                geometry_shader_source +=
                "  float mu = depth[0] / (depth[0] - depth[1]);\n"
                "  if (mu >= 0.0 && mu <= 1.0) {\n"
                "    gl_Position = gl_in[0].gl_Position + (mu * (gl_in[1].gl_Position - gl_in[0].gl_Position));\n"
                "    EmitVertex();\n"
                "  }\n"
                "  EndPrimitive();\n";
                break;
              case edge_geometry_t::CYLINDER:
              case edge_geometry_t::STREAMTUBE:
                geometry_shader_source +=
                "  for (int v1 = 0; v1 != 3; ++v1) {\n"
                "    int v2 = (v1 == 2) ? 0 : v1+1;\n"
                "    float mu = depth[v1] / (depth[v1] - depth[v2]);\n"
                "    if (mu >= 0.0 && mu <= 1.0) {\n"
                "      gl_Position = gl_in[v1].gl_Position + (mu * (gl_in[v2].gl_Position - gl_in[v1].gl_Position));\n"
                "      normal" + GS_out + " = normalize(((1.0 - mu) * normal" + GS_in + "[v1]) + (mu * normal" + GS_in + "[v2]));\n"
                "      tangent" + GS_out + " = normalize(((1.0 - mu) * tangent" + GS_in + "[v1]) + (mu * tangent" + GS_in + "[v2]));\n"
                "      EmitVertex();\n"
                "    }\n"
                "  }\n"
                "  EndPrimitive();\n";
                break;
            }

            geometry_shader_source +=
                "}\n";

          }

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

          if (use_lighting && (geometry == edge_geometry_t::CYLINDER || geometry == edge_geometry_t::STREAMTUBE)) {
            fragment_shader_source +=
              "in vec3 normal" + GS_out + ";\n"
              "uniform float ambient, diffuse, specular, shine;\n"
              "uniform vec3 light_pos;\n"
              "uniform vec3 screen_normal;\n";
          }
          if (geometry == edge_geometry_t::STREAMLINE || geometry == edge_geometry_t::STREAMTUBE) {
            fragment_shader_source +=
              "in vec3 tangent" + GS_out + ";\n";
          }

          if (crop_to_slab && is_3D) {
            fragment_shader_source +=
              "in float include;\n";
          }

          fragment_shader_source +=
              "void main() {\n";

          if (crop_to_slab && is_3D) {
            fragment_shader_source +=
              "  if (include < 0 || include > 1) discard;\n";
          }

          if (colour == edge_colour_t::DIRECTION && (geometry == edge_geometry_t::STREAMLINE || geometry == edge_geometry_t::STREAMTUBE)) {

            if (use_alpha) {
              fragment_shader_source +=
              "  color.rgb = vec3 (abs(tangent" + GS_out + "[0]), abs(tangent" + GS_out + "[1]), abs(tangent" + GS_out + "[2]));\n";
            } else {
              fragment_shader_source +=
              "  color = vec3 (abs(tangent" + GS_out + "[0]), abs(tangent" + GS_out + "[1]), abs(tangent" + GS_out + "[2]));\n";
            }

          } else {

            if (use_alpha) {
              fragment_shader_source +=
              "  color.rgb = edge_colour;\n";
            } else {
              fragment_shader_source +=
              "  color = edge_colour;\n";
            }

          }

          if (use_lighting && (geometry == edge_geometry_t::CYLINDER || geometry == edge_geometry_t::STREAMTUBE)) {
            fragment_shader_source +=
              "  color *= ambient + diffuse * clamp (dot (normal" + GS_out + ", light_pos), 0, 1);\n"
              "  color += specular * pow (clamp (dot (reflect (-light_pos, normal" + GS_out + "), vec3(0.0,0.0,1.0)), 0, 1), shine);\n";
          }

          // TODO Lighting for streamline rendering

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





