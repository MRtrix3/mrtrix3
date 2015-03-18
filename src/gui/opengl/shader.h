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

#include "gui/opengl/gl.h"
#include "app.h"

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
            Object (const std::string& source) : index_ (0) { if(!source.empty()) compile (source); }
            ~Object () {
              if (index_) {
                GL_DEBUG ("deleting OpenGL shader ID " + str(index_));
                gl::DeleteShader (index_);
              }
            }
            operator GLuint () const {
              return (index_);
            }
            void compile (const std::string& source) {
              std::string code = "#version 330 core\n" + source;
              DEBUG ("compiling OpenGL " + this->type() + " shader:\n" + code);
              if (!index_) {
                index_ = gl::CreateShader (TYPE);
                GL_DEBUG ("created OpenGL " + this->type() + " shader ID " + str (index_));
              }
              const char* p = code.c_str();
              gl::ShaderSource (index_, 1, &p, NULL);
              gl::CompileShader (index_);
              GLint status;
              gl::GetShaderiv (index_, gl::COMPILE_STATUS, &status);
              if (status == gl::FALSE_) {
                debug();
                throw Exception ("error compiling OpenGL " + this->type() + " shader ID " + str (index_));
              }
            }
            static const std::string type() { return TYPE == gl::VERTEX_SHADER ? "vertex" : "fragment"; }

            void debug () {
              assert (index_);
              print_log (false, this->type() + " shader", index_);
            }

          protected:
            GLuint index_;
            friend class Program;
        };

        typedef Object<gl::VERTEX_SHADER> Vertex;
        typedef Object<gl::GEOMETRY_SHADER> Geometry;
        typedef Object<gl::FRAGMENT_SHADER> Fragment;



        class Program
        {
          public:
            Program () : index_ (0) { }
            ~Program () { clear(); }
        
            void clear () {
              if (index_) {
                GL_DEBUG ("deleting OpenGL shader program " + str(index_));
                gl::DeleteProgram (index_);
              }
              index_ = 0;
            }
            operator GLuint () const {
              return index_;
            }
            template <GLint TYPE> void attach (const Object<TYPE>& object) {
              if (!index_) {
                index_ = gl::CreateProgram();
                GL_DEBUG ("created OpenGL shader program ID " + str(index_));
              }
              gl::AttachShader (index_, object.index_);
              GL_DEBUG ("attached OpenGL " + Object<TYPE>::type() + " shader ID " + str(object.index_) + " to program ID " + str(index_));
            }
            template <GLint TYPE> void detach (const Object<TYPE>& object) {
              assert (index_);
              assert (object.index_);
              gl::DetachShader (index_, object.index_);
              GL_DEBUG ("detached OpenGL " + Object<TYPE>::type() + " shader ID " + str(object.index_) + " from program ID " + str(index_));
            }
            void link () {
              GL_DEBUG ("linking OpenGL shader program ID " + str(index_) + "...");
              assert (index_);
              gl::LinkProgram (index_);
              GLint status;
              gl::GetProgramiv (index_, gl::LINK_STATUS, &status);
              if (status == gl::FALSE_) {
                debug();
                throw Exception (std::string ("error linking OpenGL shader program ID " + str(index_)));
              }
            }
            void start () const {
              assert (index_);
              gl::UseProgram (index_);
              GL_DEBUG ("using OpenGL shader program ID " + str(index_));
            }
            static void stop () {
              gl::UseProgram (0);
              GL_DEBUG ("using default OpenGL shader program");
            }

            void debug () const {
              assert (index_);
              print_log (true, "OpenGL shader program", index_);
            }

            Program& operator= (Program& P) {
              clear();
              index_ = P.index_;
              P.index_ = 0;
              return *this;
            }

          protected:
            GLuint index_;
        };

      }
    }
  }
}

#endif

