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



const size_t MAX_BUFFER_SIZE = 2796200;  // number of points to fill 32MB

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {

        std::string Tractogram::Shader::vertex_shader_source (const Displayable& displayable)
        {
          const Tractogram& tractogram = dynamic_cast<const Tractogram&>(displayable);

          bool colour_by_direction = (color_type == Direction ||
           (color_type == ScalarFile && scalarfile_by_direction));

          std::string source =
          "layout (location = 0) in vec3 this_vertex;\n"
          "layout (location = 1) in vec3 prev_vertex;\n"
          "layout (location = 2) in vec3 next_vertex;\n";

          switch(color_type) {
            case Direction: break;
            case Ends:
              source += "layout (location = 3) in vec3 colour;\n";
              break;
            case Manual:
              break;
            case ScalarFile:
              source += "layout (location = 3) in float amp;\n";
              break;
          }

          source +=
          "uniform mat4 MVP;\n"
          "uniform float line_thickness;\n"

          // Uniforms won't be included in compiled shader if not referenced
          // so we can unconditionally list all of them
          "uniform mat4 MV;\n"
          "uniform vec3 const_colour;\n"
          "uniform float offset, scale;\n"

          "uniform float lower;\n"
          "uniform float upper;\n"

          "uniform vec3 screen_normal;\n"
          "uniform float crop_var;\n"
          "uniform float slab_width;\n";

          source +=
          "out vec4 v_lower_point;\n"
          "out vec4 v_upper_point;\n"
          "out vec3 v_colour;\n"
          "flat out float v_include;\n";

          if(use_lighting)
            source += "out vec3 v_tangent;\n";

          // Include function
          source +=
          "void set_include_flag() {\n"
          "  bool include_flag = true;\n";

          if(do_crop_to_slab)
            source +=
            "  float include = (dot(this_vertex, screen_normal) - crop_var) / slab_width;\n"
            "  include_flag = include_flag && (include > 0 && include < 1);";

          if(color_type == ScalarFile) {
            if(tractogram.use_discard_lower())
              source += "  include_flag = include_flag && (amp < lower);\n";
            if(tractogram.use_discard_upper())
              source += "  include_flag = include_flag && (amp > upper);\n";
          }
          source +="  v_include = include_flag ? 1.0 : 0.0;\n"
                   "}\n\n";

          // Colour and lighting function
          source +=
          "void set_colour_and_lighting() {\n";

          if(use_lighting || colour_by_direction)
            source +=
            "  vec3 dir = next_vertex - this_vertex;\n";
          if(colour_by_direction)
            source += "  v_colour = normalize (abs(dir));\n";
          if(use_lighting)
            source += "  v_tangent = normalize (mat3(MV) * dir);\n";

          switch (color_type) {
            case Ends:
              source += "  v_colour = colour;\n";
              break;
            case Manual:
              source += "  v_colour = const_colour;\n";
              break;
            case ScalarFile:
              if(!ColourMap::maps[colourmap].special) {
                source += "   float amplitude = clamp (";
                if (tractogram.scale_inverted()) source += "1.0 -";
                source += " scale * (amp - offset), 0.0, 1.0);\n";
              }
              if (!scalarfile_by_direction)
                source +=
                std::string("  vec3 color;\n") +
                ColourMap::maps[colourmap].mapping +
                "  v_colour = color;\n";
              break;
            default:
              break;
          }

          source += "}\n\n";


          // Main function
          source +=
          "void main() {\n"
          "  gl_Position = MVP * vec4(this_vertex, 1);\n"
          "  set_include_flag();\n"

          "  vec4 prev_point = MVP * vec4(prev_vertex, 1);\n"
          "  vec4 end_point = MVP * vec4(next_vertex, 1);\n"
          "  vec4 line_prev = gl_Position - prev_point;\n"
          "  vec4 line_next = end_point - gl_Position;\n"
          "  vec4 normal_prev =  normalize(vec4(-line_prev.y, line_prev.x, 0.0, 0.0));\n"
          "  vec4 normal_next =  normalize(vec4(-line_next.y, line_next.x, 0.0, 0.0));\n"
          "  vec4 normal_middle = mix(normal_prev, normal_next, 0.5);\n"
          // The length of the normal_middle projected onto normal_prev should = thickness
          "  float scale = line_thickness / clamp(abs(dot(normal_middle, normal_prev)), 0.02, 1.0);\n"
          "  v_lower_point = gl_Position - (scale * normal_middle);\n"
          "  v_upper_point = gl_Position + (scale * normal_middle);\n"

          "  set_colour_and_lighting();\n"
          "}\n";

          QTextStream out(stdout);
          out << QString::fromStdString(source);

          return source;
        }


        std::string Tractogram::Shader::geometry_shader_source (const Displayable&)
        {
          std::string source =
          "layout(lines) in;\n"
          "layout(triangle_strip, max_vertices = 4) out;\n"

          "in vec4 v_lower_point[];\n"
          "in vec4 v_upper_point[];\n"
          "in vec3 v_colour[];\n"
          "flat in float v_include[];\n";

          if(use_lighting)
           source += "in vec3 v_tangent[];\n";

          source +=
          "out vec3 fColour;\n"
          "out vec3 g_tangent;\n"
          "flat out float g_include;\n"

          "void main() {\n"
          "  g_include = v_include[0] + v_include[1];\n"
          // Don't create complex primitives if not visible
          "  if (g_include < 0.5) return;\n"

          "  fColour = v_colour[0];\n";

          if(use_lighting)
            source += "  g_tangent = v_tangent[0];\n";

          source +=
          "  gl_Position = v_lower_point[0];\n"
          "  EmitVertex();\n"
          "  gl_Position = v_upper_point[0];\n"
          "  EmitVertex();\n"

          "  fColour = v_colour[1];\n";

          if(use_lighting)
            source += "  g_tangent = v_tangent[1];\n";

          source +=
          "  gl_Position = v_lower_point[1];\n"
          "  EmitVertex();\n"
          "  gl_Position = v_upper_point[1];\n"
          "  EmitVertex();\n"

          "  EndPrimitive();\n"
          "}\n";

          QTextStream out(stdout);
          out << QString::fromStdString(source);

          return source;
        }


        std::string Tractogram::Shader::fragment_shader_source (const Displayable&)
        {
          std::string source =
            "in vec3 fColour;\n"
            "flat in float g_include;\n"
            "out vec3 colour;\n";

          if (use_lighting)
            source += 
              "in vec3 g_tangent;\n"
              "uniform float ambient, diffuse, specular, shine;\n"
              "uniform vec3 light_pos;\n";

          source +=
              "void main() {\n"
              "  if (g_include < 0.5) discard;\n"
              "  colour = fColour;\n";

          if (use_lighting)
            source +=
             "  vec3 t = normalize (g_tangent);\n"
             "  float l_dot_t = dot(light_pos, t);\n"
             "  vec3 l_perp = light_pos - l_dot_t * t;\n"
             "  vec3 l_perp_norm = normalize (l_perp);\n"
             "  float n_dot_t = t.z;\n"
             "  vec3 n_perp = vec3(0.0, 0.0, 1.0) - n_dot_t * t;\n"
             "  vec3 n_perp_norm = normalize (n_perp);\n"
             "  float cos2_theta = 0.5+0.5*dot(l_perp_norm,n_perp_norm);\n"
             "  colour *= ambient + diffuse * length(l_perp) * cos2_theta;\n"
             "  colour += specular * sqrt(cos2_theta) * pow (clamp (-l_dot_t*n_dot_t + length(l_perp)*length(n_perp), 0, 1), shine);\n";

          source += "}\n";

          QTextStream out(stdout);
          out << QString::fromStdString(source);

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







        Tractogram::Tractogram (Window& window, Tractography& tool, const std::string& filename) :
            Displayable (filename),
            scalarfile_by_direction (false),
            show_colour_bar (true),
            color_type (Direction),
            scalar_filename (""),
            window (window),
            tractography_tool (tool),
            filename (filename),
            colourbar_position_index (4)
        {
          set_allowed_features (true, true, true);
          colourmap = 1;
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
                window.focus().dot(transform.screen_normal()) - tractography_tool.slab_thickness / 2);
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

          gl::Uniform1f (gl::GetUniformLocation (track_shader, "line_thickness"), tractography_tool.line_thickness);

          if (tractography_tool.line_opacity < 1.0) {
            gl::Enable (gl::BLEND);
            gl::Disable (gl::DEPTH_TEST);
            gl::DepthMask (gl::FALSE_);
            gl::BlendEquation (gl::FUNC_ADD);
            gl::BlendFunc (gl::CONSTANT_ALPHA, gl::ONE);
            gl::BlendColor (1.0, 1.0, 1.0, tractography_tool.line_opacity);
          } else {
            gl::Disable (gl::BLEND);
            gl::Enable (gl::DEPTH_TEST);
            gl::DepthMask (gl::TRUE_);
          }

          for (size_t buf = 0, N= vertex_buffers.size(); buf < N; ++buf) {
            gl::BindVertexArray (vertex_array_objects[buf]);
            gl::MultiDrawArrays (gl::LINE_STRIP, &track_starts[buf][0], &track_sizes[buf][0], num_tracks_per_buffer[buf] - 1);
          }

          if (tractography_tool.line_opacity < 1.0) {
            gl::Disable (gl::BLEND);
            gl::Enable (gl::DEPTH_TEST);
            gl::DepthMask (gl::TRUE_);
          }

          stop (track_shader);
        }








        void Tractogram::load_tracks()
        {
          DWI::Tractography::Reader<float> file (filename, properties);
          DWI::Tractography::Streamline<float> tck;
          std::vector<Point<float> > buffer;
          std::vector<GLint> starts;
          std::vector<GLint> sizes;
          size_t tck_count = 0;

          while (file (tck)) {
            starts.push_back (buffer.size() + 1);
            buffer.push_back (Point<float>());
            buffer.insert (buffer.end(), tck.begin(), tck.end());
            sizes.push_back (tck.size()-1);
            tck_count++;
            if (buffer.size() >= MAX_BUFFER_SIZE)
              load_tracks_onto_GPU (buffer, starts, sizes, tck_count);
          }
          if (buffer.size())
            load_tracks_onto_GPU (buffer, starts, sizes, tck_count);
          file.close();
        }
        
        
        
        
        void Tractogram::load_end_colours()
        {
          erase_nontrack_data();
          // TODO Is it possible to read the track endpoints from the GPU buffer rather than re-reading the .tck file?
          DWI::Tractography::Reader<float> file (filename, properties);
          for (size_t buffer_index = 0; buffer_index != vertex_buffers.size(); ++buffer_index) {
            size_t num_tracks = num_tracks_per_buffer[buffer_index];
            std::vector< Point<float> > buffer;
            DWI::Tractography::Streamline<float> tck;
            while (num_tracks--) {
              file (tck);
              const Point<float> tangent ((tck.back() - tck.front()).normalise());
              const Point<float> colour (std::abs (tangent[0]), std::abs (tangent[1]), std::abs (tangent[2]));
              buffer.push_back (Point<float>());
              for (std::vector< Point<float> >::iterator i = tck.begin(); i != tck.end(); ++i)
                *i = colour;
              buffer.insert (buffer.end(), tck.begin(), tck.end());
            }
            load_end_colours_onto_GPU (buffer);
          }
          file.close();
        }





        void Tractogram::load_track_scalars (const std::string& filename)
        {
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
              buffer.push_back (NAN);
              for (size_t i = 0; i < tck_scalar.size(); ++i) {
                buffer.push_back (tck_scalar[i]);
                if (tck_scalar[i] > value_max) value_max = tck_scalar[i];
                if (tck_scalar[i] < value_min) value_min = tck_scalar[i];
              }
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
              std::vector<GLint>& track_lengths (track_sizes[buffer_index]);
              for (size_t index = 0; index != num_tracks; ++index, ++running_index) {
                const float value = scalars[running_index];
                buffer.push_back (NAN);
                tck_scalar.assign (track_lengths[index], value);
                buffer.insert (buffer.end(), tck_scalar.begin(), tck_scalar.end());
                if (value > value_max) value_max = value;
                if (value < value_min) value_min = value;
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
          buffer.push_back (Point<float>());
          GLuint vertexbuffer;
          gl::GenBuffers (1, &vertexbuffer);
          gl::BindBuffer (gl::ARRAY_BUFFER, vertexbuffer);
          gl::BufferData (gl::ARRAY_BUFFER, buffer.size() * sizeof(Point<float>), &buffer[0][0], gl::STATIC_DRAW);

          GLuint vertex_array_object;
          gl::GenVertexArrays (1, &vertex_array_object);
          gl::BindVertexArray (vertex_array_object);
          gl::EnableVertexAttribArray (0);
          gl::VertexAttribPointer (0, 3, gl::FLOAT, gl::FALSE_, 0, (void*)(3*sizeof(float)));
          gl::EnableVertexAttribArray (1);
          gl::VertexAttribPointer (1, 3, gl::FLOAT, gl::FALSE_, 0, (void*)0);
          gl::EnableVertexAttribArray (2);
          gl::VertexAttribPointer (2, 3, gl::FLOAT, gl::FALSE_, 0, (void*)(6*sizeof(float)));

          vertex_array_objects.push_back (vertex_array_object);
          vertex_buffers.push_back (vertexbuffer);
          track_starts.push_back (starts);
          track_sizes.push_back (sizes);
          num_tracks_per_buffer.push_back (tck_count);
          buffer.clear();
          starts.clear();
          sizes.clear();
          tck_count = 0;
        }
        
        
        
        
        
        void Tractogram::load_end_colours_onto_GPU (std::vector< Point<float> >& buffer) {
          buffer.push_back (Point<float>());
          GLuint vertexbuffer;
          gl::GenBuffers (1, &vertexbuffer);
          gl::BindBuffer (gl::ARRAY_BUFFER, vertexbuffer);
          gl::BufferData (gl::ARRAY_BUFFER, buffer.size() * sizeof(Point<float>), &buffer[0][0], gl::STATIC_DRAW);

          gl::BindVertexArray (vertex_array_objects[colour_buffers.size()]);
          gl::EnableVertexAttribArray (3);
          gl::VertexAttribPointer (3, 3, gl::FLOAT, gl::FALSE_, 0, (void*)(3*sizeof(float)));

          colour_buffers.push_back (vertexbuffer);
          buffer.clear();
        }





        void Tractogram::load_scalars_onto_GPU (std::vector<float>& buffer) {
          buffer.push_back (NAN);
          GLuint vertexbuffer;
          gl::GenBuffers (1, &vertexbuffer);
          gl::BindBuffer (gl::ARRAY_BUFFER, vertexbuffer);
          gl::BufferData (gl::ARRAY_BUFFER, buffer.size() * sizeof(float), &buffer[0], gl::STATIC_DRAW);

          gl::BindVertexArray (vertex_array_objects[scalar_buffers.size()]);
          gl::EnableVertexAttribArray (3);
          gl::VertexAttribPointer (3, 1, gl::FLOAT, gl::FALSE_, 0, (void*)(sizeof(float)));
          scalar_buffers.push_back (vertexbuffer);
          buffer.clear();
        }



      }
    }
  }
}


