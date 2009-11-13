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

#ifndef __mrview_texture_h__
#define __mrview_texture_h__

#include "use_gl.h"
#include <GL/gl.h>
#include "point.h"


namespace MR {
  namespace Viewer {

    class Texture {
      public:
        class TexEl {
          public:
            TexEl (GLubyte* data) : v (data) { }
            void I  (GLubyte intensity) { v[0] = v[1] = v[2] = intensity; v[3] = 255; }
            void IA (GLubyte intensity, GLubyte alpha) { v[0] = v[1] = v[2] = intensity; v[3] = alpha; }
            void RGB (GLubyte R, GLubyte G, GLubyte B) { v[0] = R; v[1] = G; v[2] = B; v[3] = 255; }
            void RGBA (GLubyte R, GLubyte G, GLubyte B, GLubyte alpha) { v[0] = R; v[1] = G; v[2] = B; v[3] = alpha; }
          protected:
            GLubyte* v;
        };



        Texture (bool is_RGBA) : data (NULL), size (0), data_size (0), id (0), RGBA (is_RGBA) { }
        ~Texture () { if (id > 0) glDeleteTextures (1, &id); if (data) delete [] data; }
        void dump () const;
        void allocate (int new_size);
        int  width () const { return (size); }
        TexEl rgba (int x, int y) { return (TexEl (data + 4*(x+size*y))); }
        GLubyte& alpha (int x, int y) { return (data[x+size*y]); }
        void clear () { memset (data, 0, size*size*(RGBA ? 4 : 1)); }
        void select () { glBindTexture (GL_TEXTURE_2D, id); }
        void commit ()
        {
          glPixelStorei (GL_UNPACK_ALIGNMENT, 1);
          glBindTexture (GL_TEXTURE_2D, id);
          glTexSubImage2D (GL_TEXTURE_2D, 0, 0, 0, size, size, ( RGBA ? GL_RGBA : GL_ALPHA ), GL_UNSIGNED_BYTE, data);
        }
        bool  is_rgba () const { return (RGBA); }



      private:
        GLubyte* data;
        int      size, data_size;
        GLuint   id;
        bool     RGBA;
    };

  }
}

#endif



