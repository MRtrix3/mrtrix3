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
              bind();
              gl::TexParameteri (tex_type, gl::TEXTURE_BASE_LEVEL, 0);
              gl::TexParameteri (tex_type, gl::TEXTURE_MAX_LEVEL, 0);
              gl::TexParameteri (tex_type, gl::TEXTURE_MAG_FILTER, gl::LINEAR);
              gl::TexParameteri (tex_type, gl::TEXTURE_MIN_FILTER, gl::LINEAR);
              //gl::TexParameteri (tex_type, gl::TEXTURE_WRAP_S, gl::CLAMP);
              //gl::TexParameteri (tex_type, gl::TEXTURE_WRAP_T, gl::CLAMP);
              //if (tex_type == gl::TEXTURE_3D)
                //gl::TexParameteri (tex_type, gl::TEXTURE_WRAP_R, gl::CLAMP);
            }
          }
          GLenum type () const { return tex_type; }
          void clear () { if (id) gl::DeleteTextures (1, &id); id = 0; }
          void bind () const { assert (id); gl::BindTexture (tex_type, id); }
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
          void gen () { if (!id) gl::GenBuffers (1, &id); }
          void clear () { if (id) gl::DeleteBuffers (1, &id); id = 0; }
          void bind (GLenum target) const { assert (id); gl::BindBuffer (target, id); }
        protected:
          GLuint id;
      };


      class VertexArrayObject {
        public:
          VertexArrayObject () : id (0) { }
          ~VertexArrayObject () { clear(); }
          operator GLuint () const { return id; }
          void gen () { if (!id) gl::GenVertexArrays (1, &id); }
          void clear () { if (id) gl::DeleteVertexArrays (1, &id); id = 0; }
          void bind () const { assert (id); gl::BindVertexArray (id); }
        protected:
          GLuint id;
      };


      QGLFormat core_format ();

    }
  }
}



#endif

