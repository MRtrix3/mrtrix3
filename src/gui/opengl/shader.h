/*******************************************************************************
    Copyright (C) 2014 Brain Research Institute, Melbourne, Australia
    
    Permission is hereby granted under the Patent Licence Agreement between
    the BRI and Siemens AG from July 3rd, 2012, to Siemens AG obtaining a
    copy of this software and associated documentation files (the
    "Software"), to deal in the Software without restriction, including
    without limitation the rights to possess, use, develop, manufacture,
    import, offer for sale, market, sell, lease or otherwise distribute
    Products, and to permit persons to whom the Software is furnished to do
    so, subject to the following conditions:
    
    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.
    
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
    OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
    CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
    TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
    SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*******************************************************************************/


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
            Object (const std::string& source) : index_ (0) { compile (source); }
            ~Object () {
              if (index_) gl::DeleteShader (index_);
            }
            operator GLuint () const {
              return (index_);
            }
            void compile (const std::string& source) {
              std::string code = "#version 330 core\n" + source;
              if (App::log_level > 2) {
                std::string msg ("compiling OpenGL ");
                msg += TYPE == gl::VERTEX_SHADER ? "vertex" : "fragment";
                msg += " shader:\n" + code;
                DEBUG (msg);
              }
              if (!index_) index_ = gl::CreateShader (TYPE);
              const char* p = code.c_str();
              gl::ShaderSource (index_, 1, &p, NULL);
              gl::CompileShader (index_);
              GLint status;
              gl::GetShaderiv (index_, gl::COMPILE_STATUS, &status);
              if (status == gl::FALSE_) {
                debug();
                throw Exception (std::string ("error compiling ") +
                                 (TYPE == gl::VERTEX_SHADER ? "vertex shader" : "fragment shader"));
              }


            }
            void debug () {
              assert (index_);
              print_log (false,
                (TYPE == gl::VERTEX_SHADER ? "vertex shader" : "fragment shader"),
                index_);
            }

          protected:
            GLuint index_;
            friend class Program;
        };

        typedef Object<gl::VERTEX_SHADER> Vertex;
        typedef Object<gl::FRAGMENT_SHADER> Fragment;



        class Program
        {
          public:
            Program () : index_ (0) { }
            ~Program () {
              if (index_) gl::DeleteProgram (index_);
            }
            void clear () {
              if (index_) gl::DeleteProgram (index_);
              index_ = 0;
            }
            operator GLuint () const {
              return (index_);
            }
            template <GLint TYPE> void attach (const Object<TYPE>& object) {
              if (!index_) index_ = gl::CreateProgram();
              gl::AttachShader (index_, object.index_);
            }
            template <GLint TYPE> void detach (const Object<TYPE>& object) {
              assert (index_);
              assert (object.index_);
              gl::DetachShader (index_, object.index_);
            }
            void link () {
              DEBUG ("linking OpenGL shader program...");
              assert (index_);
              gl::LinkProgram (index_);
              GLint status;
              gl::GetProgramiv (index_, gl::LINK_STATUS, &status);
              if (status == gl::FALSE_) {
                debug();
                throw Exception (std::string ("error linking shader program"));
              }
            }
            void start () const {
              assert (index_);
              gl::UseProgram (index_);
            }
            static void stop () {
              gl::UseProgram (0);
            }

            void debug () const {
              assert (index_);
              print_log (true, "shader program", index_);
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

