/* Copyright (c) 2008-2017 the MRtrix3 contributors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
 */


#include "gui/opengl/gl.h"
#include "file/config.h"

namespace MR
{
  namespace GUI
  {
    namespace GL
    {


      void set_default_context () {
        //CONF option: VSync
        //CONF default: 0 (false)
        //CONF Whether the screen update should synchronise with the monitor's
        //CONF vertical refresh (to avoid tearing artefacts).
        
        //CONF option: NeedOpenGLCoreProfile
        //CONF default: 1 (true)
        //CONF Whether the creation of an OpenGL 3.3 context requires it to be
        //CONF a core profile (needed on newer versions of the ATI drivers on
        //CONF Linux, for instance).

        //CONF option: MSAA
        //CONF default: 0 (false)
        //CONF How many samples to use for multi-sample anti-aliasing (to
        //CONF improve display quality).

        GL::Format f;
#if QT_VERSION >= 0x050400
        f.setSwapBehavior (GL::Format::DoubleBuffer);
        f.setRenderableType (GL::Format::OpenGL);
#else
        f.setDoubleBuffer (true);
#endif

        if (File::Config::get_bool ("NeedOpenGLCoreProfile", true)) {
          f.setVersion (3,3);
          f.setProfile (GL::Format::CoreProfile);
        }
        
        f.setDepthBufferSize (24);
        f.setRedBufferSize (8);
        f.setGreenBufferSize (8);
        f.setBlueBufferSize (8);
        f.setAlphaBufferSize (0);

        int swap_interval = MR::File::Config::get_int ("VSync", 0);
        f.setSwapInterval (swap_interval);

        int nsamples = File::Config::get_int ("MSAA", 0);
        if (nsamples > 1) 
          f.setSamples (nsamples);

        GL::Format::setDefaultFormat (f);
      }






      void init ()
      {
        INFO ("GL renderer:  " + std::string ( (const char*) gl::GetString (gl::RENDERER)));
        INFO ("GL version:   " + std::string ( (const char*) gl::GetString (gl::VERSION)));
        INFO ("GL vendor:    " + std::string ( (const char*) gl::GetString (gl::VENDOR)));
        
        GLint gl_version (0), gl_version_major (0);
        gl::GetIntegerv (gl::MAJOR_VERSION, &gl_version_major);
        gl::GetIntegerv (gl::MINOR_VERSION, &gl_version);
        GL_CHECK_ERROR;

        gl_version += 10*gl_version_major;
        if (gl_version == 0) {
          WARN ("unable to determine OpenGL version - operation may be unstable if actual version is less than 3.3");
        }
        else if (gl_version < 33) {
          FAIL ("your OpenGL implementation is not sufficient to run MRView - need version 3.3 or higher");
          FAIL ("    operation is likely to be unstable");
        }
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





    }
  }
}



