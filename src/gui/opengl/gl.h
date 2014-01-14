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
using std::std::isfinite;
#else
#include "gui/opengl/gl_core_3_3.h"
#include <QtGui>
#endif
#include <QGLWidget>

// necessary to avoid conflict with Qt4's macros:
#ifdef Complex
# undef Complex
#endif
#ifdef foreach
# undef foreach
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
              glGenTextures (1, &id);
              bind();
              glTexParameteri (tex_type, GL_TEXTURE_BASE_LEVEL, 0);
              glTexParameteri (tex_type, GL_TEXTURE_MAX_LEVEL, 0);
              glTexParameteri (tex_type, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
              glTexParameteri (tex_type, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
              //glTexParameteri (tex_type, GL_TEXTURE_WRAP_S, GL_CLAMP);
              //glTexParameteri (tex_type, GL_TEXTURE_WRAP_T, GL_CLAMP);
              //if (tex_type == GL_TEXTURE_3D)
                //glTexParameteri (tex_type, GL_TEXTURE_WRAP_R, GL_CLAMP);
            }
          }
          GLenum type () const { return tex_type; }
          void clear () { if (id) glDeleteTextures (1, &id); id = 0; }
          void bind () const { assert (id); glBindTexture (tex_type, id); }
          void set_interp (GLint type) const {
            bind();
            glTexParameteri (tex_type, GL_TEXTURE_MAG_FILTER, type);
            glTexParameteri (tex_type, GL_TEXTURE_MIN_FILTER, type);
          }
          void set_interp (bool interpolate) const { set_interp (interpolate ? GL_LINEAR : GL_NEAREST); }
        protected:
          GLuint id;
          GLenum tex_type;
      };


      class VertexBuffer {
        public:
          VertexBuffer () : id (0) { }
          ~VertexBuffer () { clear(); }
          operator GLuint () const { return id; }
          void gen () { if (!id) glGenBuffers (1, &id); }
          void clear () { if (id) glDeleteBuffers (1, &id); id = 0; }
          void bind (GLenum target) const { assert (id); glBindBuffer (target, id); }
        protected:
          GLuint id;
      };


      class VertexArrayObject {
        public:
          VertexArrayObject () : id (0) { }
          ~VertexArrayObject () { clear(); }
          operator GLuint () const { return id; }
          void gen () { if (!id) glGenVertexArrays (1, &id); }
          void clear () { if (id) glDeleteVertexArrays (1, &id); id = 0; }
          void bind () const { assert (id); glBindVertexArray (id); }
        protected:
          GLuint id;
      };

    }
  }
}



#endif

