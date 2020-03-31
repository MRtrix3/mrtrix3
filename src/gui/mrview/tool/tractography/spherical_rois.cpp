/* Copyright (c) 2008-2019 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Covered Software is provided under this License on an "as is"
 * basis, without warranty of any kind, either expressed, implied, or
 * statutory, including, without limitation, warranties that the
 * Covered Software is free of defects, merchantable, fit for a
 * particular purpose or non-infringing.
 * See the Mozilla Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
 */

#include "gui/mrview/tool/tractography/spherical_rois.h"

#include "gui/mrview/tool/tractography/tractography.h"
#include "gui/opengl/lighting.h"


namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {



        void SphericalROIs::Shared::initialise()
        {
          if (sphere.num_indices)
            return;
          sphere.LOD (5);
          vao.gen();
          vao.bind();
          sphere.vertex_buffer.bind (gl::ARRAY_BUFFER);
          gl::EnableVertexAttribArray (0);
          gl::VertexAttribPointer (0, 3, gl::FLOAT, gl::FALSE_, 0, (void*)(0));

          //CONF option: MRViewSphericalSeedColour
          //CONF default: 0,1,1 (cyan)
          //CONF The colour with which to draw tractography seeds that are
          //CONF defined using spherical coordinates.

          //CONF option: MRViewSphericalIncludeColour
          //CONF default: 0,1,0 (green)
          //CONF The colour with which to draw tractography inclusion regions
          //CONF that are defined using spherical coordinates.

          //CONF option: MRViewSphericalExcludeColour
          //CONF default: 1,0,0 (ref)
          //CONF The colour with which to draw tractography exclusion regions
          //CONF that are defined using spherical coordinates.

          //CONF option: MRViewSphericalMaskColour
          //CONF default: 1,1,0 (yellow)
          //CONF The colour with which to draw tractography masks that are
          //CONF defined using spherical coordinates.

          auto get_colour = [] (const std::string config_key, const Eigen::Vector3f& default_value) -> Eigen::Vector3f
          {
            const std::string config_str = File::Config::get (config_key);
            if (!config_str.size())
              return default_value;
            try {
              const auto config_values = parse_floats (config_str);
              if (config_values.size() != 3)
                throw Exception ("Config file entry for key \"" + config_key + "\" must contain three floating-point numbers");
              return Eigen::Vector3f (float(config_values[0]), float(config_values[1]), float(config_values[2]));
            } catch (Exception& e) {
              e.display();
              WARN ("Error reading key \"" + config_key + "\" from config file; using default");
              return default_value;
            }
          };

          type2colour.resize(5);
          type2colour[type_t::SEED]    = get_colour ("MRViewSphericalSeedColour",    Eigen::Vector3f (0,1,1));
          type2colour[type_t::INCLUDE] = get_colour ("MRViewSphericalIncludeColour", Eigen::Vector3f (0,1,0));
          type2colour[type_t::EXCLUDE] = get_colour ("MRViewSphericalExcludeColour", Eigen::Vector3f (1,0,0));
          type2colour[type_t::MASK]    = get_colour ("MRViewSphericalMaskColour",    Eigen::Vector3f (1,1,0));

        }






        std::string SphericalROIs::Shader::vertex_shader_source (const Displayable&)
        {
          std::string source;
          if (is_3D) {

            source =
              "layout (location = 0) in vec3 vertexPosition_modelspace;\n"

              "uniform mat4 MVP;\n"
              "uniform vec3 roi_centre;\n"
              "uniform float radius;\n";

            if (use_lighting) {
              source +=
              "uniform mat4 MV;\n"
              "out vec3 v_normal;\n";
            }

            source +=
              "void main() {\n";

            if (use_lighting) {
              source +=
              "  v_normal = normalize (mat3(MV) * vertexPosition_modelspace);\n";
            }

            source +=
              "  vec3 pos = roi_centre + (vertexPosition_modelspace * radius);\n"
              "  gl_Position = MVP * vec4 (pos, 1);\n"
              "}\n";

          } else { // 2D

            source =
              "layout (location = 0) in vec3 centre;\n"
              "layout (location = 1) in float radius;\n"
              "layout (location = 2) in vec3 colour;\n"

              "uniform mat4 MV, MVP;\n"
              "uniform vec3 screen_normal;\n"
              "uniform float focus_distance;\n";

            if (use_transparency) {
              source +=
              "uniform float opacity;\n"
              "out vec4 v_colour;\n";
            } else {
              source +=
              "out vec3 v_colour;\n";
            }
            source +=
              "out float v_radius;\n"

              "void main() {\n"
              "  gl_Position = MVP * vec4(centre, 1);\n"
              "  float dist_to_slice = dot(centre, screen_normal) - focus_distance;\n"
              "  if (dist_to_slice < radius)\n"
              "    v_radius = sqrt(radius*radius - dist_to_slice*dist_to_slice);\n"
              "  else\n"
              "    v_radius = 0.0;\n";

            if (use_transparency) {
              source +=
              "  v_colour = vec4(colour, opacity);\n";
            } else {
              source +=
              "  v_colour = colour;\n";
            }

            source +=
              "}\n";

          }

          return source;
        }


        std::string SphericalROIs::Shader::geometry_shader_source (const Displayable&)
        {
          // No geometry shader required when operating in 3D mode
          if (is_3D)
            return std::string();

          std::string source =
              "layout(points) in;\n"
              "layout(triangle_strip, max_vertices = 4) out;\n"

              "uniform float scale_x, scale_y;\n"

              "in float v_radius[];\n";
            if (use_transparency) {
              source +=
              "in vec4 v_colour[];\n"
              "out vec4 g_colour;\n";
            } else {
              source +=
              "in vec3 v_colour[];\n"
              "out vec3 g_colour;\n";
            }

            source +=
              "out vec2 g_pos;\n"

              "void generate (float x, float y) {\n"
              "  g_pos = vec2(x, y);\n"
              "  gl_Position = gl_in[0].gl_Position + vec4 (x * v_radius[0] / scale_x, y * v_radius[0] / scale_y, 0.0, 0.0);\n"
              "  EmitVertex();\n"
              "}\n"

              "void main() {\n"

              "  if (v_radius[0] == 0.0)\n"
              "    return;\n"
              "  g_colour = v_colour[0];\n"

              "  generate (-1.0, -1.0);\n"
              "  generate (+1.0, -1.0);\n"
              "  generate (-1.0, +1.0);\n"
              "  generate (+1.0, +1.0);\n"

              "}\n";

          return source;
        }


        std::string SphericalROIs::Shader::fragment_shader_source (const Displayable&)
        {
          std::string source;
          if (is_3D) {

            source +=
              "uniform vec3 colour;\n";

            if (use_transparency) {
              source +=
              "uniform float opacity;\n";
            }

            if (use_lighting) {
              source +=
              "in vec3 v_normal;\n"
              "uniform float ambient, diffuse, specular, shine;\n"
              "uniform vec3 light_pos;\n";
            }

            if (use_transparency) {
              source +=
              "out vec4 f_colour;\n";
            } else {
              source +=
              "out vec3 f_colour;\n";
            }

            source +=
              "void main() {\n";

            if (use_transparency) {
              source +=
              "  f_colour.rgb = colour;\n";
            } else {
              source +=
              "  f_colour = colour;\n";
            }



            if (use_lighting) {
              source +=
              //"  f_colour *= ambient + diffuse * clamp (dot (v_normal, light_pos), 0, 1);\n"
              //"  f_colour += specular * pow (clamp (abs (dot (reflect (-light_pos, v_normal), vec3(0.0,0.0,1.0))), 0, 1), shine);\n";


              "  float light_dot_surfaceN = -dot(light_pos, v_normal);"
              // Ambient and diffuse component
              "  f_colour *= ambient + diffuse * clamp(light_dot_surfaceN, 0, 1);\n"
              // Specular component
              "  if (light_dot_surfaceN > 0.0) {\n"
              "    vec3 reflection = light_pos + 2 * light_dot_surfaceN * v_normal;\n"
              "    f_colour += specular * pow(clamp(-reflection.z, 0, 1), shine);\n"
              "  }\n";

            }

            if (use_transparency) {
              source +=
              "  f_colour.a = opacity;\n";
            }

            source +=
              "}\n";

          } else { // 2D

            source +=
              "uniform float ss_lower;\n"

              "in vec2 g_pos;\n";

            if (use_transparency) {
              source +=
              "in vec4 g_colour;\n"
              "out vec4 f_colour;\n";
            } else {
              source +=
              "in vec3 g_colour;\n"
              "out vec3 f_colour;\n";
            }

            source +=
              "void main() {\n"

              // Turn this square into a circle
              "  float dist = sqrt(dot (g_pos, g_pos));\n"
              "  if (dist > 1.0)\n"
              "    discard;\n"

              "  f_colour = g_colour;\n"

              "}\n";

          }

          return source;
        }




        bool SphericalROIs::Shader::need_update (const Displayable& object) const
        {
          const SphericalROIs& spherical_rois (dynamic_cast<const SphericalROIs&> (object));
          if (is_3D != spherical_rois.tractography_tool.is_3D)
            return true;
          if (use_lighting != spherical_rois.tractography_tool.use_lighting)
            return true;
          if (use_transparency == bool(spherical_rois.tractography_tool.spherical_roi_opacity == 1.0f))
            return true;
          return Displayable::Shader::need_update (object);
        }



        void SphericalROIs::Shader::update (const Displayable& object)
        {
          const SphericalROIs& spherical_rois (dynamic_cast<const SphericalROIs&> (object));
          is_3D = spherical_rois.tractography_tool.is_3D;
          use_lighting = spherical_rois.tractography_tool.use_lighting;
          use_transparency = (spherical_rois.tractography_tool.spherical_roi_opacity != 1.0);
          Displayable::Shader::update (object);
        }







        SphericalROIs::SphericalROIs (const std::string& filename,
                                      const GUI::MRView::Tool::Tractography& tool) :
            Displayable (filename),
            tractography_tool (tool),
            shared (tool.spherical_roi_shared),
            vao_dirty (true)
        {
          set_allowed_features (false, true, true);
        }



        SphericalROIs::~SphericalROIs ()
        {
          clear();
        }




        void SphericalROIs::load (const MR::DWI::Tractography::Properties& properties)
        {
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;
          clear();

          vector<Eigen::Vector3f> centres, colours;
          vector<float> radii;
          for (const auto& roi : properties.prior_rois) {
            try {
              vector<default_type> values;
              // May be an image rather than a sphere specification;
              //   proceed without warning if that's the case
              try {
                values = parse_floats (roi.second);
              } catch (...) {
                continue;
              }
              if (values.size() != 4)
                throw Exception ("Error reading ROI specification \"" + roi.first + ": " + roi.second + "\": Should contain 4 floating-point values");

              type_t type;
              if (roi.first == "mask") {
                type = type_t::MASK;
              } else if (roi.first == "include") {
                type = type_t::INCLUDE;
              } else if (roi.first == "exclude") {
                type = type_t::EXCLUDE;
              } else {
                throw Exception ("Error reading ROI specification \"" + roi.first + ": " + roi.second + "\": Unknown ROI type");
              }
              colours.push_back (shared.type2colour[type]);
              centres.push_back (Eigen::Vector3f (values[0], values[1], values[2]));
              radii.push_back (values[3]);
              data.push_back (SphereSpec (type, centres.back(), values[3]));
            } catch (Exception& e) {
              e.display();
            }
          }
          // TODO Try to extract any spherical seeds as well

          assert (radii.size() == centres.size());
          assert (colours.size() == centres.size());

          vertex_buffer.gen();
          radii_buffer.gen();
          colour_buffer.gen();
          vertex_array_object.gen();
          vertex_array_object.bind();
          vertex_buffer.bind (gl::ARRAY_BUFFER);
          gl::BufferData (gl::ARRAY_BUFFER, centres.size()*sizeof(Eigen::Vector3f), centres.data(), gl::STATIC_DRAW);
          gl::EnableVertexAttribArray (0);
          gl::VertexAttribPointer (0, 3, gl::FLOAT, gl::FALSE_, 3 * sizeof(float), (void*)0);
          radii_buffer.bind (gl::ARRAY_BUFFER);
          gl::BufferData (gl::ARRAY_BUFFER, radii.size()*sizeof(float), radii.data(), gl::STATIC_DRAW);
          gl::EnableVertexAttribArray (1);
          gl::VertexAttribPointer (1, 1, gl::FLOAT, gl::FALSE_, sizeof(float), (void*)0);
          colour_buffer.bind (gl::ARRAY_BUFFER);
          gl::BufferData (gl::ARRAY_BUFFER, colours.size()*sizeof(Eigen::Vector3f), colours.data(), gl::STATIC_DRAW);
          gl::EnableVertexAttribArray (2);
          gl::VertexAttribPointer (2, 3, gl::FLOAT, gl::FALSE_, 3 * sizeof(float), (void*)0);

          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;
        }



        void SphericalROIs::clear()
        {
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;
          data.clear();
          vertex_buffer.clear();
          colour_buffer.clear();
          radii_buffer.clear();
          vertex_array_object.clear();
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;
        }




        void SphericalROIs::render (const Projection& transform)
        {
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;

          start (shader);
          transform.set (shader);

          if (tractography_tool.is_3D) {

            shared.sphere.vertex_buffer.bind (gl::ARRAY_BUFFER);
            shared.vao.bind();
            shared.sphere.index_buffer.bind();

            gl::Enable (gl::DEPTH_TEST);
            if (tractography_tool.spherical_roi_opacity == 1.0f) {
              gl::Disable (gl::BLEND);
              gl::DepthMask (gl::TRUE_);
            } else {
              gl::Enable (gl::BLEND);
              gl::DepthMask (gl::FALSE_);
              gl::BlendEquation (gl::FUNC_ADD);
              gl::BlendFunc (gl::CONSTANT_ALPHA, gl::ONE_MINUS_CONSTANT_ALPHA);
              gl::BlendColor (1.0, 1.0, 1.0, tractography_tool.spherical_roi_opacity);
              gl::Uniform1f (gl::GetUniformLocation (shader, "opacity"), tractography_tool.spherical_roi_opacity);
            }
            GL_CHECK_ERROR;

            GLint specular_ID = 0;
            if (tractography_tool.use_lighting) {
              gl::UniformMatrix4fv (gl::GetUniformLocation (shader, "MV"), 1, gl::FALSE_, transform.modelview());
              gl::Uniform3fv (gl::GetUniformLocation (shader, "light_pos"), 1, tractography_tool.lighting->lightpos);
              gl::Uniform1f  (gl::GetUniformLocation (shader, "ambient"),      tractography_tool.lighting->ambient);
              gl::Uniform1f  (gl::GetUniformLocation (shader, "diffuse"),      tractography_tool.lighting->diffuse);
              specular_ID = gl::GetUniformLocation (shader, "specular");
              gl::Uniform1f  (specular_ID,                                     tractography_tool.lighting->specular);
              gl::Uniform1f  (gl::GetUniformLocation (shader, "shine"),        tractography_tool.lighting->shine);
            }
            GL_CHECK_ERROR;

            const GLint roi_centre_ID = gl::GetUniformLocation (shader, "roi_centre");
            const GLint radius_ID     = gl::GetUniformLocation (shader, "radius");
            const GLint colour_ID     = gl::GetUniformLocation (shader, "colour");

            for (const auto& roi : data) {
              gl::Uniform3fv (roi_centre_ID, 1, roi.centre.data());
              gl::Uniform1f  (radius_ID,        roi.radius);
              gl::Uniform3fv (colour_ID,     1, shared.type2colour[roi.type].data());

              if (tractography_tool.spherical_roi_opacity != 1.0f) {
                //gl::CullFace (gl::FRONT);
                if (tractography_tool.use_lighting)
                  gl::Uniform1f (specular_ID, (1.0f - tractography_tool.spherical_roi_opacity) * tractography_tool.lighting->specular);
                gl::DrawElements (gl::TRIANGLES, shared.sphere.num_indices, gl::UNSIGNED_INT, (void*)0);
                //gl::CullFace (gl::BACK);
                if (tractography_tool.use_lighting)
                  gl::Uniform1f (specular_ID, tractography_tool.lighting->specular);
              }
              gl::DrawElements (gl::TRIANGLES, shared.sphere.num_indices, gl::UNSIGNED_INT, (void*)0);
              GL_CHECK_ERROR;
            }

            // Reset to defaults if we've been doing transparency
            if (tractography_tool.spherical_roi_opacity != 1.0f) {
              gl::Disable (gl::BLEND);
              gl::DepthMask (gl::TRUE_);
            }

          } else {

            vertex_array_object.bind();

            gl::Uniform3fv (gl::GetUniformLocation (shader, "screen_normal"), 1, transform.screen_normal().data());
            gl::Uniform1f (gl::GetUniformLocation (shader, "focus_distance"), Window::main->focus().dot (transform.screen_normal()));
            if (tractography_tool.spherical_roi_opacity != 1.0f)
              gl::Uniform1f (gl::GetUniformLocation (shader, "opacity"), tractography_tool.spherical_roi_opacity);

            // Scale factors to convert realspace radius to a fraction of the screen width in X and Y
            // These quantities are in fact the realspace length in mm spanned by the width / height of the screen
            const float scale_x = Eigen::Vector3f (transform.modelview_projection_inverse() * GL::vec4 (1.0, 0.0, 0.0, 0.0)).norm();
            const float scale_y = Eigen::Vector3f (transform.modelview_projection_inverse() * GL::vec4 (0.0, 1.0, 0.0, 0.0)).norm();
            gl::Uniform1f (gl::GetUniformLocation (shader, "scale_x"), scale_x);
            gl::Uniform1f (gl::GetUniformLocation (shader, "scale_y"), scale_y);
            GL_CHECK_ERROR;

            gl::Disable (gl::DEPTH_TEST);
            gl::DepthMask (gl::FALSE_);
            if (tractography_tool.spherical_roi_opacity < 1.0) {
              gl::Enable (gl::BLEND);
              gl::Disable (gl::DEPTH_TEST);
              gl::BlendFunc (gl::SRC_ALPHA, gl::ONE_MINUS_SRC_ALPHA);
              gl::BlendEquation (gl::FUNC_ADD);
            } else {
              gl::Disable (gl::BLEND);
            }
            GL_CHECK_ERROR;

            gl::DrawArrays (gl::POINTS, 0, data.size());
            GL_CHECK_ERROR;

            // restore OpenGL environment:
            gl::Disable (gl::BLEND);
            gl::Enable (gl::DEPTH_TEST);
            gl::DepthMask (gl::TRUE_);

          }

          stop (shader);
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;
        }




      }
    }
  }
}


