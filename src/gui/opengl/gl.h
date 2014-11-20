/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 27/06/08.

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

#ifndef __gui_opengl_gl_h__
#define __gui_opengl_gl_h__

#include "mrtrix.h"
#include "debug.h"

#include <QtGlobal>
#if QT_VERSION >= 0x050000
#include <QtWidgets>
#else
#include <QtGui>
#endif
#include <QGLWidget>
#include "gui/opengl/gl_core_3_3.h"

// necessary to avoid conflict with Qt4's macros:
#ifdef Complex
# undef Complex
#endif
#ifdef foreach
# undef foreach
#endif

#ifdef GL_DEBUG
# undef GL_DEBUG
# define GL_DEBUG(msg) DEBUG(msg)
#else
# define GL_DEBUG(msg) (void)0
#endif

namespace MR
{
  namespace GUI
  {
    namespace GL
    {

      void init ();

      const char* ErrorString (GLenum errorcode);


      class Texture {
        public:
          Texture () : id (0) { }
          ~Texture () { clear(); }
          operator GLuint () const { return id; }
          void gen (GLenum target) { 
            if (!id) {
              tex_type = target;
              gl::GenTextures (1, &id);
              GL_DEBUG ("created OpenGL texture ID " + str(id));
              bind();
              gl::TexParameteri (tex_type, gl::TEXTURE_BASE_LEVEL, 0);
              gl::TexParameteri (tex_type, gl::TEXTURE_MAX_LEVEL, 0);
              gl::TexParameteri (tex_type, gl::TEXTURE_MAG_FILTER, gl::LINEAR);
              gl::TexParameteri (tex_type, gl::TEXTURE_MIN_FILTER, gl::LINEAR);
              gl::TexParameteri (tex_type, gl::TEXTURE_WRAP_S, gl::CLAMP_TO_EDGE);
              gl::TexParameteri (tex_type, gl::TEXTURE_WRAP_T, gl::CLAMP_TO_EDGE);
              if (tex_type == gl::TEXTURE_3D)
                gl::TexParameteri (tex_type, gl::TEXTURE_WRAP_R, gl::CLAMP_TO_EDGE);
            }
          }
          GLenum type () const { return tex_type; }
          void clear () { 
            if (id) {
              GL_DEBUG ("deleting OpenGL texture ID " + str(id));
              gl::DeleteTextures (1, &id); 
            }
            id = 0;
          }
          void bind () const {
            assert (id); 
            GL_DEBUG ("binding OpenGL texture ID " + str(id));
            gl::BindTexture (tex_type, id); 
          }
          void set_interp (GLint type) const {
            bind();
            gl::TexParameteri (tex_type, gl::TEXTURE_MAG_FILTER, type);
            gl::TexParameteri (tex_type, gl::TEXTURE_MIN_FILTER, type);
          }
          void set_interp_on (bool interpolate) const { set_interp (interpolate ? gl::LINEAR : gl::NEAREST); }
        protected:
          GLuint id;
          GLenum tex_type;
      };


      class VertexBuffer {
        public:
          VertexBuffer () : id (0) { }
          ~VertexBuffer () { clear(); }
          operator GLuint () const { return id; }
          void gen () { 
            if (!id) {
              gl::GenBuffers (1, &id); 
              GL_DEBUG ("created OpenGL vertex buffer ID " + str(id));
            }
          }
          void clear () { 
            if (id) {
              GL_DEBUG ("deleting OpenGL vertex buffer ID " + str(id));
              gl::DeleteBuffers (1, &id); 
              id = 0; 
            }
          }
          void bind (GLenum target) const {
            assert (id); 
            GL_DEBUG ("binding OpenGL vertex buffer ID " + str(id));
            gl::BindBuffer (target, id); 
          }
        protected:
          GLuint id;
      };


      class VertexArrayObject {
        public:
          VertexArrayObject () : id (0) { }
          ~VertexArrayObject () { clear(); }
          operator GLuint () const { return id; }
          void gen () {
            if (!id) {
              gl::GenVertexArrays (1, &id); 
              GL_DEBUG ("created OpenGL vertex array ID " + str(id));
            }
          }
          void clear () { 
            if (id) {
              GL_DEBUG ("deleting OpenGL vertex array ID " + str(id));
              gl::DeleteVertexArrays (1, &id); id = 0; 
            }
          }
          void bind () const { 
            assert (id); 
            GL_DEBUG ("binding OpenGL vertex array ID " + str(id));
            gl::BindVertexArray (id);
          }
        protected:
          GLuint id;
      };


      QGLFormat core_format ();

    }
  }
}



#endif

