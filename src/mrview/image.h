/*
   Copyright 2009 Brain Research Institute, Melbourne, Australia

   Written by J-Donald Tournier, 13/11/09.

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

#ifndef __viewer_image_h__
#define __viewer_image_h__

#include <QAction>

#include "image/voxel.h"

class QAction;

namespace MR {
  namespace Viewer {

    class Window;

    class Image : public QAction
    {
      public:
        Image (Window& parent, MR::Image::Header* header);
        ~Image ();

        void reset_windowing ();

      private:
        Window& window;
        Ptr<MR::Image::Header> H;

        friend class Window;
    };

  }
}

#endif

