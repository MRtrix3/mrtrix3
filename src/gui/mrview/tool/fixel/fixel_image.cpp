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

#include "gui/mrview/tool/fixel/fixel_image.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {

        std::string FixelImage::Shader::vertex_shader_source (const Displayable& fixel)
        {

          std::string source =
              "layout (location = 0) in vec3 vertexPosition_modelspace;\n"
              "layout (location = 1) in vec3 previousVertex;\n"
              "layout (location = 2) in vec3 nextVertex;\n"
              "uniform mat4 MVP;\n"
              "flat out float amp_out;\n"
              "out vec3 fragmentColour;\n";

          switch (color_type) {
            case Direction: break;
            case Colour:
              source += "uniform vec3 const_colour;\n";
              break;
            case Value:
              source += "layout (location = 3) in float amp;\n"
                        "uniform float offset, scale;\n";
              break;
          }

          source +=
              "void main() {\n"
              "  gl_Position =  MVP * vec4(vertexPosition_modelspace,1);\n";

//          if (color_type == Direction)
//            source +=
//              "  vec3 dir;\n"
//              "  if (isnan (previousVertex.x))\n"
//              "    dir = nextVertex - vertexPosition_modelspace;\n"
//              "  else if (isnan (nextVertex.x))\n"
//              "    dir = vertexPosition_modelspace - previousVertex;\n"
//              "  else\n"
//              "    dir = nextVertex - previousVertex;\n";
//              source += "  fragmentColour = dir;\n";

          switch (color_type) {
            case Colour:
              source +=
                  "  fragmentColour = const_colour;\n";
              break;
            case Value:
              source += "  amp_out = amp;\n";
              if (!ColourMap::maps[colourmap].special) {
                source += "  float amplitude = clamp (";
                if (fixel.scale_inverted())
                  source += "1.0 -";
                source += " scale * (amp - offset), 0.0, 1.0);\n  ";
              }
              break;
            default:
              break;
          }

          source += "}\n";

          return source;
        }



        std::string FixelImage::Shader::fragment_shader_source (const Displayable& fixel)
        {
          std::string source =
              "in float include; \n"
              "out vec3 color;\n"
              "flat in float amp_out;\n"
              "in vec3 fragmentColour;\n";
          if (color_type == Value) {
            if (fixel.use_discard_lower())
              source += "uniform float lower;\n";
            if (fixel.use_discard_upper())
              source += "uniform float upper;\n";
          }

          source +=
              "void main(){\n";

          if (color_type == Value) {
            if (fixel.use_discard_lower())
              source += "  if (amp_out < lower) discard;\n";
            if (fixel.use_discard_upper())
              source += "  if (amp_out > upper) discard;\n";
          }

          source +=
            std::string("  color = ") + ((color_type == Direction) ? "normalize (abs (fragmentColour))" : "fragmentColour" ) + ";\n";

          source += "}\n";

          return source;
        }


        bool FixelImage::Shader::need_update (const Displayable& object) const
        {
          const FixelImage& fixel (dynamic_cast<const FixelImage&> (object));
          if (color_type != fixel.color_type)
            return true;
          return Displayable::Shader::need_update (object);
        }




        void FixelImage::Shader::update (const Displayable& object)
        {
          const FixelImage& fixel (dynamic_cast<const FixelImage&> (object));
          do_crop_to_slice = fixel.fixel_tool.do_crop_to_slice;
          color_type = fixel.color_type;
          Displayable::Shader::update (object);
        }





        void FixelImage::render (const Projection& transform, bool is_3D, int plane, int slice) {

          start (fixel_shader);
          transform.set (fixel_shader);

          if (color_type == Value) {
            if (use_discard_lower())
              gl::Uniform1f (gl::GetUniformLocation (fixel_shader, "lower"), lessthan);
            if (use_discard_upper())
              gl::Uniform1f (gl::GetUniformLocation (fixel_shader, "upper"), greaterthan);
          }
          else if (color_type == Colour)
            gl::Uniform3fv (gl::GetUniformLocation (fixel_shader, "const_colour"), 1, colour);

          if (fixel_tool.line_opacity < 1.0) {
            gl::Enable (gl::BLEND);
            gl::Disable (gl::DEPTH_TEST);
            gl::DepthMask (gl::FALSE_);
            gl::BlendEquation (gl::FUNC_ADD);
            gl::BlendFunc (gl::CONSTANT_ALPHA, gl::ONE);
            gl::BlendColor (1.0, 1.0, 1.0, fixel_tool.line_opacity);
          } else {
            gl::Disable (gl::BLEND);
            gl::Enable (gl::DEPTH_TEST);
            gl::DepthMask (gl::TRUE_);
          }

          gl::LineWidth (fixel_tool.line_thickness);


          std::vector<Point<float> > buffer;
          std::vector<float> values;
          std::vector<GLint> starts;
          std::vector<GLint> sizes;
          size_t count = 0;
          MR::Image::Loop loop (fixel_vox);
          for (loop.start (fixel_vox); loop.ok(); loop.next (fixel_vox)) {
            for (size_t f = 0; f != fixel_vox.value().size(); ++f) {


              transform.voxel2scanner (fixel_vox, voxel_pos);
              starts.push_back (buffer.size());
              buffer.push_back (Point<float>());
              buffer.push_back (voxel_pos + (fixel_vox.value()[f].dir *  line_length));
              buffer.push_back (voxel_pos + (fixel_vox.value()[f].dir * -line_length));
              sizes.push_back (2);
              count++;



             // tck_writer (tck);
              //if (tsf_writer) {
               // std::vector<float> scalars;
               // scalars.push_back (fixel_vox.value()[f].value);
               // scalars.push_back (fixel_vox.value()[f].value);
               // (*tsf_writer) (scalars);
             // }
            }
	  }

  //              for (size_t buf = 0; buf < vertex_buffers.size(); ++buf) {
  //                gl::BindVertexArray (vertex_array_objects[buf]);
  //                gl::MultiDrawArrays (gl::LINE_STRIP, &track_starts[buf][0], &track_sizes[buf][0], num_tracks_per_buffer[buf]);
  //              }

          if (fixel_tool.line_opacity < 1.0) {
            gl::Disable (gl::BLEND);
            gl::Enable (gl::DEPTH_TEST);
            gl::DepthMask (gl::TRUE_);
          }

          stop (track_shader);
        }


      }
    }
  }
}
