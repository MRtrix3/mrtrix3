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

#include "mrview/texture.h"

namespace MR {
  namespace Viewer {

    void Texture::allocate (int new_size)
    {
      int tsize = 64;
      while (tsize < new_size) tsize *= 2;
      if (tsize == size) return;

      size = tsize;

      if (data_size < size) {
        if (data) delete [] data;
        data_size = size;
        data = new GLubyte [data_size*data_size*( RGBA ? 4 : 1 )];
      }

      if (id == 0) glGenTextures (1, &id);
      glBindTexture (GL_TEXTURE_2D, id);
      glTexImage2D (GL_TEXTURE_2D, 0, (RGBA ? GL_RGBA : GL_ALPHA), size, size, 0, (RGBA ? GL_RGBA : GL_ALPHA), GL_UNSIGNED_BYTE, data);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    }



    void Texture::dump () const
    {
      for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++)
          if (RGBA) std::cout << "[ " << (int) (data[4*(x+size*y)]) << " "  << (int) (data[4*(x+size*y)+1]) << " " << (int) (data[4*(x+size*y)+2]) << " " << (int) (data[4*(x+size*y)+3]) << " ] ";
          else std::cout << (int) data[x+size*y] << " ";
        std::cout << "\n";
      }
    }


  }
}


