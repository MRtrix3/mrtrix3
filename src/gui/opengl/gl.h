/* Copyright (c) 2008-2023 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Covered Software is provided under this License on an "as is"
 * basis, without warranty of any kind, either expressed, implied, or
 * statutory, including, without limitation, warranties that the
 * Covered Software is free of defects, merchantable, fit for a
 * particular purpose or non-infringing.
 * See the Mozilla Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
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

// uncomment to trace texture/VAO/VBO/FBO operations:
//#define GL_SHOW_DEBUG_MESSAGE

#ifdef GL_SHOW_DEBUG_MESSAGE
# define GL_DEBUG(msg) DEBUG(msg)
#else
# define GL_DEBUG(msg) (void)0
#endif

#ifdef NDEBUG
# define GL_CHECK_ERROR
#else
# define GL_CHECK_ERROR ::MR::GUI::GL::check_error (__FILE__, __LINE__)
#endif


#define GLGETBOOL(x,n) { GLboolean __v[n]; gl::GetBooleanv (x, __v); std::cerr << #x " = "; for (auto i : __v) std::cerr << int(i) << " "; std::cerr << "\n"; }
#define GLGETINT(x,n) { GLint __v[n]; gl::GetIntegerv (x, __v); std::cerr << #x " = "; for (auto i : __v) std::cerr << int(i) << " "; std::cerr << "\n"; }

namespace MR
{
  namespace GUI
  {
    namespace GL
    {



#if QT_VERSION >= 0x050400

      using Area = QOpenGLWidget;
      using Format = QSurfaceFormat;

#else
      class Area : public QGLWidget { NOMEMALIGN
        public:
          using QGLWidget::QGLWidget;
          QImage grabFramebuffer () { return QGLWidget::grabFrameBuffer(); }
      };

      using Format = QGLFormat;
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



#ifndef NDEBUG
      void __assert_context_is_current (QWidget* glarea);
      inline void assert_context_is_current(QWidget* glarea = nullptr) { __assert_context_is_current (glarea); }
#else
      inline void assert_context_is_current(QWidget* = nullptr) { }
#endif

      extern Area* glwidget;


      namespace Context
      {
#if QT_VERSION >= 0x050400
        inline std::pair<QOpenGLContext*,QSurface*> current() {
          QOpenGLContext* context = QOpenGLContext::currentContext();
          QSurface* surface = context ? context->surface() : nullptr;
          return { context, surface };
        }

        inline std::pair<QOpenGLContext*,QSurface*> get (QWidget* window) {
          QOpenGLContext* context = reinterpret_cast<QOpenGLWidget*> (window)->context();
          QSurface* surface = context ? context->surface() : nullptr;
          return { context, surface };
        }

        inline std::pair<QOpenGLContext*,QSurface*> makeCurrent (QWidget* window) {
          auto previous_context = current();
          if (window)
            reinterpret_cast<QOpenGLWidget*> (window)->makeCurrent();
          return previous_context;
        }

        inline void restore (std::pair<QOpenGLContext*,QSurface*> previous_context) {
          if (previous_context.first)
            previous_context.first->makeCurrent (previous_context.second);
        }
#else
        inline std::pair<int,int> current() { return { 0, 0 }; }
        inline std::pair<int,int> get (QWidget*) { return { 0, 0 }; }
        inline std::pair<int,int> makeCurrent (QWidget*) { return { 0, 0 }; }
        inline void restore (std::pair<int,int>) { }
#endif

        struct Grab { NOMEMALIGN
          decltype (current()) previous_context;
          Grab (QWidget* window = nullptr) : previous_context (makeCurrent (window ? window : GL::glwidget)) {
            assert_context_is_current (window);
          }
          ~Grab () { restore (previous_context); }
        };

#ifndef NDEBUG
        struct Checker { NOMEMALIGN
          decltype (current()) original_context;
          void set () { original_context = current(); }
          void operator() () const { assert (current() == original_context); }
        };
#else
        struct Checker { NOMEMALIGN
          void set () { }
          void operator() () const { }
        };
#endif
      }



      class Texture { NOMEMALIGN
        public:
          Texture () : id (0), tex_type (0) { }
          ~Texture () { clear(); }
          Texture (const Texture&) : id (0), tex_type (0) { }
          Texture (Texture&& t) : id (t.id), tex_type (t.tex_type) { t.id = t.tex_type = 0; }
          Texture& operator= (Texture&& t) { clear(); id = t.id; tex_type = t.tex_type; t.id = t.tex_type = 0; return *this; }
          void cache_copy(const Texture& t) { id = t.id; tex_type = t.tex_type; }
          operator GLuint () const { return id; }
          void gen (GLenum target, GLint interp_type = gl::LINEAR) {
            if (!id) {
              check_context.set();
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
              check_context();
              GL_DEBUG ("deleting OpenGL texture ID " + str(id));
              gl::DeleteTextures (1, &id);
            }
            id = 0;
            tex_type = 0;
          }
          void bind () const {
            assert (id);
            check_context();
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
          Context::Checker check_context;
          GLuint id;
          GLenum tex_type;
      };


      class VertexBuffer { NOMEMALIGN
        public:
          VertexBuffer () : id (0) { }
          ~VertexBuffer () { clear(); }
          VertexBuffer (const VertexBuffer&) : id (0) { }
          VertexBuffer (VertexBuffer&& t) : id (t.id) { t.id = 0; }
          VertexBuffer& operator= (VertexBuffer&& t) { clear(); id = t.id; t.id = 0; return *this; }
          operator GLuint () const { return id; }
          void gen () {
            if (!id) {
              check_context.set();
              gl::GenBuffers (1, &id);
              GL_DEBUG ("created OpenGL vertex buffer ID " + str(id));
            }
          }
          void clear () {
            if (id) {
              check_context();
              GL_DEBUG ("deleting OpenGL vertex buffer ID " + str(id));
              gl::DeleteBuffers (1, &id);
              id = 0;
            }
          }
          void bind (GLenum target) const {
            assert (id);
            check_context();
            GL_DEBUG ("binding OpenGL vertex buffer ID " + str(id));
            gl::BindBuffer (target, id);
          }
        protected:
          Context::Checker check_context;
          GLuint id;
      };


      class VertexArrayObject { NOMEMALIGN
        public:
          VertexArrayObject () : id (0) { }
          ~VertexArrayObject () { clear(); }
          VertexArrayObject (const VertexArrayObject&) : id (0) { }
          VertexArrayObject (VertexArrayObject&& t) : id (t.id) { t.id = 0; }
          VertexArrayObject& operator= (VertexArrayObject&& t) { clear(); id = t.id; t.id = 0; return *this; }
          operator GLuint () const { return id; }
          void gen () {
            if (!id) {
              check_context.set();
              gl::GenVertexArrays (1, &id);
              GL_DEBUG ("created OpenGL vertex array ID " + str(id));
            }
          }
          void clear () {
            if (id) {
              check_context();
              GL_DEBUG ("deleting OpenGL vertex array ID " + str(id));
              gl::DeleteVertexArrays (1, &id); id = 0;
            }
          }
          void bind () const {
            assert (id);
            check_context();
            GL_DEBUG ("binding OpenGL vertex array ID " + str(id));
            gl::BindVertexArray (id);
          }
        protected:
          Context::Checker check_context;
          GLuint id;
      };


      class IndexBuffer { NOMEMALIGN
        public:
          IndexBuffer () : id (0) { }
          ~IndexBuffer () { clear(); }
          IndexBuffer (const IndexBuffer&) : id (0) { }
          IndexBuffer (IndexBuffer&& t) : id (t.id) { t.id = 0; }
          IndexBuffer& operator= (IndexBuffer&& t) { clear(); id = t.id; t.id = 0; return *this; }
          operator GLuint () const { return id; }
          void gen () {
            if (!id) {
              check_context.set();
              gl::GenBuffers (1, &id);
              GL_DEBUG ("created OpenGL index buffer ID " + str(id));
            }
          }
          void clear () {
            if (id) {
              check_context();
              GL_DEBUG ("deleting OpenGL index buffer ID " + str(id));
              gl::DeleteBuffers (1, &id);
              id = 0;
            }
          }
          void bind () const {
            assert (id);
            check_context();
            GL_DEBUG ("binding OpenGL index buffer ID " + str(id));
            gl::BindBuffer (gl::ELEMENT_ARRAY_BUFFER, id);
          }
        protected:
          Context::Checker check_context;
          GLuint id;
      };



      class FrameBuffer { NOMEMALIGN
        public:
          FrameBuffer () : id (0) { }
          ~FrameBuffer () { clear(); }
          FrameBuffer (const FrameBuffer&) : id (0) { }
          FrameBuffer (FrameBuffer&& t) : id (t.id) { t.id = 0; }
          FrameBuffer& operator= (FrameBuffer&& t) { clear(); id = t.id; t.id = 0; return *this; }
          operator GLuint () const { return id; }
          void gen () {
            if (!id) {
              check_context.set();
              gl::GenFramebuffers (1, &id);
              GL_DEBUG ("created OpenGL framebuffer ID " + str(id));
            }
          }
          void clear () {
            if (id) {
              check_context();
              GL_DEBUG ("deleting OpenGL framebuffer ID " + str(id));
              gl::DeleteFramebuffers (1, &id);
              unbind();
            }
            id = 0;
          }
          void bind () const {
            assert (id);
            check_context();
            GL_DEBUG ("binding OpenGL framebuffer ID " + str(id));
            gl::BindFramebuffer (gl::FRAMEBUFFER, id);
          }
          void unbind () const {
            check_context();
            GL_DEBUG ("binding default OpenGL framebuffer");
#if QT_VERSION >= 0x050400
            gl::BindFramebuffer (gl::FRAMEBUFFER, QOpenGLContext::currentContext()->defaultFramebufferObject());
#else
            gl::BindFramebuffer (gl::FRAMEBUFFER, 0);
#endif
          }


          void attach_color (Texture& tex, size_t attachment) const {
            assert (tex);
            bind();
            GL_DEBUG ("texture ID " + str (tex) + " attached to framebuffer ID " + str(id) + " at color attachement " + str(attachment));
            gl::FramebufferTexture (gl::FRAMEBUFFER, GLenum (size_t (gl::COLOR_ATTACHMENT0) + attachment), tex, 0);
          }
          void draw_buffers (size_t first) const {
            check_context();
            GLenum list[1] = { GLenum (size_t (gl::COLOR_ATTACHMENT0) + first) };
            gl::DrawBuffers (1 , list);
          }
          void draw_buffers (size_t first, size_t second) const {
            check_context();
            GLenum list[2] = { GLenum (size_t(gl::COLOR_ATTACHMENT0) + first), GLenum (size_t (gl::COLOR_ATTACHMENT0) + second) };
            gl::DrawBuffers (2 , list);
          }

          void check() const {
            check_context();
            if (gl::CheckFramebufferStatus (gl::FRAMEBUFFER) != gl::FRAMEBUFFER_COMPLETE)
              throw Exception ("FIXME: framebuffer is not complete");
          }
        protected:
          Context::Checker check_context;
          GLuint id;
      };



    }
  }
}



#endif

