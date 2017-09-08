/* Copyright (c) 2008-2017 the MRtrix3 contributors.
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


#include "progressbar.h"
#include "gui/mrview/tool/tractography/tractogram.h"
#include "gui/mrview/window.h"
#include "gui/projection.h"
#include "dwi/tractography/file.h"
#include "dwi/tractography/properties.h"
#include "dwi/tractography/scalar_file.h"
#include "gui/opengl/lighting.h"
#include "gui/mrview/mode/base.h"



const size_t MAX_BUFFER_SIZE = 2796200;  // number of points to fill 32MB

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {
        const int Tractogram::max_sample_stride;

        std::string Tractogram::Shader::vertex_shader_source (const Displayable& displayable)
        {
          const Tractogram& tractogram = dynamic_cast<const Tractogram&>(displayable);

          std::string source =
            "layout (location = 0) in vec3 vertex;\n"
            "layout (location = 1) in vec3 prev_vertex;\n"
            "layout (location = 2) in vec3 next_vertex;\n";

          if (color_type == TrackColourType::Ends)
            source += "layout (location = 3) in vec3 end_colour;\n";
          else if (color_type == TrackColourType::ScalarFile)
            source += "layout (location = 3) in float amp;\n";

          if (threshold_type == TrackThresholdType::SeparateFile)
            source += "layout (location = 4) in float thresh_amp;\n";

          source +=
          "uniform mat4 MVP;\n"
          "uniform float line_thickness;\n"

          // Uniforms won't be included in compiled shader if not referenced
          // so we can unconditionally list all of them
          "uniform vec3 screen_normal;\n"
          "uniform float crop_var;\n"
          "uniform float slab_width;\n"
          "uniform float offset, scale;\n"
          "uniform float scale_x, scale_y;\n"

          "out vec3 v_tangent;\n"
          "out vec2 v_end;\n";


          if (do_crop_to_slab)
            source += "out float v_include;\n";

          if (threshold_type != TrackThresholdType::None)
            source += "out float v_amp;\n";

          if (color_type == TrackColourType::Ends || color_type == TrackColourType::ScalarFile)
            source += "out vec3 v_colour;\n";

          // Main function
          source +=
            "void main() {\n"
            "  gl_Position = MVP * vec4(vertex, 1);\n"
            "  v_tangent = next_vertex - prev_vertex;\n"
            "  vec2 dir = mat3x2(MVP) * v_tangent;\n"
            "  v_end = line_thickness * normalize (vec2 (dir.y/scale_x, -dir.x/scale_y));\n"
            "  v_end.x *= scale_y; v_end.y *= scale_x;\n"
            ;

          if (do_crop_to_slab)
            source += "  v_include = (dot(vertex, screen_normal) - crop_var) / slab_width;\n";

          if (threshold_type == TrackThresholdType::UseColourFile)
            source += "  v_amp = amp;\n";
          else if (threshold_type == TrackThresholdType::SeparateFile)
            source += "  v_amp = thresh_amp;\n";

          if (color_type == TrackColourType::Ends)
            source += "  v_colour = end_colour;\n";
          else if (color_type == TrackColourType::ScalarFile) { // TODO: move to frag shader:
            if (!ColourMap::maps[colourmap].special) {
              source += "  float amplitude = clamp (";
              if (tractogram.scale_inverted()) source += "1.0 -";
              source += " scale * (amp - offset), 0.0, 1.0);\n";
            }
            source +=
                std::string("  vec3 color;\n  ") +
                ColourMap::maps[colourmap].glsl_mapping +
                "  v_colour = color;\n";
          }

          source += "}\n";

          return source;
        }


        std::string Tractogram::Shader::geometry_shader_source (const Displayable&)
        {
          std::string source =
          "layout(lines) in;\n"
          "layout(triangle_strip, max_vertices = 4) out;\n"
          "uniform float line_thickness;\n"
          "uniform float downscale_factor;\n"
          "uniform mat4 MV;\n"

          "in vec3 v_tangent[];\n"
          "in vec2 v_end[];\n";

          if (threshold_type != TrackThresholdType::None)
           source +=
            "in float v_amp[];\n"
            "out float g_amp;\n";

          if (do_crop_to_slab)
            source += "in float v_include[];\n"
              "out float g_include;\n";

          if (use_lighting || color_type == TrackColourType::Direction)
            source += "out vec3 g_tangent;\n";

          if (color_type == TrackColourType::ScalarFile || color_type == TrackColourType::Ends)
            source +=
              "in vec3 v_colour[];\n"
              "out vec3 fColour;\n";

          if (use_lighting)
           source +=
            "const float PI = " + str(Math::pi) + ";\n"
            "out float g_height;\n";

          source += "void main() {\n";

          if (do_crop_to_slab)
            source +=
              "  if (v_include[0] < 0.0 && v_include[1] < 0.0) return;\n"
              "  if (v_include[0] > 1.0 && v_include[1] > 1.0) return;\n";

          // First vertex:
          if (use_lighting || color_type == TrackColourType::Direction)
            source += "  g_tangent = v_tangent[0];\n";
          if (do_crop_to_slab)
            source += "  g_include = v_include[0];\n";
          if (threshold_type != TrackThresholdType::None)
            source += "  g_amp = v_amp[0];\n";
          if (color_type == TrackColourType::ScalarFile || color_type == TrackColourType::Ends)
            source += "  fColour = v_colour[0];\n";

          if (use_lighting)
            source += "  g_height = 0.0;\n";
          source +=
            "  gl_Position = gl_in[0].gl_Position - vec4(v_end[0],0,0);\n"
            "  EmitVertex();\n";

          if (use_lighting)
            source += "  g_height = PI;\n";
          source +=
            "  gl_Position = gl_in[0].gl_Position + vec4(v_end[0],0,0);\n"
            "  EmitVertex();\n";

          // Second vertex:
          if (use_lighting || color_type == TrackColourType::Direction)
            source += "  g_tangent = v_tangent[1];\n";
          if (do_crop_to_slab)
            source += "  g_include = v_include[1];\n";
          if (threshold_type != TrackThresholdType::None)
            source += "  g_amp = v_amp[1];\n";
          if (color_type == TrackColourType::ScalarFile || color_type == TrackColourType::Ends)
            source += "  fColour = v_colour[1];\n";

          if (use_lighting)
            source += "  g_height = 0.0;\n";
          source +=
            "  gl_Position = gl_in[1].gl_Position - vec4 (v_end[1],0,0);\n"
            "  EmitVertex();\n";

          if (use_lighting)
            source += "  g_height = PI;\n";
          source +=
            "  gl_Position = gl_in[1].gl_Position + vec4 (v_end[1],0,0);\n"
            "  EmitVertex();\n"
            "}\n";

          return source;
        }


        std::string Tractogram::Shader::fragment_shader_source (const Displayable& displayable)
        {
          const Tractogram& tractogram = dynamic_cast<const Tractogram&>(displayable);

          std::string source =
            "uniform float lower, upper;\n"
            "uniform vec3 const_colour;\n"
            "uniform mat4 MV;\n"
            "out vec3 colour;\n";

          if (color_type == TrackColourType::ScalarFile || color_type == TrackColourType::Ends)
            source += "in vec3 fColour;\n";
          if (use_lighting || color_type == TrackColourType::Direction)
            source += "in vec3 g_tangent;\n";

          if (threshold_type != TrackThresholdType::None)
            source += "in float g_amp;\n";

          if (use_lighting)
            source +=
              "uniform float ambient, diffuse, specular, shine;\n"
              "uniform vec3 light_pos;\n"
              "in float g_height;\n";
          if (do_crop_to_slab)
            source += "in float g_include;\n";

          source += "void main() {\n";

          if (do_crop_to_slab)
            source +=
              "  if (g_include < 0.0 || g_include > 1.0) discard;\n";

          if (threshold_type != TrackThresholdType::None) {
            if (tractogram.use_discard_lower())
              source += "  if (g_amp < lower) discard;\n";
            if (tractogram.use_discard_upper())
              source += "  if (g_amp > upper) discard;\n";
          }

          switch (color_type) {
            case TrackColourType::Direction:
              source += "  colour = abs (normalize (g_tangent));\n";
              break;
            case TrackColourType::ScalarFile:
              source += "  colour = fColour;\n";
              break;
            case TrackColourType::Ends:
              source += "  colour = fColour;\n";
              break;
            case TrackColourType::Manual:
              source += "  colour = const_colour;\n";
          }

          if (use_lighting)
            // g_height tells us where we are across the cylinder (0 - PI)
            source +=
              // compute surface normal:
              "  float s = sin (g_height);\n"
              "  float c = cos (g_height);\n"
              "  vec3 tangent = normalize (mat3(MV) * g_tangent);\n"
              "  vec3 in_plane_x = normalize (vec3(-tangent.y, tangent.x, 0.0f));\n"
              "  vec3 in_plane_y = normalize (vec3(-tangent.x, -tangent.y, 0.0f));\n"
              "  vec3 surface_normal = c*in_plane_x +  s*abs(tangent.z)*in_plane_y;\n"
              "  surface_normal.z -= s * sqrt(tangent.x*tangent.x + tangent.y*tangent.y);\n"

             "  float light_dot_surfaceN = -dot(light_pos, surface_normal);"
             // Ambient and diffuse component
             "  colour *= ambient + diffuse * clamp(light_dot_surfaceN, 0, 1);\n"

             // Specular component
             "  if (light_dot_surfaceN > 0.0) {\n"
             "    vec3 reflection = light_pos + 2 * light_dot_surfaceN * surface_normal;\n"
             "    colour += specular * pow(clamp(-reflection.z, 0, 1), shine);\n"
             "  }\n";

          source += "}\n";

          return source;
        }


        bool Tractogram::Shader::need_update (const Displayable& object) const
        {
          const Tractogram& tractogram (dynamic_cast<const Tractogram&> (object));
          if (do_crop_to_slab != tractogram.tractography_tool.crop_to_slab())
            return true;
          if (color_type != tractogram.color_type)
            return true;
          if (threshold_type != tractogram.threshold_type)
            return true;
          if (use_lighting != tractogram.tractography_tool.use_lighting)
            return true;

          return Displayable::Shader::need_update (object);
        }




        void Tractogram::Shader::update (const Displayable& object)
        {
          const Tractogram& tractogram (dynamic_cast<const Tractogram&> (object));
          do_crop_to_slab = tractogram.tractography_tool.crop_to_slab();
          use_lighting = tractogram.tractography_tool.use_lighting;
          color_type = tractogram.color_type;
          threshold_type = tractogram.threshold_type;
          Displayable::Shader::update (object);
        }







        Tractogram::Tractogram (Tractography& tool, const std::string& filename) :
            Displayable (filename),
            show_colour_bar (true),
            original_fov (NAN),
            intensity_scalar_filename (std::string()),
            threshold_scalar_filename (std::string()),
            tractography_tool (tool),
            filename (filename),
            color_type (TrackColourType::Direction),
            threshold_type (TrackThresholdType::None),
            sample_stride (0),
            vao_dirty (true),
            threshold_min (NaN),
            threshold_max (NaN)
        {
          set_allowed_features (true, true, true);
          colourmap = 1;
          connect (&window(), SIGNAL (fieldOfViewChanged()), this, SLOT (on_FOV_changed()));
          on_FOV_changed ();
        }





        Tractogram::~Tractogram ()
        {
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;
          if (vertex_buffers.size())
            gl::DeleteBuffers (vertex_buffers.size(), &vertex_buffers[0]);
          if (vertex_array_objects.size())
            gl::DeleteVertexArrays (vertex_array_objects.size(), &vertex_array_objects[0]);
          if (colour_buffers.size())
            gl::DeleteBuffers (colour_buffers.size(), &colour_buffers[0]);
          if (intensity_scalar_buffers.size())
            gl::DeleteBuffers (intensity_scalar_buffers.size(), &intensity_scalar_buffers[0]);
          if (threshold_scalar_buffers.size())
            gl::DeleteBuffers (threshold_scalar_buffers.size(), &threshold_scalar_buffers[0]);
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;
        }




        void Tractogram::render (const Projection& transform)
        {
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;
          if (tractography_tool.do_crop_to_slab && tractography_tool.slab_thickness <= 0.0)
            return;

          start (track_shader);
          transform.set (track_shader);

          if (tractography_tool.do_crop_to_slab) {
            gl::Uniform3fv (gl::GetUniformLocation (track_shader, "screen_normal"), 1, transform.screen_normal().data());
            gl::Uniform1f (gl::GetUniformLocation (track_shader, "crop_var"),
                window().focus().dot(transform.screen_normal()) - tractography_tool.slab_thickness / 2);
            gl::Uniform1f (gl::GetUniformLocation (track_shader, "slab_width"),
                tractography_tool.slab_thickness);
          }

          if (threshold_type != TrackThresholdType::None) {
            if (use_discard_lower())
              gl::Uniform1f (gl::GetUniformLocation (track_shader, "lower"), lessthan);
            if (use_discard_upper())
              gl::Uniform1f (gl::GetUniformLocation (track_shader, "upper"), greaterthan);
          }

          if (color_type == TrackColourType::Manual)
            gl::Uniform3fv (gl::GetUniformLocation (track_shader, "const_colour"), 1, colour.data());

          if (color_type == TrackColourType::ScalarFile) {
            gl::Uniform1f (gl::GetUniformLocation (track_shader, "offset"), display_midpoint - 0.5f * display_range);
            gl::Uniform1f (gl::GetUniformLocation (track_shader, "scale"), 1.0 / display_range);
          }

          if (tractography_tool.use_lighting) {
            gl::UniformMatrix4fv (gl::GetUniformLocation (track_shader, "MV"), 1, gl::FALSE_, transform.modelview());
            gl::Uniform3fv (gl::GetUniformLocation (track_shader, "light_pos"), 1, tractography_tool.lighting->lightpos);
            gl::Uniform1f (gl::GetUniformLocation (track_shader, "ambient"), tractography_tool.lighting->ambient);
            gl::Uniform1f (gl::GetUniformLocation (track_shader, "diffuse"), tractography_tool.lighting->diffuse);
            gl::Uniform1f (gl::GetUniformLocation (track_shader, "specular"), tractography_tool.lighting->specular);
            gl::Uniform1f (gl::GetUniformLocation (track_shader, "shine"), tractography_tool.lighting->shine);
          }

          if (!std::isfinite (original_fov)) {
            // set line thickness once upon loading, but don't touch it after that:
            // it shouldn't change when the background image changes
            default_type dim[] = {
              window().image()->header().size (0) * window().image()->header().spacing (0),
              window().image()->header().size (1) * window().image()->header().spacing (1),
              window().image()->header().size (2) * window().image()->header().spacing (2)
            };
            original_fov = std::pow (dim[0]*dim[1]*dim[2], 1.0f/3.0f);
          }

          line_thickness_screenspace = tractography_tool.line_thickness*original_fov*(transform.width()+transform.height()) / ( 2.0*window().FOV()*transform.width()*transform.height());

          gl::Uniform1f (gl::GetUniformLocation (track_shader, "line_thickness"), line_thickness_screenspace);
          gl::Uniform1f (gl::GetUniformLocation (track_shader, "scale_x"), transform.width());
          gl::Uniform1f (gl::GetUniformLocation (track_shader, "scale_y"), transform.height());

          if (tractography_tool.line_opacity < 1.0) {
            gl::Enable (gl::BLEND);
            gl::BlendEquation (gl::FUNC_ADD);
            gl::BlendFunc (gl::CONSTANT_ALPHA, gl::ONE);
            gl::Disable (gl::DEPTH_TEST);
            gl::DepthMask (gl::TRUE_);
            gl::BlendColor (1.0, 1.0, 1.0,  tractography_tool.line_opacity / 0.5);
            render_streamlines();
            gl::BlendFunc (gl::CONSTANT_ALPHA, gl::ONE_MINUS_CONSTANT_ALPHA);
            gl::Enable (gl::DEPTH_TEST);
            gl::DepthMask (gl::TRUE_);
            gl::BlendColor (1.0, 1.0, 1.0, tractography_tool.line_opacity / 0.5);
            render_streamlines();

          } else {
            gl::Disable (gl::BLEND);
            gl::Enable (gl::DEPTH_TEST);
            gl::DepthMask (gl::TRUE_);
            render_streamlines();
          }

          if (tractography_tool.line_opacity < 1.0) {
            gl::Disable (gl::BLEND);
            gl::Enable (gl::DEPTH_TEST);
            gl::DepthMask (gl::TRUE_);
          }

          stop (track_shader);
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;
        }




        inline void Tractogram::render_streamlines ()
        {
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;
          for (size_t buf = 0, N = vertex_buffers.size(); buf < N; ++buf) {
            gl::BindVertexArray (vertex_array_objects[buf]);

            if (should_update_stride)
              update_stride();

            if (vao_dirty) {

              switch (color_type) {
                case TrackColourType::Ends:
                  gl::BindBuffer (gl::ARRAY_BUFFER, colour_buffers[buf]);
                  gl::EnableVertexAttribArray (3);
                  gl::VertexAttribPointer (3, 3, gl::FLOAT, gl::FALSE_, 3 * sample_stride * sizeof(float), (void*)0);
                  break;
                case TrackColourType::ScalarFile:
                  gl::BindBuffer (gl::ARRAY_BUFFER, intensity_scalar_buffers[buf]);
                  gl::EnableVertexAttribArray (3);
                  gl::VertexAttribPointer (3, 1, gl::FLOAT, gl::FALSE_, sample_stride * sizeof(float), (void*)0);
                  break;
                default:
                  break;
              }

              if (threshold_type == TrackThresholdType::SeparateFile) {
                gl::BindBuffer (gl::ARRAY_BUFFER, threshold_scalar_buffers[buf]);
                gl::EnableVertexAttribArray (4);
                gl::VertexAttribPointer (4, 1, gl::FLOAT, gl::FALSE_, sample_stride * sizeof(float), (void*)0);
              }

              gl::BindBuffer (gl::ARRAY_BUFFER, vertex_buffers[buf]);
              gl::EnableVertexAttribArray (0);
              gl::VertexAttribPointer (0, 3, gl::FLOAT, gl::FALSE_, 3*sample_stride*sizeof(float), (void*)(3*sample_stride*sizeof(float)));
              gl::EnableVertexAttribArray (1);
              gl::VertexAttribPointer (1, 3, gl::FLOAT, gl::FALSE_, 3*sample_stride*sizeof(float), (void*)0);
              gl::EnableVertexAttribArray (2);
              gl::VertexAttribPointer (2, 3, gl::FLOAT, gl::FALSE_, 3*sample_stride*sizeof(float), (void*)(6*sample_stride*sizeof(float)));

              for(size_t j = 0, M = track_sizes[buf].size(); j < M; ++j) {
                track_sizes[buf][j] = (GLint) std::floor (original_track_sizes[buf][j] / (float)sample_stride);
                track_starts[buf][j] = (GLint) (std::ceil (original_track_starts[buf][j] / (float)sample_stride)) - 1;
              }
            }
            gl::MultiDrawArrays (gl::LINE_STRIP, &track_starts[buf][0], &track_sizes[buf][0], num_tracks_per_buffer[buf]);
          }

          vao_dirty = false;
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;
        }




        inline void Tractogram::update_stride ()
        {
          const float step_size = DWI::Tractography::get_step_size (properties);
          GLint new_stride = 1;
          if (std::isfinite (step_size)) {
            new_stride = GLint (tractography_tool.line_thickness * original_fov / step_size);
            new_stride = std::max (1, std::min (max_sample_stride, new_stride));
          }

          if (new_stride != sample_stride) {
            sample_stride = new_stride;
            vao_dirty = true;
          }

          should_update_stride = false;
        }



        void Tractogram::load_tracks()
        {
          // Make sure to set graphics context!
          // We're setting up vertex array objects
          MRView::GrabContext context;
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;

          DWI::Tractography::Reader<float> file (filename, properties);
          DWI::Tractography::Streamline<float> tck;
          vector<Eigen::Vector3f> buffer;
          vector<GLint> starts;
          vector<GLint> sizes;
          size_t tck_count = 0;

          on_FOV_changed();

          while (file (tck)) {

            size_t N = tck.size();
            if(!N) continue;

            // Pre padding
            // To support downsampling, we want to ensure that the starting track vertex
            // is used even when we're using a stride > 1
            for (size_t i = 0; i < max_sample_stride; ++i)
              buffer.push_back (tck.front());

            starts.push_back (buffer.size());

            buffer.insert (buffer.end(), tck.begin(), tck.end());

            // Post padding
            // Similarly, to support downsampling, we also want to ensure the final track vertex
            // will be used even we're using a stride > 1
            for (size_t i = 0; i < max_sample_stride; ++i)
              buffer.push_back(tck.back());

            sizes.push_back (N);
            tck_count++;
            if (buffer.size() >= MAX_BUFFER_SIZE)
              load_tracks_onto_GPU (buffer, starts, sizes, tck_count);

            endpoint_tangents.push_back ((tck.back() - tck.front()).normalized());
          }
          if (buffer.size()) {
            load_tracks_onto_GPU (buffer, starts, sizes, tck_count);
          }
          file.close();
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;
        }




        void Tractogram::load_end_colours()
        {
          // These data are now retained in memory - no need to re-scan track file
          if (colour_buffers.size())
            return;

          // Make sure to set graphics context!
          // We're setting up vertex array objects
          MRView::GrabContext context;
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;

          erase_colour_data();
          size_t total_tck_counter = 0;
          for (size_t buffer_index = 0, N = vertex_buffers.size(); buffer_index < N; ++buffer_index) {

            const size_t num_tracks = num_tracks_per_buffer[buffer_index];
            vector<Eigen::Vector3f> buffer;
            for (size_t buffer_tck_counter = 0; buffer_tck_counter != num_tracks; ++buffer_tck_counter) {

              const Eigen::Vector3f& tangent (endpoint_tangents[total_tck_counter++]);
              const Eigen::Vector3f colour (std::abs (tangent[0]), std::abs (tangent[1]), std::abs (tangent[2]));
              const size_t tck_length = original_track_sizes[buffer_index][buffer_tck_counter];

              // Includes pre- and post-padding to coincide with tracks buffer
              for (size_t i = 0; i != tck_length + (2 * max_sample_stride); ++i)
                buffer.push_back (colour);

            }
            load_end_colours_onto_GPU (buffer);
          }
          // Don't need this now that we've initialised the GPU buffers
          endpoint_tangents.clear();
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;
        }





        void Tractogram::load_intensity_track_scalars (const std::string& filename)
        {
          // Make sure to set graphics context!
          // We're setting up vertex array objects
          MRView::GrabContext context;
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;

          erase_intensity_scalar_data ();
          value_min = std::numeric_limits<float>::infinity();
          value_max = -std::numeric_limits<float>::infinity();
          vector<float> buffer;
          vector<float> tck_scalar;

          if (Path::has_suffix (filename, ".tsf")) {
            DWI::Tractography::Properties scalar_properties;
            DWI::Tractography::ScalarReader<float> file (filename, scalar_properties);
            DWI::Tractography::check_properties_match (properties, scalar_properties, ".tck / .tsf");
            while (file (tck_scalar)) {

              size_t tck_size = tck_scalar.size();

              if(!tck_size)
                continue;

              // Pre padding to coincide with tracks buffer
              for (size_t i = 0; i < max_sample_stride; ++i)
                buffer.push_back (tck_scalar.front());

              for (size_t i = 0; i < tck_size; ++i) {
                buffer.push_back (tck_scalar[i]);
                value_max = std::max (value_max, tck_scalar[i]);
                value_min = std::min (value_min, tck_scalar[i]);
              }

              // Post padding to coincide with tracks buffer
              for (size_t i = 0; i < max_sample_stride; ++i)
                buffer.push_back (tck_scalar.back());

              if (buffer.size() >= MAX_BUFFER_SIZE)
                load_intensity_scalars_onto_GPU (buffer);
            }
            if (buffer.size())
              load_intensity_scalars_onto_GPU (buffer);
            file.close();
          } else {
            const Eigen::VectorXf scalars = MR::load_vector<float> (filename);
            size_t total_num_tracks = 0;
            for (vector<size_t>::const_iterator i = num_tracks_per_buffer.begin(); i != num_tracks_per_buffer.end(); ++i)
              total_num_tracks += *i;
            if (size_t(scalars.size()) != total_num_tracks)
              throw Exception ("The scalar text file does not contain the same number of elements as the selected tractogram");
            size_t running_index = 0;

            for (size_t buffer_index = 0; buffer_index != vertex_buffers.size(); ++buffer_index) {
              const size_t num_tracks = num_tracks_per_buffer[buffer_index];
              vector<GLint>& track_lengths (original_track_sizes[buffer_index]);

              for (size_t index = 0; index != num_tracks; ++index, ++running_index) {
                const float value = scalars[running_index];
                tck_scalar.assign (track_lengths[index], value);

                // Pre padding to coincide with tracks buffer
                for (size_t i = 0; i < max_sample_stride; ++i)
                  buffer.push_back (tck_scalar.front());

                buffer.insert (buffer.end(), tck_scalar.begin(), tck_scalar.end());

                // Post padding to coincide with tracks buffer
                for (size_t i = 0; i < max_sample_stride; ++i)
                  buffer.push_back (tck_scalar.back());

                value_max = std::max (value_max, value);
                value_min = std::min (value_min, value);
              }

              load_intensity_scalars_onto_GPU (buffer);
            }
          }
          intensity_scalar_filename = filename;
          this->set_windowing (value_min, value_max);
          if (!std::isfinite (greaterthan))
            greaterthan = value_max;
          if (!std::isfinite (lessthan))
            lessthan = value_min;
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;
        }



        void Tractogram::load_threshold_track_scalars (const std::string& filename)
        {
          // Make sure to set graphics context!
          // We're setting up vertex array objects
          MRView::GrabContext context;
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;

          erase_threshold_scalar_data ();
          threshold_min = std::numeric_limits<float>::infinity();
          threshold_max = -std::numeric_limits<float>::infinity();
          vector<float> buffer;
          vector<float> tck_scalar;

          if (Path::has_suffix (filename, ".tsf")) {
            DWI::Tractography::Properties scalar_properties;
            DWI::Tractography::ScalarReader<float> file (filename, scalar_properties);
            DWI::Tractography::check_properties_match (properties, scalar_properties, ".tck / .tsf");
            while (file (tck_scalar)) {

              size_t tck_size = tck_scalar.size();

              if(!tck_size)
                continue;

              // Pre padding to coincide with tracks buffer
              for (size_t i = 0; i < max_sample_stride; ++i)
                buffer.push_back (tck_scalar.front());

              for (size_t i = 0; i < tck_size; ++i) {
                buffer.push_back (tck_scalar[i]);
                threshold_max = std::max (threshold_max, tck_scalar[i]);
                threshold_min = std::min (threshold_min, tck_scalar[i]);
              }

              // Post padding to coincide with tracks buffer
              for (size_t i = 0; i < max_sample_stride; ++i)
                buffer.push_back (tck_scalar.back());

              if (buffer.size() >= MAX_BUFFER_SIZE)
                load_threshold_scalars_onto_GPU (buffer);
            }
            if (buffer.size())
              load_threshold_scalars_onto_GPU (buffer);
            file.close();
          } else {
            const Eigen::VectorXf scalars = MR::load_vector<float> (filename);
            size_t total_num_tracks = 0;
            for (vector<size_t>::const_iterator i = num_tracks_per_buffer.begin(); i != num_tracks_per_buffer.end(); ++i)
              total_num_tracks += *i;
            if (size_t(scalars.size()) != total_num_tracks)
              throw Exception ("The scalar text file does not contain the same number of elements as the selected tractogram");
            size_t running_index = 0;

            for (size_t buffer_index = 0; buffer_index != vertex_buffers.size(); ++buffer_index) {
              const size_t num_tracks = num_tracks_per_buffer[buffer_index];
              vector<GLint>& track_lengths (original_track_sizes[buffer_index]);

              for (size_t index = 0; index != num_tracks; ++index, ++running_index) {
                const float value = scalars[running_index];
                tck_scalar.assign (track_lengths[index], value);

                // Pre padding to coincide with tracks buffer
                for (size_t i = 0; i < max_sample_stride; ++i)
                  buffer.push_back (tck_scalar.front());

                buffer.insert (buffer.end(), tck_scalar.begin(), tck_scalar.end());

                // Post padding to coincide with tracks buffer
                for (size_t i = 0; i < max_sample_stride; ++i)
                  buffer.push_back (tck_scalar.back());

                threshold_max = std::max (threshold_max, value);
                threshold_min = std::min (threshold_min, value);
              }

              load_threshold_scalars_onto_GPU (buffer);
            }
          }
          threshold_scalar_filename = filename;
          greaterthan = threshold_max;
          lessthan = threshold_min;

          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;
        }



        void Tractogram::erase_colour_data()
        {
          MRView::GrabContext context;
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;
          if (colour_buffers.size()) {
            gl::DeleteBuffers (colour_buffers.size(), &colour_buffers[0]);
            colour_buffers.clear();
          }
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;
        }




        void Tractogram::erase_intensity_scalar_data ()
        {
          MRView::GrabContext context;
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;
          if (intensity_scalar_buffers.size()) {
            gl::DeleteBuffers (intensity_scalar_buffers.size(), &intensity_scalar_buffers[0]);
            intensity_scalar_buffers.clear();
          }
          intensity_scalar_filename.clear();
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;
        }




        void Tractogram::erase_threshold_scalar_data ()
        {
          MRView::GrabContext context;
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;
          if (threshold_scalar_buffers.size()) {
            gl::DeleteBuffers (threshold_scalar_buffers.size(), &threshold_scalar_buffers[0]);
            threshold_scalar_buffers.clear();
          }
          threshold_scalar_filename.clear();
          threshold_min = threshold_max = NaN;
          set_use_discard_lower (false);
          set_use_discard_upper (false);
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;
        }



        void Tractogram::set_color_type (const TrackColourType c)
        {
          if ((color_type == TrackColourType::Ends && c == TrackColourType::ScalarFile)
              || (color_type == TrackColourType::ScalarFile && c == TrackColourType::Ends))
            vao_dirty = true;
          color_type = c;
        }

        void Tractogram::set_threshold_type (const TrackThresholdType t)
        {
          threshold_type = t;
          switch (threshold_type) {
            case TrackThresholdType::None:
              threshold_min = threshold_max = NaN;
              break;
            case TrackThresholdType::UseColourFile:
              threshold_min = value_min;
              threshold_max = value_max;
              break;
            case TrackThresholdType::SeparateFile:
              break;
          }
        }



        void Tractogram::load_tracks_onto_GPU (vector<Eigen::Vector3f>& buffer,
            vector<GLint>& starts,
            vector<GLint>& sizes,
            size_t& tck_count)
        {
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;

          GLuint vertex_array_object;
          gl::GenVertexArrays (1, &vertex_array_object);
          gl::BindVertexArray (vertex_array_object);

          GLuint vertexbuffer;
          gl::GenBuffers (1, &vertexbuffer);
          gl::BindBuffer (gl::ARRAY_BUFFER, vertexbuffer);
          gl::BufferData (gl::ARRAY_BUFFER, buffer.size() * sizeof(Eigen::Vector3f), &buffer[0][0], gl::STATIC_DRAW);

          vertex_array_objects.push_back (vertex_array_object);
          vertex_buffers.push_back (vertexbuffer);
          track_starts.push_back (starts);
          track_sizes.push_back (sizes);
          original_track_starts.push_back (starts);
          original_track_sizes.push_back (sizes);
          num_tracks_per_buffer.push_back (tck_count);

          buffer.clear();
          starts.clear();
          sizes.clear();
          tck_count = 0;
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;
        }

        void Tractogram::load_end_colours_onto_GPU (vector<Eigen::Vector3f>& buffer)
        {
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;

          GLuint vertexbuffer;
          gl::GenBuffers (1, &vertexbuffer);
          gl::BindBuffer (gl::ARRAY_BUFFER, vertexbuffer);
          gl::BufferData (gl::ARRAY_BUFFER, buffer.size() * sizeof(Eigen::Vector3f), &buffer[0][0], gl::STATIC_DRAW);

          vao_dirty = true;

          colour_buffers.push_back (vertexbuffer);
          buffer.clear();
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;
        }





        void Tractogram::load_intensity_scalars_onto_GPU (vector<float>& buffer)
        {
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;

          GLuint vertexbuffer;
          gl::GenBuffers (1, &vertexbuffer);
          gl::BindBuffer (gl::ARRAY_BUFFER, vertexbuffer);
          gl::BufferData (gl::ARRAY_BUFFER, buffer.size() * sizeof(float), &buffer[0], gl::STATIC_DRAW);

          vao_dirty = true;

          intensity_scalar_buffers.push_back (vertexbuffer);
          buffer.clear();

          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;
        }




        void Tractogram::load_threshold_scalars_onto_GPU (vector<float>& buffer)
        {
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;

          GLuint vertexbuffer;
          gl::GenBuffers (1, &vertexbuffer);
          gl::BindBuffer (gl::ARRAY_BUFFER, vertexbuffer);
          gl::BufferData (gl::ARRAY_BUFFER, buffer.size() * sizeof(float), &buffer[0], gl::STATIC_DRAW);

          vao_dirty = true;

          threshold_scalar_buffers.push_back (vertexbuffer);
          buffer.clear();

          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;
        }


      }
    }
  }
}


