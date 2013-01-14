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

#include "progressbar.h"
#include "image/stride.h"
#include "gui/mrview/tractogram.h"
#include "gui/mrview/window.h"
#include "gui/projection.h"
#include <stdio.h>

const size_t MAX_BUFFER_SIZE = 349525;  // number of points to fill 4MB

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {

//        Tractogram::Tractogram (const std::string& filename) :
//          Displayable (filename),
//          filename (filename),
//          VertexArrayID (0),
//          use_default_line_thickness (true),
//          line_thickness (1)
//        {
//          file.open (filename, properties);
//          load_tracks();
//        }


        Tractogram::Tractogram (const std::string& filename, Tractography& parent_tool_window) :
          Displayable (filename),
          parent_tool_window (parent_tool_window),
          filename (filename),
          VertexArrayID (0),
          use_default_line_thickness (true),
          line_thickness (1.0)
        {
          file.open (filename, properties);
          load_tracks();
        }


        Tractogram::~Tractogram ()
        {
          file.close();
        }


        void Tractogram::render2D (const Projection& transform)
        {
//          CONSOLE (transform);
          //get current MVP
          // Set MVP uniform

          // compute dot product of postition with MVP z, out that to fragment
          // clip (discard)


          std::string VertexShaderCode =
              "#version 330 core \n "
              "layout(location = 0) in vec3 vertexPosition_modelspace;\n"
              "layout(location = 1) in vec3 previousVertex;\n "
              "layout(location = 2) in vec3 nextVertex;\n "
              "out vec3 fragmentColor;\n"
              "uniform mat4 MVP;\n"
              "void main(){\n"
              "  gl_Position =  MVP * vec4(vertexPosition_modelspace,1);\n"
              "  if (isnan(previousVertex.x))\n"
              "    fragmentColor = nextVertex - vertexPosition_modelspace;\n"
              "  else if (isnan(nextVertex.x))\n"
              "    fragmentColor = vertexPosition_modelspace - previousVertex;\n"
              "  else\n"
              "    fragmentColor = nextVertex - previousVertex;\n"
              "  fragmentColor = normalize (abs(fragmentColor));\n"
              "}";


           std::string FragmentShaderCode = "#version 330 core\n"
                                          "in vec3 fragmentColor;\n"
                                          "out vec3 color;\n"
                                          "void main(){\n"
                                          "  color = fragmentColor;\n"
                                          "}";
           if (!shader) {
             GL::Shader::Vertex vertex_shader;
             vertex_shader.compile (VertexShaderCode);
             shader.attach (vertex_shader);
             GL::Shader::Fragment frag_shader;
             frag_shader.compile (FragmentShaderCode);
             shader.attach (frag_shader);
             shader.link();
           }

           if (!VertexArrayID)
             glGenVertexArrays(1, &VertexArrayID);

           glBindVertexArray(VertexArrayID);

           glEnable(GL_DEPTH_TEST);
           glDepthMask(GL_TRUE);

          // Use our shader
          shader.start();
          // Send our transformation to the currently bound shader,
          // in the "MVP" uniform
          DEBUG_OPENGL;
          GLuint MatrixID = glGetUniformLocation (shader, "MVP");
          DEBUG_OPENGL;

          const Math::Matrix<float>& M (transform.get_MVP());
          GLfloat MVP[] = {
            M(0,0), M(1,0), M(2,0), M(3,0),
            M(0,1), M(1,1), M(2,1), M(3,1),
            M(0,2), M(1,2), M(2,2), M(3,2),
            M(0,3), M(1,3), M(2,3), M(3,3)
          };
          DEBUG_OPENGL;
          glUniformMatrix4fv (MatrixID, 1, GL_FALSE, MVP);

          DEBUG_OPENGL;

          for (size_t buf = 0; buf < vertex_buffers.size(); ++buf) {
            // 1rst attribute buffer : vertices
            DEBUG_OPENGL;
            glEnableVertexAttribArray (0);
            glBindBuffer (GL_ARRAY_BUFFER, vertex_buffers[buf]);
            glVertexAttribPointer (
                0,                  // attribute. No particular reason for 0, but must match the layout in the shader.
                3,                  // size
                GL_FLOAT,           // type
                GL_FALSE,           // normalized?
                0,                  // stride
                (void*)(3*sizeof(float))            // array buffer offset
            );
            DEBUG_OPENGL;

            // 1rst attribute buffer : vertices
            glEnableVertexAttribArray (1);
            glVertexAttribPointer (
                1,                  // attribute. No particular reason for 0, but must match the layout in the shader.
                3,                  // size
                GL_FLOAT,           // type
                GL_FALSE,           // normalized?
                0,                  // stride
                (void*)0            // array buffer offset
            );
            DEBUG_OPENGL;

            // 1rst attribute buffer : vertices
            glEnableVertexAttribArray (2);
            glVertexAttribPointer (
                2,                  // attribute. No particular reason for 0, but must match the layout in the shader.
                3,                  // size
                GL_FLOAT,           // type
                GL_FALSE,           // normalized?
                0,                  // stride
                (void*)(6*sizeof(float))            // array buffer offset
            );

            DEBUG_OPENGL;

            if (use_default_line_thickness)
              glLineWidth(parent_tool_window.get_line_thickness());
            else
              glLineWidth(line_thickness);

            DEBUG_OPENGL;
            glMultiDrawArrays (GL_LINE_STRIP, &track_starts[buf][0], &track_sizes[buf][0], num_tracks_per_buffer[buf]);
          }
          DEBUG_OPENGL;

          shader.stop();
          DEBUG_OPENGL;

        }

        void Tractogram::render3D ()
        {

        }


        void Tractogram::load_tracks()
        {
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

            if (buffer.size() >= MAX_BUFFER_SIZE) {
              buffer.push_back (Point<float>());
              GLuint vertexbuffer;
              glGenBuffers (1, &vertexbuffer);
              glBindBuffer (GL_ARRAY_BUFFER, vertexbuffer);
              glBufferData (GL_ARRAY_BUFFER, buffer.size() * sizeof(Point<float>), &buffer[0][0], GL_STATIC_DRAW);
              vertex_buffers.push_back (vertexbuffer);
              track_starts.push_back (starts);
              track_sizes.push_back (sizes);
              num_tracks_per_buffer.push_back (tck_count);
              tck_count = 0;
              buffer.clear();
              starts.clear();
              sizes.clear();
            }
          }
          buffer.push_back (Point<float>());
          GLuint vertexbuffer;
          glGenBuffers (1, &vertexbuffer);
          glBindBuffer (GL_ARRAY_BUFFER, vertexbuffer);
          glBufferData (GL_ARRAY_BUFFER, buffer.size() * sizeof(Point<float>), &buffer[0][0], GL_STATIC_DRAW);
          vertex_buffers.push_back (vertexbuffer);
          track_starts.push_back (starts);
          track_sizes.push_back (sizes);
          num_tracks_per_buffer.push_back (tck_count);
        }

      }
    }
  }
}


