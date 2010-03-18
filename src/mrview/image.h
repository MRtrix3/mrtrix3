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

#include "opengl/gl.h"
#include "image/voxel.h"
#include "dataset/interp/linear.h"

class QAction;

namespace MR {
  namespace Viewer {

    class Window;

    class Image : public QAction
    {
      Q_OBJECT

      public:
        Image (Window& parent, const MR::Image::Header* header);
        ~Image ();

        void reset_windowing ();

        void render2D (int projection, int slice);
        void get_axes (int projection, int& x, int& y) { 
          if (projection) {
            if (projection == 1) { x = 0; y = 2; }
            else { x = 0; y = 1; }
          }
          else { x = 1; y = 2; }
        }

        const MR::Image::Header& H;
        MR::Image::Voxel<float> vox;
        MR::DataSet::Interp::Linear<MR::Image::Voxel<float> > interp;

      signals:
        void ready ();

      private:
        Window& window;
        GLuint texture2D[3];
        int slice_position[3], interpolation;
        float value_min, value_max;
        float display_min, display_max;

        void update_texture2D (int projection, int slice);

        friend class Window;
    };

  }
}

#endif

