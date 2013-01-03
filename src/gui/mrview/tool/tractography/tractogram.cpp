/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier & David Raffelt 17/12/12

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

#include <QMenu>

#include "progressbar.h"
#include "image/stride.h"
#include "gui/mrview/tool/tractography/tractogram.h"
#include "gui/mrview/window.h"
#include "gui/projection.h"



namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {

        Tractogram::Tractogram (const std::string& filename) :
          QAction (NULL),
          filename (filename),
          interpolation (GL_LINEAR),
          value_min (NAN),
          value_max (NAN)
        {
          file.open (filename, properties);
        }



        Tractogram::Tractogram (Window& window, const std::string& filename) :
          QAction (shorten (filename, 20, 0).c_str(), &window),
          filename (filename),
          interpolation (GL_LINEAR),
          value_min (NAN),
          value_max (NAN)
        {
          file.open (filename, properties);
        }


        Tractogram::~Tractogram ()
        {
  //        glDeleteTextures (3, texture2D);
  //        glDeleteTextures (1, &texture3D);
          file.close();
        }



        void Tractogram::render ()
        {


        }



        void load_tracks() {

//          std::vector<Point<float> > tck;
//
//          size_t index = 0;
//          while (file.next (tck)) {
//
//
//          }

        }

      }
    }
  }
}


