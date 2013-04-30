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

#include <QMenu>
#include <stdio.h>

#include "progressbar.h"
#include "image/stride.h"
#include "gui/mrview/tool/tractography/tractogram.h"
#include "gui/mrview/window.h"
#include "gui/projection.h"
#include "dwi/tractography/file.h"
#include "dwi/tractography/scalar_file.h"



const size_t MAX_BUFFER_SIZE = 2796200;  // number of points to fill 32MB

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {

        Tractogram::Tractogram (Window& window, Tractography& tool, const std::string& filename) :
          Displayable (filename),
          scalarfile_by_direction (false),
          do_crop_to_slab (true),
          do_threshold (false),
          show_colour_bar (true),
          color_type (Direction),
          scalar_filename (""),
          window (window),
          tractography_tool (tool),
          filename (filename),
          colourbar_position_index (4) {
            colourmap_index = 1;
        }


        Tractogram::~Tractogram ()
        {
          if (vertex_buffers.size())
            glDeleteBuffers (vertex_buffers.size(), &vertex_buffers[0]);
          if (vertex_array_objects.size())
            glDeleteVertexArrays (vertex_array_objects.size(), &vertex_array_objects[0]);
          if (scalar_buffers.size())
            glDeleteBuffers (scalar_buffers.size(), &scalar_buffers[0]);
        }


        void Tractogram::render2D (const Projection& transform)
        {
          if (tractography_tool.hide_tracks->isChecked())
            return;

          if (tractography_tool.do_crop_to_slab && tractography_tool.slab_thickness <= 0.0)
            return;

          if (tractography_tool.do_shader_update || !(this)) {
            do_crop_to_slab =  tractography_tool.do_crop_to_slab;
            recompile();
            tractography_tool.do_shader_update = false;
          }

          start (transform);

          if (tractography_tool.do_crop_to_slab) {
            glUniform3f (get_uniform ("screen_normal"),
                transform.screen_normal()[0], transform.screen_normal()[1], transform.screen_normal()[2]);
            glUniform1f (get_uniform ("crop_var"),
                window.focus().dot(transform.screen_normal()) - tractography_tool.slab_thickness / 2);
            glUniform1f (get_uniform ("slab_width"),
                tractography_tool.slab_thickness);
          }

          if (color_type == ScalarFile && do_threshold) {
            glUniform1f (get_uniform ("lessthan"), lessthan);
            glUniform1f (get_uniform ("greaterthan"), greaterthan);
          }

//          glEnable( GL_LINE_SMOOTH );
//          glHint( GL_LINE_SMOOTH_HINT, GL_NICEST );
          if (tractography_tool.line_opacity < 1.0) {
            glEnable (GL_BLEND);
            glDisable (GL_DEPTH_TEST);
            glDepthMask (GL_FALSE);
            glBlendEquation (GL_FUNC_ADD);
            glBlendFunc (GL_CONSTANT_ALPHA, GL_ONE);
            glBlendColor (1.0, 1.0, 1.0, tractography_tool.line_opacity);
          } else {
            glDisable (GL_BLEND);
            glEnable (GL_DEPTH_TEST);
            glDepthMask (GL_TRUE);
          }

          glLineWidth (tractography_tool.line_thickness);

          for (size_t buf = 0; buf < vertex_buffers.size(); ++buf) {
            glBindVertexArray (vertex_array_objects[buf]);
            glMultiDrawArrays (GL_LINE_STRIP, &track_starts[buf][0], &track_sizes[buf][0], num_tracks_per_buffer[buf]);
          }

          if (tractography_tool.line_opacity < 1.0) {
            glDisable (GL_BLEND);
            glEnable (GL_DEPTH_TEST);
            glDepthMask (GL_TRUE);
          }

          stop();
        }


        void Tractogram::render3D ()
        {

        }


        void Tractogram::recompile ()
        {

          if (shader_program)
            shader_program.clear();

          std::string vertex_shader_code =
              "layout (location = 0) in vec3 vertexPosition_modelspace;\n"
              "layout (location = 1) in vec3 previousVertex;\n"
              "layout (location = 2) in vec3 nextVertex;\n"
              "out vec3 fragmentColour;\n"
              "uniform mat4 MVP;\n"
              "vec3 color;\n"
              "flat out float amp_out;\n";  //TODO

          if (color_type == ScalarFile)
            vertex_shader_code += "layout (location = 3) in float amp;\n"
                "uniform float offset, scale;\n";

          if (do_crop_to_slab) {
            vertex_shader_code +=
                "out float include;\n"
                "uniform vec3 screen_normal;\n"
                "uniform float crop_var;\n"
                "uniform float slab_width;\n";
          }

          vertex_shader_code +=
              "void main() {\n"
              "  gl_Position =  MVP * vec4(vertexPosition_modelspace,1);\n";

          switch (color_type) {
          case Direction:
            vertex_shader_code +=
                "  if (isnan (previousVertex.x))\n"
                "    color = nextVertex - vertexPosition_modelspace;\n"
                "  else if (isnan (nextVertex.x))\n"
                "    color = vertexPosition_modelspace - previousVertex;\n"
                "  else\n"
                "    color = nextVertex - previousVertex;\n"
                "  color = normalize (abs (color));\n";
            break;
          case Colour:
            vertex_shader_code +=
                "  color = vec3(" + str(colour[0]) + ", " + str(colour[1]) + ", " + str(colour[2]) + ");\n";
            break;
          case ScalarFile:
            vertex_shader_code += "  amp_out = amp;\n";
            if (!ColourMap::maps[colourmap_index].special) {
              vertex_shader_code += "  float amplitude = clamp (";
              if (flags_ & InvertScale) vertex_shader_code += "1.0 -";
              vertex_shader_code += " scale * (amp - offset), 0.0, 1.0);\n  ";
            }
            if (scalarfile_by_direction) {
              vertex_shader_code +=
                  "  if (isnan (previousVertex.x))\n"
                  "    color = nextVertex - vertexPosition_modelspace;\n"
                  "  else if (isnan (nextVertex.x))\n"
                  "    color = vertexPosition_modelspace - previousVertex;\n"
                  "  else\n"
                  "    color = nextVertex - previousVertex;\n"
                  "  color = normalize (abs (color));\n";
            } else {
              vertex_shader_code += ColourMap::maps[colourmap_index].mapping;
            }

            break;
          default:
            assert(0);
            break;
          }

          if (do_crop_to_slab)
            vertex_shader_code +=
                "  include = (dot (vertexPosition_modelspace, screen_normal) - crop_var) / slab_width;\n";

          vertex_shader_code += "  fragmentColour = color;\n}\n";

          std::string fragment_shader_code =
              "in vec3 fragmentColour;\n"
              "in float include; \n"
              "out vec3 color;\n"
              "flat in float amp_out;\n";

          if (color_type == ScalarFile && do_threshold)
            fragment_shader_code += "uniform float lessthan, greaterthan;\n";

          fragment_shader_code +=
              "void main(){\n";

          if (do_crop_to_slab)
            fragment_shader_code += "  if (include < 0 || include > 1) discard;\n";

          if (color_type == ScalarFile && do_threshold)
            fragment_shader_code += "  if (amp_out < lessthan) discard;\n"
                "  if (amp_out > greaterthan) discard;\n";

          fragment_shader_code +=
              "  color = " + std::string ( color_type == Direction ? "normalize" : "" ) + " (fragmentColour);\n"
              "}\n";



          GL::Shader::Vertex vertex_shader (vertex_shader_code);
          GL::Shader::Fragment fragment_shader (fragment_shader_code);
          shader_program.attach (vertex_shader);
          shader_program.attach (fragment_shader);
          shader_program.link();

        }


        void Tractogram::load_tracks()
        {
          DWI::Tractography::Reader<float> file (filename, properties);
          std::vector<Point<float> > tck;
          std::vector<Point<float> > buffer;
          std::vector<GLint> starts;
          std::vector<GLint> sizes;
          size_t tck_count = 0;

          while (file.next (tck)) {
            starts.push_back (buffer.size());
            buffer.push_back (Point<float>());
            buffer.insert (buffer.end(), tck.begin(), tck.end());
            sizes.push_back(tck.size());
            tck_count++;
            if (buffer.size() >= MAX_BUFFER_SIZE)
              load_tracks_onto_GPU (buffer, starts, sizes, tck_count);
          }
          if (buffer.size())
            load_tracks_onto_GPU (buffer, starts, sizes, tck_count);
          file.close();
        }


        void Tractogram::load_track_scalars (std::string filename)
        {
          DWI::Tractography::Properties scalar_properties;
          DWI::Tractography::ScalarReader<float> file (filename, scalar_properties);

//            if (scalar_properties.timestamp != properties.timestamp)
//              throw Exception ("The scalar track file does not match the selected tractogram   ");

          // free any previously loaded scalar data
          if (scalar_buffers.size()) {
            glDeleteBuffers (scalar_buffers.size(), &scalar_buffers[0]);
            scalar_buffers.clear();
          }

          scalar_filename = filename;

          value_min = std::numeric_limits<float>::infinity();
          value_max = -std::numeric_limits<float>::infinity();

          std::vector<float> tck_scalar;
          std::vector<float > buffer;

          while (file.next (tck_scalar)) {
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
          this->set_windowing (value_min, value_max);
          greaterthan = value_max;
          lessthan = value_min;
        }


      }
    }
  }
}


