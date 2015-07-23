/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier & David Raffelt 17/12/12

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

#include "progressbar.h"
#include "image/stride.h"
#include "gui/mrview/tool/tractography/tractogram.h"
#include "gui/mrview/window.h"
#include "gui/projection.h"
#include "dwi/tractography/file.h"
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

          if (color_type == Ends)
            source += "layout (location = 3) in vec3 end_colour;\n";
          else if (color_type == ScalarFile)
            source += "layout (location = 3) in float amp;\n";

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

          if (color_type == ScalarFile)
            source += "out float v_amp;\n";
          else if (color_type == Ends)
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

          if (color_type == Ends)
            source += "  v_colour = end_colour;\n";
          else if (color_type == ScalarFile) { // TODO: move to frag shader:
              source += "  v_amp = amp;\n";
              if (!ColourMap::maps[colourmap].special) {
                source += "   float amplitude = clamp (";
                if (tractogram.scale_inverted()) source += "1.0 -";
                source += " scale * (amp - offset), 0.0, 1.0);\n";
              }
              if (!scalarfile_by_direction)
                source +=
                  std::string("  vec3 color;\n") +
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

          if (color_type == ScalarFile)
           source +=
            "in float v_amp[];\n" // does this need to be declared flat? I don't see how this could be interpolated anyway...
            "out float g_amp;\n";
          if (do_crop_to_slab)
            source += "in float v_include[];\n"
              "out float g_include;\n";

          if (use_lighting || color_type == Direction)
            source += "out vec3 g_tangent;\n";

          if (color_type == ScalarFile || color_type == Ends)
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
          if (use_lighting || color_type == Direction)
            source += "  g_tangent = v_tangent[0];\n";
          if (do_crop_to_slab)
            source += "  g_include = v_include[0];\n";
          if (color_type == ScalarFile)
            source += "  g_amp = v_amp[0];\n";
          if (color_type == ScalarFile || color_type == Ends)
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
          if (use_lighting || color_type == Direction)
            source += "  g_tangent = v_tangent[1];\n";
          if (do_crop_to_slab)
            source += "  g_include = v_include[1];\n";
          if (color_type == ScalarFile)
            source += "  g_amp = v_amp[1];\n";
          if (color_type == ScalarFile || color_type == Ends)
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

          if (color_type == ScalarFile || color_type == Ends)
            source += "in vec3 fColour;\n";
          if (use_lighting || color_type == Direction)
            source += "in vec3 g_tangent;\n";

          if (color_type == ScalarFile)
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

          switch (color_type) {
            case Direction:
              source += "  colour = abs (normalize (g_tangent));\n";
              break;
            case ScalarFile:
              if (tractogram.use_discard_lower())
                source += "  if (g_amp < lower) discard;\n";
              if (tractogram.use_discard_upper())
                source += "  if (g_amp > upper) discard;\n";
            case Ends:
              source += "  colour = fColour;\n";
              break;
            case Manual:
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
          if (do_crop_to_slab != tractogram.tractography_tool.crop_to_slab() ||
              color_type != tractogram.color_type) 
            return true;
          if (tractogram.color_type == ScalarFile)
            if (scalarfile_by_direction != tractogram.scalarfile_by_direction)
              return true;
          if (use_lighting != tractogram.tractography_tool.use_lighting)
            return true;

          return Displayable::Shader::need_update (object);
        }




        void Tractogram::Shader::update (const Displayable& object) 
        {
          const Tractogram& tractogram (dynamic_cast<const Tractogram&> (object));
          do_crop_to_slab = tractogram.tractography_tool.crop_to_slab();
          scalarfile_by_direction = tractogram.scalarfile_by_direction;
          use_lighting = tractogram.tractography_tool.use_lighting;
          color_type = tractogram.color_type;
          Displayable::Shader::update (object);
        }







        Tractogram::Tractogram (Tractography& tool, const std::string& filename) :
            Displayable (filename),
            scalarfile_by_direction (false),
            show_colour_bar (true),
            color_type (Direction),
            original_fov (NAN),
            scalar_filename (""),
            tractography_tool (tool),
            filename (filename),
            sample_stride (0)
        {
          set_allowed_features (true, true, true);
          colourmap = 1;
          connect (&window(), SIGNAL (fieldOfViewChanged()), this, SLOT (on_FOV_changed()));
          on_FOV_changed ();
        }





        Tractogram::~Tractogram ()
        {
          if (vertex_buffers.size())
            gl::DeleteBuffers (vertex_buffers.size(), &vertex_buffers[0]);
          if (vertex_array_objects.size())
            gl::DeleteVertexArrays (vertex_array_objects.size(), &vertex_array_objects[0]);
          if (colour_buffers.size())
            gl::DeleteBuffers (colour_buffers.size(), &colour_buffers[0]);
          if (scalar_buffers.size())
            gl::DeleteBuffers (scalar_buffers.size(), &scalar_buffers[0]);
        }




        void Tractogram::render (const Projection& transform)
        {
          if (tractography_tool.do_crop_to_slab && tractography_tool.slab_thickness <= 0.0)
            return;

          start (track_shader);
          transform.set (track_shader);

          if (tractography_tool.do_crop_to_slab) {
            gl::Uniform3f (gl::GetUniformLocation (track_shader, "screen_normal"),
                transform.screen_normal()[0], transform.screen_normal()[1], transform.screen_normal()[2]);
            gl::Uniform1f (gl::GetUniformLocation (track_shader, "crop_var"),
                window().focus().dot(transform.screen_normal()) - tractography_tool.slab_thickness / 2);
            gl::Uniform1f (gl::GetUniformLocation (track_shader, "slab_width"),
                tractography_tool.slab_thickness);
          }

          if (color_type == ScalarFile) {
            if (use_discard_lower())
              gl::Uniform1f (gl::GetUniformLocation (track_shader, "lower"), lessthan);
            if (use_discard_upper())
              gl::Uniform1f (gl::GetUniformLocation (track_shader, "upper"), greaterthan);
          }
          else if (color_type == Manual)
              gl::Uniform3fv (gl::GetUniformLocation (track_shader, "const_colour"), 1, colour);

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
            float dim[] = {
              window().image()->header().dim (0) * window().image()->header().vox (0),
              window().image()->header().dim (1) * window().image()->header().vox (1),
              window().image()->header().dim (2) * window().image()->header().vox (2)
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
        }




        inline void Tractogram::render_streamlines ()
        {
          for (size_t buf = 0, N= vertex_buffers.size(); buf < N; ++buf) {
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
                  gl::BindBuffer (gl::ARRAY_BUFFER, scalar_buffers[buf]);
                  gl::EnableVertexAttribArray (3);
                  gl::VertexAttribPointer (3, 1, gl::FLOAT, gl::FALSE_, sample_stride * sizeof(float), (void*)0);
                  break;
                default:
                  break;
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
        }




        inline void Tractogram::update_stride ()
        {
          float step_size = (properties.find ("step_size") == properties.end() ? 0.0 : to<float>(properties["step_size"]));
          GLint new_stride = GLint (tractography_tool.line_thickness * original_fov / step_size);
          new_stride = std::max (1, std::min (max_sample_stride, new_stride));

          if (new_stride != sample_stride) {
            sample_stride = new_stride;
            vao_dirty = true;
          }

          should_update_stride = false;
        }



        void Tractogram::load_tracks()
        {
          DWI::Tractography::Reader<float> file (filename, properties);
          DWI::Tractography::Streamline<float> tck;
          std::vector<Point<float> > buffer;
          std::vector<GLint> starts;
          std::vector<GLint> sizes;
          size_t tck_count = 0;

          on_FOV_changed();

          // Make sure to set graphics context!
          // We're setting up vertex array objects
          Window::GrabContext context;

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

            sizes.push_back (N - 1);
            tck_count++;
            if (buffer.size() >= MAX_BUFFER_SIZE)
              load_tracks_onto_GPU (buffer, starts, sizes, tck_count);
          }
          if (buffer.size()) {
            load_tracks_onto_GPU (buffer, starts, sizes, tck_count);
          }
          file.close();
        }
        
        
        
        
        void Tractogram::load_end_colours()
        {
          // Make sure to set graphics context!
          // We're setting up vertex array objects
          Window::GrabContext context;

          erase_nontrack_data();
          // TODO Is it possible to read the track endpoints from the GPU buffer rather than re-reading the .tck file?
          DWI::Tractography::Reader<float> file (filename, properties);
          for (size_t buffer_index = 0, N = vertex_buffers.size(); buffer_index < N; ++buffer_index) {
            size_t num_tracks = num_tracks_per_buffer[buffer_index];
            std::vector< Point<float> > buffer;
            DWI::Tractography::Streamline<float> tck;
            while (num_tracks--) {
              file (tck);
              const Point<float> tangent ((tck.back() - tck.front()).normalise());
              const Point<float> colour (std::abs (tangent[0]), std::abs (tangent[1]), std::abs (tangent[2]));
              for (auto& i : tck)
                i = colour;

              // Pre padding to coincide with tracks buffer
              for (size_t i = 0; i < max_sample_stride; ++i)
                buffer.push_back(tck.front());

              buffer.insert (buffer.end(), tck.begin(), tck.end());

              // Post padding to coincide with tracks buffer
              for (size_t i = 0; i < max_sample_stride; ++i)
                buffer.push_back(tck.back());

            }
            load_end_colours_onto_GPU (buffer);
          }
          file.close();
        }





        void Tractogram::load_track_scalars (const std::string& filename)
        {
          // Make sure to set graphics context!
          // We're setting up vertex array objects
          Window::GrabContext context;

          erase_nontrack_data();
          scalar_filename = filename;
          value_min = std::numeric_limits<float>::infinity();
          value_max = -std::numeric_limits<float>::infinity();
          std::vector<float> buffer;
          std::vector<float> tck_scalar;

          if (Path::has_suffix (filename, ".tsf")) {
            DWI::Tractography::Properties scalar_properties;
            DWI::Tractography::ScalarReader<float> file (filename, scalar_properties);
            DWI::Tractography::check_properties_match (properties, scalar_properties, ".tck / .tsf");
            while (file (tck_scalar)) {

              size_t tck_size = tck_scalar.size();

              if(!tck_size)
                continue;

              // Pre padding to coincide with tracks buffer
              for (size_t i = 1; i < max_sample_stride; ++i)
                buffer.push_back(tck_scalar.front());

              for (size_t i = 0; i < tck_size; ++i) {
                buffer.push_back (tck_scalar[i]);
                value_max = std::max(value_max, tck_scalar[i]);
                value_min = std::min(value_min, tck_scalar[i]);
              }

              // Post padding to coincide with tracks buffer
              for (size_t i = 1; i < max_sample_stride; ++i)
                buffer.push_back(tck_scalar.back());

              if (buffer.size() >= MAX_BUFFER_SIZE)
                load_scalars_onto_GPU (buffer);
            }
            if (buffer.size())
              load_scalars_onto_GPU (buffer);
            file.close();
          } else {
            Math::Vector<float> scalars (filename);
            size_t total_num_tracks = 0;
            for (std::vector<size_t>::const_iterator i = num_tracks_per_buffer.begin(); i != num_tracks_per_buffer.end(); ++i)
              total_num_tracks += *i;
            if (scalars.size() != total_num_tracks)
              throw Exception ("The scalar text file does not contain the same number of elements as the selected tractogram");
            size_t running_index = 0;

            for (size_t buffer_index = 0; buffer_index != vertex_buffers.size(); ++buffer_index) {
              const size_t num_tracks = num_tracks_per_buffer[buffer_index];
              std::vector<GLint>& track_lengths (original_track_sizes[buffer_index]);

              for (size_t index = 0; index != num_tracks; ++index, ++running_index) {
                const float value = scalars[running_index];
                tck_scalar.assign (track_lengths[index], value);

                // Pre padding to coincide with tracks buffer
                for (size_t i = 1; i < max_sample_stride; ++i)
                  buffer.push_back(tck_scalar.front());

                buffer.insert (buffer.end(), tck_scalar.begin(), tck_scalar.end());

                // Post padding to coincide with tracks buffer
                for (size_t i = 1; i < max_sample_stride; ++i)
                  buffer.push_back(tck_scalar.back());

                value_max = std::max(value_max, value);
                value_min = std::min(value_min, value);
              }

              load_scalars_onto_GPU (buffer);
            }
          }
          this->set_windowing (value_min, value_max);
          greaterthan = value_max;
          lessthan = value_min;
        }
        
        
        
        void Tractogram::erase_nontrack_data()
        {
          if (colour_buffers.size()) {
            gl::DeleteBuffers (colour_buffers.size(), &colour_buffers[0]);
            colour_buffers.clear();
          }
          if (scalar_buffers.size()) {
            gl::DeleteBuffers (scalar_buffers.size(), &scalar_buffers[0]);
            scalar_buffers.clear();
            set_use_discard_lower (false);
            set_use_discard_upper (false);
          }
        }




        void Tractogram::load_tracks_onto_GPU (std::vector<Point<float> >& buffer,
            std::vector<GLint>& starts,
            std::vector<GLint>& sizes,
            size_t& tck_count) {

          GLuint vertex_array_object;
          gl::GenVertexArrays (1, &vertex_array_object);
          gl::BindVertexArray (vertex_array_object);

          GLuint vertexbuffer;
          gl::GenBuffers (1, &vertexbuffer);
          gl::BindBuffer (gl::ARRAY_BUFFER, vertexbuffer);
          gl::BufferData (gl::ARRAY_BUFFER, buffer.size() * sizeof(Point<float>), &buffer[0][0], gl::STATIC_DRAW);

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
        }
        
        
        
        
        
        void Tractogram::load_end_colours_onto_GPU (std::vector< Point<float> >& buffer) {
          GLuint vertexbuffer;
          gl::GenBuffers (1, &vertexbuffer);
          gl::BindBuffer (gl::ARRAY_BUFFER, vertexbuffer);
          gl::BufferData (gl::ARRAY_BUFFER, buffer.size() * sizeof(Point<float>), &buffer[0][0], gl::STATIC_DRAW);

          vao_dirty = true;

          colour_buffers.push_back (vertexbuffer);
          buffer.clear();
        }





        void Tractogram::load_scalars_onto_GPU (std::vector<float>& buffer) {

          GLuint vertexbuffer;
          gl::GenBuffers (1, &vertexbuffer);
          gl::BindBuffer (gl::ARRAY_BUFFER, vertexbuffer);
          gl::BufferData (gl::ARRAY_BUFFER, buffer.size() * sizeof(float), &buffer[0], gl::STATIC_DRAW);

          vao_dirty = true;

          scalar_buffers.push_back (vertexbuffer);
          buffer.clear();
        }


      }
    }
  }
}


