/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 12/01/09.

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

#ifndef __gui_opengl_shader_h__
#define __gui_opengl_shader_h__

#include "app.h"
#include "gui/opengl/gl.h"

namespace MR
{
  namespace GUI
  {
    namespace GL
    {
      namespace Shader
      {

        void print_log (bool is_program, const std::string& type_name, GLuint index);

        class Program;

        template <GLint TYPE> class Object
        {
          public:
            Object () : index_ (0) { }
            ~Object () {
              if (index_) glDeleteShader (index_);
            }
            operator GLuint () const {
              return (index_);
            }
            void compile (const std::string& source) {
              if (App::log_level > 2) {
                std::string msg ("compiling OpenGL ");
                msg += TYPE == GL_VERTEX_SHADER ? "vertex" : "fragment";
                msg += " shader:\n" + source;
                DEBUG (msg);
              }
              if (!index_) index_ = glCreateShader (TYPE);
              const char* p = source.c_str();
              glShaderSource (index_, 1, &p, NULL);
              glCompileShader (index_);
              GLint status;
              glGetShaderiv (index_, GL_COMPILE_STATUS, &status);
              if (status == 0) {
                debug();
                throw Exception (std::string ("error compiling ") +
                                 (TYPE == GL_VERTEX_SHADER ? "vertex shader" : "fragment shader"));
              }


            }
            void debug () {
              assert (index_);
              print_log (false,
                (TYPE == GL_VERTEX_SHADER ? "vertex shader" : "fragment shader"),
                index_);
            }

          protected:
            GLuint index_;
            friend class Program;
        };

        typedef Object<GL_VERTEX_SHADER> Vertex;
        typedef Object<GL_FRAGMENT_SHADER> Fragment;


        class Uniform
        {
          public:
            float operator= (float value) {
              glUniform1f (index_, value);
              return (value);
            }
            int operator= (int value) {
              glUniform1i (index_, value);
              return (value);
            }

          protected:
            GLint index_;
            Uniform (GLint index) : index_ (index) { }
            friend class Program;
        };



        class Program
        {
          public:
            Program () : index_ (0) { }
            ~Program () {
              if (index_) glDeleteProgram (index_);
            }
            void clear () {
              if (index_) glDeleteProgram (index_);
              index_ = 0;
            }
            operator GLuint () const {
              return (index_);
            }
            template <GLint TYPE> void attach (const Object<TYPE>& object) {
              if (!index_) index_ = glCreateProgram();
              glAttachShader (index_, object.index_);
            }
            template <GLint TYPE> void detach (const Object<TYPE>& object) {
              assert (index_);
              assert (object.index_);
              glDetachShader (index_, object.index_);
            }
            void link () {
              DEBUG ("linking OpenGL shader program...");
              assert (index_);
              glLinkProgram (index_);
              GLint status;
              glGetProgramiv (index_, GL_LINK_STATUS, &status);
              if (status == 0) {
                debug();
                throw Exception (std::string ("error linking shader program"));
              }
            }
            void start () {
              assert (index_);
              glUseProgram (index_);
            }
            static void stop () {
              glUseProgram (0);
            }

            Uniform get_uniform (const std::string& name) {
              return (Uniform (glGetUniformLocation (index_, name.c_str())));
            }
            void debug () {
              assert (index_);
              print_log (true, "shader program", index_);
            }

          protected:
            GLuint index_;
        };

      }
    }
  }
}

#endif

