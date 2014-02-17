#ifndef __gui_opengl_font_h__
#define __gui_opengl_font_h__

#include "gui/opengl/shader.h"

namespace MR
{
  namespace GUI
  {
    namespace GL
    {

      class Font {
        public:
          Font (const QFont& font) :
            metric (font),
            font (font) { } 

          void initGL ();

          const QFontMetrics metric;

          void start (int width, int height, float red, float green, float blue) const {
            assert (program);
            gl::Disable (gl::DEPTH_TEST);
            gl::DepthMask (gl::FALSE_);
            gl::Enable (gl::BLEND);
            gl::BlendEquation (gl::FUNC_ADD);
            gl::BlendFunc (gl::SRC_ALPHA, gl::ONE_MINUS_SRC_ALPHA);
            program.start();
            gl::Uniform1f (gl::GetUniformLocation (program, "scale_x"), 2.0 / width);
            gl::Uniform1f (gl::GetUniformLocation (program, "scale_y"), 2.0 / height);
            gl::Uniform1f (gl::GetUniformLocation (program, "red"), red);
            gl::Uniform1f (gl::GetUniformLocation (program, "green"), green);
            gl::Uniform1f (gl::GetUniformLocation (program, "blue"), blue);
          }

          void stop () const {
            program.stop();
            gl::DepthMask (gl::TRUE_);
            gl::Disable (gl::BLEND);
          }

          void render (const std::string& text, int x, int y) const;

        protected:
          const QFont& font;
          GL::Texture tex;
          GL::VertexBuffer vertex_buffer[2];
          GL::VertexArrayObject vertex_array_object;
          GL::Shader::Program program;
          int font_width[256], font_height;
          float font_tex_pos[256], font_tex_width[256];
      };


    }
  }
}

#endif


