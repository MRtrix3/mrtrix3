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

#ifndef __opengl_shader_h__
#define __opengl_gl_h__

#include "opengl/gl.h"

namespace MR {
  namespace GL {
    namespace Shader {

      void print_log (const std::string& type, GLhandleARB obj);

      class Program;

      template <GLint TYPE> class Object 
      {
        public:
          Object () : index_ (0) { }
          ~Object () { if (index_) glDeleteObjectARB (index_); }
          operator GLhandleARB () const { return (index_); }
          void compile (const std::string& source) 
          { 
            if (!index_) index_ = glCreateShaderObjectARB (TYPE);
            const char* p = source.c_str();
            glShaderSourceARB (index_, 1, &p, NULL);
            glCompileShaderARB (index_);
            GLint status;
            glGetObjectParameterivARB (index_, GL_OBJECT_COMPILE_STATUS_ARB, &status);
            if (status == 0) {
              debug();
              throw Exception (std::string ("error compiling ") + 
                  (TYPE == GL_VERTEX_SHADER_ARB ? "vertex shader" : "fragment shader"));
            }
                  

          }
          void debug () { 
            assert (index_);
            print_log (
                (TYPE == GL_VERTEX_SHADER_ARB ? "vertex shader" : "fragment shader"),
                index_); 
          }

        protected:
          GLhandleARB index_;
          friend class Program;
      };

      typedef Object<GL_VERTEX_SHADER_ARB> Vertex;
      typedef Object<GL_FRAGMENT_SHADER_ARB> Fragment;


      class Uniform 
      {
        public:
          float operator= (float value) { glUniform1fARB (index_, value); return (value); }
          int operator= (int value) { glUniform1iARB (index_, value); return (value); }

        protected:
          GLint index_;
          Uniform (GLint index) : index_ (index) { }
          friend class Program;
      };



      class Program 
      {
        public:
          Program () : index_ (0) { }
          ~Program () { if (index_) glDeleteObjectARB (index_); }
          operator GLhandleARB () const { return (index_); }
          template <GLint TYPE> void attach (const Object<TYPE>& object) 
          {
            if (!index_) index_ = glCreateProgramObjectARB(); 
            glAttachObjectARB (index_, object.index_); 
          }
          template <GLint TYPE> void detach (const Object<TYPE>& object) 
          {
            assert (index_);
            assert (object.index_);
            glDetachObjectARB (index_, object.index_); 
          }
          void link () 
          {
            assert (index_);
            glLinkProgramARB (index_); 
            GLint status;
            glGetObjectParameterivARB (index_, GL_OBJECT_LINK_STATUS_ARB, &status);
            if (status == 0) {
              debug();
              throw Exception (std::string ("error linking shader program"));
            }
          }
          void use () { assert (index_); glUseProgramObjectARB (index_); }
          static void stop () { glUseProgramObjectARB (0); }

          Uniform get_uniform (const std::string& name)
          { return (Uniform (glGetUniformLocationARB (index_, name.c_str()))); }
          void debug () { assert (index_); print_log ("shader program", index_); }

        protected:
          GLhandleARB index_;
      };

    }
  }
}

#endif

