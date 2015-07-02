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

#ifdef NDEBUG
# define GL_CHECK_ERROR
# define GL_DEBUG(msg) (void)0
#else 
# define GL_CHECK_ERROR ::MR::GUI::GL::check_error (__FILE__, __LINE__)
# define GL_DEBUG(msg) DEBUG(msg)
#endif

namespace MR
{
  namespace GUI
  {
    namespace GL
    {

#if QT_VERSION >= 0x050400
      typedef QOpenGLWidget Area;
      typedef QSurfaceFormat Format;
#else
      class Area : public QGLWidget {
        public:
          using QGLWidget::QGLWidget;
          QImage grabFramebuffer () { return QGLWidget::grabFrameBuffer(); }
      };
      typedef QGLFormat Format;
#endif
 
      void init ();
      void set_default_context ();

      const char* ErrorString (GLenum errorcode);

      inline void check_error (const char* filename, int line) {
        GLenum err = gl::GetError();
        while (err) {
          FAIL (std::string ("[") + filename + ": " + str(line) + "] OpenGL error: " + ErrorString (err));
          err = gl::GetError();
        }
      }


      class Texture {
        public:
          Texture () : id (0) { }
          ~Texture () { clear(); }
          Texture (const Texture&) : id (0) { }
          Texture (Texture&& t) : id (t.id) { t.id = 0; }
          Texture& operator= (Texture&& t) { clear(); id = t.id; t.id = 0; return *this; }
          operator GLuint () const { return id; }
          void gen (GLenum target, GLint interp_type = gl::LINEAR) {
            if (!id) {
              tex_type = target;
              gl::GenTextures (1, &id);
              GL_DEBUG ("created OpenGL texture ID " + str(id));
              bind();
              gl::TexParameteri (tex_type, gl::TEXTURE_BASE_LEVEL, 0);
              gl::TexParameteri (tex_type, gl::TEXTURE_MAX_LEVEL, 0);
              gl::TexParameteri (tex_type, gl::TEXTURE_MAG_FILTER, interp_type);
              gl::TexParameteri (tex_type, gl::TEXTURE_MIN_FILTER, interp_type);
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
          VertexBuffer (const VertexBuffer&) : id (0) { }
          VertexBuffer (VertexBuffer&& t) : id (t.id) { t.id = 0; }
          VertexBuffer& operator= (VertexBuffer&& t) { clear(); id = t.id; t.id = 0; return *this; }
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
          VertexArrayObject (const VertexArrayObject&) : id (0) { }
          VertexArrayObject (VertexArrayObject&& t) : id (t.id) { t.id = 0; }
          VertexArrayObject& operator= (VertexArrayObject&& t) { clear(); id = t.id; t.id = 0; return *this; }
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



      class FrameBuffer {
        public:
          FrameBuffer () : id (0) { }
          ~FrameBuffer () { clear(); }
          FrameBuffer (const FrameBuffer&) : id (0) { }
          FrameBuffer (FrameBuffer&& t) : id (t.id) { t.id = 0; }
          FrameBuffer& operator= (FrameBuffer&& t) { clear(); id = t.id; t.id = 0; return *this; }
          operator GLuint () const { return id; }
          void gen () { 
            if (!id) {
              gl::GenFramebuffers (1, &id);
              GL_DEBUG ("created OpenGL framebuffer ID " + str(id));
            }
          }
          void clear () { 
            if (id) {
              GL_DEBUG ("deleting OpenGL framebuffer ID " + str(id));
              gl::DeleteFramebuffers (1, &id); 
              unbind();
            }
            id = 0;
          }
          void bind () const {
            assert (id); 
            GL_DEBUG ("binding OpenGL framebuffer ID " + str(id));
            gl::BindFramebuffer (gl::FRAMEBUFFER, id); 
          }
          void unbind () const {
            GL_DEBUG ("binding default OpenGL framebuffer");
            gl::BindFramebuffer (gl::FRAMEBUFFER, 0); 
          }

          void attach_color (Texture& tex, size_t attachment) const {
            assert (id);
            assert (tex);
            bind();
            GL_DEBUG ("texture ID " + str (tex) + " attached to framebuffer ID " + str(id) + " at color attachement " + str(attachment));
            gl::FramebufferTexture (gl::FRAMEBUFFER, GLenum (size_t (gl::COLOR_ATTACHMENT0) + attachment), tex, 0);
          }
          void draw_buffers (size_t first) const {
            GLenum list[1] = { GLenum (size_t (gl::COLOR_ATTACHMENT0) + first) };
            gl::DrawBuffers (1 , list);
          }
          void draw_buffers (size_t first, size_t second) const {
            GLenum list[2] = { GLenum (size_t(gl::COLOR_ATTACHMENT0) + first), GLenum (size_t (gl::COLOR_ATTACHMENT0) + second) };
            gl::DrawBuffers (2 , list);
          }

          void check() const {
            if (gl::CheckFramebufferStatus (gl::FRAMEBUFFER) != gl::FRAMEBUFFER_COMPLETE)
              throw Exception ("FIXME: framebuffer is not complete");
          }
        protected:
          GLuint id;
      };



    }
  }
}



#endif

