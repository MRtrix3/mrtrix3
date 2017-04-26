/* Copyright (c) 2008-2017 the MRtrix3 contributors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
 */


#include "gui/mrview/window.h"
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
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;
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
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;
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

          if (use_lighting) {

            fragment_shader_source +=
              "uniform float ambient, diffuse, specular, shine;\n"
              "uniform vec3 light_pos;\n";

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

          if (use_lighting) {
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

          if (geometry == edge_geometry_t::LINE) {
            vertex_shader_source +=
              "layout (location = 1) in vec3 vertexTangent_modelspace;\n";
            if (use_lighting) {
              vertex_shader_source +=
              "out vec3 tangent" + GS_in + ";\n";
            }
          }

          if (geometry == edge_geometry_t::CYLINDER) {
            vertex_shader_source +=
              "layout (location = 1) in vec3 vertexNormal_modelspace;\n"
              "uniform vec3 centre_one, centre_two;\n"
              "uniform mat3 rot_matrix;\n"
              "uniform float radius;\n";
            if (use_lighting) {
              vertex_shader_source +=
              "out vec3 normal" + GS_in + ";\n";
            }
          }

          if (geometry == edge_geometry_t::STREAMLINE) {
            vertex_shader_source +=
              "layout (location = 1) in vec3 vertexTangent_modelspace;\n";
            if (use_lighting) {
              vertex_shader_source +=
              "out vec3 tangent" + GS_in + ";\n";
            }
          }

          if (geometry == edge_geometry_t::STREAMTUBE) {
            vertex_shader_source +=
              "layout (location = 1) in vec3 vertexTangent_modelspace;\n"
              "layout (location = 2) in vec3 vertexNormal_modelspace;\n"
              "uniform float radius;\n";
            if (use_lighting) {
              vertex_shader_source +=
              "out vec3 normal" + GS_in + ";\n";
            }
          }

          if (colour == edge_colour_t::DIRECTION && (geometry == edge_geometry_t::STREAMLINE || geometry == edge_geometry_t::STREAMTUBE)) {
            vertex_shader_source +=
              "out vec3 dir" + GS_in + ";\n";
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
              if (use_lighting) {
              vertex_shader_source +=
              "  tangent" + GS_in + " = normalize (mat3(MV) * vertexTangent_modelspace);\n";
              }
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
              "  vec3 pos = centre + (radius * offset);\n";
              if (use_lighting) {
              vertex_shader_source +=
              "  normal" + GS_in + " = normalize (mat3(MV) * (vertexNormal_modelspace * rot_matrix));\n";
              }
              break;
            case edge_geometry_t::STREAMLINE:
              vertex_shader_source +=
              "  vec3 pos = vertexPosition_modelspace;\n";
              if (use_lighting) {
              vertex_shader_source +=
              "  tangent" + GS_in + " = normalize (mat3(MV) * vertexTangent_modelspace);\n";
              }
              if (colour == edge_colour_t::DIRECTION) {
              vertex_shader_source +=
              "  dir" + GS_in + " = vertexTangent_modelspace;\n";
              }
              break;
            case edge_geometry_t::STREAMTUBE:
              vertex_shader_source +=
              "  vec3 pos = vertexPosition_modelspace + (radius * vertexNormal_modelspace);\n";
              if (use_lighting) {
              vertex_shader_source +=
              "  normal" + GS_in + " = normalize (mat3(MV) * vertexNormal_modelspace);\n";
              }
              if (colour == edge_colour_t::DIRECTION) {
              vertex_shader_source +=
              "  dir" + GS_in + " = vertexTangent_modelspace;\n";
              }
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
                if (use_lighting) {
                geometry_shader_source +=
                "in vec3 tangent" + GS_in + "[2];\n"
                "out vec3 tangent" + GS_out + ";\n";
                }
                break;
              case edge_geometry_t::CYLINDER:
              case edge_geometry_t::STREAMTUBE:
                geometry_shader_source +=
                "layout(triangles) in;\n"
                "layout(line_strip, max_vertices=2) out;\n"
                "in float depth[3];\n";
                if (use_lighting) {
                geometry_shader_source +=
                "in vec3 normal" + GS_in + "[3];\n"
                "out vec3 normal" + GS_out + ";\n";
                }
                break;
            }

            if (colour == edge_colour_t::DIRECTION && (geometry == edge_geometry_t::STREAMLINE || geometry == edge_geometry_t::STREAMTUBE)) {
              geometry_shader_source +=
                "in vec3 dir" + GS_in + "[3];\n"
                "out vec3 dir" + GS_out + ";\n";
            }

            geometry_shader_source +=
                "void main() {\n";

            switch (geometry) {
              case edge_geometry_t::LINE:
              case edge_geometry_t::STREAMLINE:
                geometry_shader_source +=
                "  float mu = depth[0] / (depth[0] - depth[1]);\n"
                "  if (mu >= 0.0 && mu <= 1.0) {\n"
                "    gl_Position = gl_in[0].gl_Position + (mu * (gl_in[1].gl_Position - gl_in[0].gl_Position));\n";
                if (use_lighting) {
                geometry_shader_source +=
                "    tangent" + GS_out + " = normalize(((1.0 - mu) * tangent" + GS_in + "[1]) + (mu * tangent" + GS_in + "[0]));\n";
                }
                if (colour == edge_colour_t::DIRECTION && geometry == edge_geometry_t::STREAMLINE) {
                geometry_shader_source +=
                "      dir" + GS_out + " = normalize(((1.0 - mu) * dir" + GS_in + "[v1]) + (mu * dir" + GS_in + "[v2]));\n";
                }
                geometry_shader_source +=
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
                "      gl_Position = gl_in[v1].gl_Position + (mu * (gl_in[v2].gl_Position - gl_in[v1].gl_Position));\n";
                if (use_lighting) {
                geometry_shader_source +=
                "      normal" + GS_out + " = normalize(((1.0 - mu) * normal" + GS_in + "[v1]) + (mu * normal" + GS_in + "[v2]));\n";
                }
                if (colour == edge_colour_t::DIRECTION && geometry == edge_geometry_t::STREAMTUBE) {
                geometry_shader_source +=
                "      dir" + GS_out + " = normalize(((1.0 - mu) * dir" + GS_in + "[v1]) + (mu * dir" + GS_in + "[v2]));\n";
                }
                geometry_shader_source +=
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

          if (use_lighting) {
            fragment_shader_source +=
              "uniform float ambient, diffuse, specular, shine;\n"
              "uniform vec3 light_pos;\n";
            if (geometry == edge_geometry_t::CYLINDER || geometry == edge_geometry_t::STREAMTUBE) {
              fragment_shader_source +=
              "in vec3 normal" + GS_out + ";\n";
            } else { // Line and streamline
              fragment_shader_source +=
              "in vec3 tangent" + GS_out + ";\n";
            }
          }
          if (colour == edge_colour_t::DIRECTION && (geometry == edge_geometry_t::STREAMLINE || geometry == edge_geometry_t::STREAMTUBE)) {
            fragment_shader_source +=
              "in vec3 dir" + GS_out + ";\n";
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
              "  color.rgb = vec3 (abs(dir" + GS_out + "[0]), abs(dir" + GS_out + "[1]), abs(dir" + GS_out + "[2]));\n";
            } else {
              fragment_shader_source +=
              "  color = vec3 (abs(dir" + GS_out + "[0]), abs(dir" + GS_out + "[1]), abs(dir" + GS_out + "[2]));\n";
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

          if (use_lighting) {
            if (geometry == edge_geometry_t::CYLINDER || geometry == edge_geometry_t::STREAMTUBE) {
              fragment_shader_source +=
              "  color *= ambient + diffuse * clamp (dot (normal" + GS_out + ", light_pos), 0, 1);\n"
              "  color += specular * pow (clamp (dot (reflect (-light_pos, normal" + GS_out + "), vec3(0.0,0.0,1.0)), 0, 1), shine);\n";
            } else { // Line and streamline
              fragment_shader_source +=
              "  vec3 t = normalize (tangent" + GS_out + ");\n"
              "  float l_dot_t = dot(light_pos, t);\n"
              "  vec3 l_perp = light_pos - l_dot_t * t;\n"
              "  vec3 l_perp_norm = normalize (l_perp);\n"
              "  float n_dot_t = t.z;\n"
              "  vec3 n_perp = vec3(0.0, 0.0, 1.0) - n_dot_t * t;\n"
              "  vec3 n_perp_norm = normalize (n_perp);\n"
              "  float cos2_theta = 0.5+0.5*dot(l_perp_norm,n_perp_norm);\n"
              "  color *= ambient + diffuse * length(l_perp) * cos2_theta;\n"
              "  color += specular * sqrt(cos2_theta) * pow (clamp (-l_dot_t*n_dot_t + length(l_perp)*length(n_perp), 0, 1), shine);\n";
            }
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





