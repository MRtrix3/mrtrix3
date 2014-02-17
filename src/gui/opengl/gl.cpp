#include "gui/opengl/gl.h"
#include "file/config.h"

namespace MR
{
  namespace GUI
  {
    namespace GL
    {

      void init ()
      {
        INFO ("GL renderer:  " + std::string ( (const char*) gl::GetString (gl::RENDERER)));
        INFO ("GL version:   " + std::string ( (const char*) gl::GetString (gl::VERSION)));
        INFO ("GL vendor:    " + std::string ( (const char*) gl::GetString (gl::VENDOR)));
        GLint gl_version, gl_version_major;
        gl::GetIntegerv (gl::MAJOR_VERSION, &gl_version_major);
        gl::GetIntegerv (gl::MINOR_VERSION, &gl_version);
        gl_version += 10*gl_version_major;
        if (gl_version < 33)
          FAIL ("your OpenGL implementation is not sufficient to run MRView - need version 3.3 or higher");
/*
        GLenum status = gl::CheckFramebufferStatus (gl::FRAMEBUFFER);
        switch (status) {
          case gl::FRAMEBUFFER_UNDEFINED: throw Exception ("default framebuffer does not exist!");
          case gl::FRAMEBUFFER_INCOMPLETE_ATTACHMENT: throw Exception ("default framebuffer is incomplete!");
          case gl::FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT: throw Exception ("default framebuffer has no image attached!");
          case gl::FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER: throw Exception ("default framebuffer has incomplete draw buffer!");
          case gl::FRAMEBUFFER_UNSUPPORTED: throw Exception ("default framebuffer does not support requested format!");
          case gl::FRAMEBUFFER_COMPLETE: break;
          default: throw Exception ("default framebuffer cannot be used for unknown reason!");
        }
        */
      }

      const char* ErrorString (GLenum errorcode) 
      {
        switch (errorcode) {
          case gl::INVALID_ENUM: return "invalid value for enumerated argument";
          case gl::INVALID_VALUE: return "value out of range";
          case gl::INVALID_OPERATION: return "operation not allowed given current state";
          case gl::OUT_OF_MEMORY: return "insufficient memory";
          case gl::INVALID_FRAMEBUFFER_OPERATION: return "invalid framebuffer operation";
          default: return "unknown error";
        }
      }





      QGLFormat core_format () {
        QGLFormat f (QGL::DoubleBuffer | QGL::DepthBuffer | QGL::Rgba);
        int swap_interval = MR::File::Config::get_int ("VSync", 0);
        f.setSwapInterval (swap_interval);
#ifdef MRTRIX_MACOSX
        f.setVersion (3,3);
        f.setProfile (QGLFormat::CoreProfile);
#endif
        int nsamples = File::Config::get_int ("MSAA", 0);
        if (nsamples > 1) {
          f.setSampleBuffers (true);
          f.setSamples (nsamples);
        }
        return f;
      }

    }
  }
}



