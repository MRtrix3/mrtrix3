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



namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {

        std::string SphericalROIs::Shader::vertex_shader_source (const Displayable&)
        {
          std::string source =
            "layout (location = 0) in vec3 centre;\n"
            "layout (location = 1) in float radius;\n"
            "layout (location = 2) in vec3 colour;\n"

            "uniform mat4 MV, MVP;\n"
            "uniform vec3 screen_normal;\n"
            "uniform float focus_distance;\n"
            "uniform float opacity;\n"
            "out float v_radius;\n"
            "out vec4 v_colour;\n"

            "void main() {\n"
            "  gl_Position = MVP * vec4(centre, 1);\n"
            "  float dist_to_slice = dot(centre, screen_normal) - focus_distance;\n"
            "  if (dist_to_slice < radius)\n"
            "    v_radius = sqrt(radius*radius - dist_to_slice*dist_to_slice);\n"
            "  else\n"
            "    v_radius = 0.0;\n"

            "  v_colour = vec4(colour, opacity);\n"

            "}\n";

          return source;
        }


        std::string SphericalROIs::Shader::geometry_shader_source (const Displayable&)
        {
          std::string source =
            "layout(points) in;\n"
            "layout(triangle_strip, max_vertices = 4) out;\n"

            "uniform float scale_x, scale_y;\n"

            "in float v_radius[];\n"
            "in vec4 v_colour[];\n"
            "out vec2 g_pos;\n"
            "out vec4 g_colour;\n"

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
          std::string source =
            "uniform float ss_lower;\n"

            "in vec2 g_pos;\n"
            "in vec4 g_colour;\n"
            "out vec4 f_colour;\n"

            "void main() {\n"

            // Turn this square into a circle
            "  float dist = sqrt(dot (g_pos, g_pos));\n"
            "  if (dist > 1.0)\n"
            "    discard;\n"

            "  f_colour = g_colour;\n"

            "}\n";

          return source;
        }


        bool SphericalROIs::Shader::need_update (const Displayable& object) const
        {
          const SphericalROIs& spherical_rois (dynamic_cast<const SphericalROIs&> (object));
          if (do_crop_to_slab != spherical_rois.tractography_tool.crop_to_slab())
            return true;
          if (use_lighting != spherical_rois.tractography_tool.use_lighting)
            return true;
          return Displayable::Shader::need_update (object);
        }




        void SphericalROIs::Shader::update (const Displayable& object)
        {
          const SphericalROIs& spherical_rois (dynamic_cast<const SphericalROIs&> (object));
          do_crop_to_slab = spherical_rois.tractography_tool.crop_to_slab();
          use_lighting = spherical_rois.tractography_tool.use_lighting;
          Displayable::Shader::update (object);
        }







        SphericalROIs::SphericalROIs (const std::string& filename,
                                      const GUI::MRView::Tool::Tractography& tool) :
            Displayable (filename),
            tractography_tool (tool),
            vao_dirty (true)
        {
          set_allowed_features (false, true, true);
        }





        SphericalROIs::~SphericalROIs ()
        {
          clear();
        }




        void SphericalROIs::load (const DWI::Tractography::Properties& properties)
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

              //CONF option: MRViewSphericalMaskColour
              //CONF default: 1,1,0 (yellow)
              //CONF The colour with which to draw tractography masks that are
              //CONF defined using spherical coordinates.

              //CONF option: MRViewSphericalIncludeColour
              //CONF default: 0,1,0 (green)
              //CONF The colour with which to draw tractography inclusion regions
              //CONF that are defined using spherical coordinates.

              //CONF option: MRViewSphericalExcludeColour
              //CONF default: 1,0,0 (ref)
              //CONF The colour with which to draw tractography exclusion regions
              //CONF that are defined using spherical coordinates.

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

              if (roi.first == "mask") {
                colours.push_back (get_colour ("MRViewSphericalMaskColour", Eigen::Vector3f (1,1,0)));
              } else if (roi.first == "include") {
                colours.push_back (get_colour ("MRViewSphericalIncludeColour", Eigen::Vector3f (0,1,0)));
              } else if (roi.first == "exclude") {
                colours.push_back (get_colour ("MRViewSphericalExcludeColour", Eigen::Vector3f (1,0,0)));
              } else {
                throw Exception ("Error reading ROI specification \"" + roi.first + ": " + roi.second + "\": Unknown ROI type");
              }
              centres.push_back (Eigen::Vector3f (values[0], values[1], values[2]));
              radii.push_back (values[3]);
              data.push_back (SphereSpec (centres.back(), radii.back()));
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
          vertex_array_object.bind();

          gl::Uniform3fv (gl::GetUniformLocation (shader, "screen_normal"), 1, transform.screen_normal().data());
          gl::Uniform1f (gl::GetUniformLocation (shader, "focus_distance"), Window::main->focus().dot (transform.screen_normal()));
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

          stop (shader);
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;
        }




      }
    }
  }
}


