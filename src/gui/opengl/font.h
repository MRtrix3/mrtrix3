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


