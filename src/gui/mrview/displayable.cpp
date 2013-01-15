/*
   Copyright 2012 Brain Research Institute, Melbourne, Australia

   Written by J-Donald Tournier and David Raffelt, 07/12/12.

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

#include "gui/mrview/displayable.h"
#include "progressbar.h"
#include "image/stride.h"
#include "gui/mrview/image.h"
#include "gui/mrview/window.h"
#include "gui/projection.h"



namespace MR
{
  namespace GUI
  {
    namespace MRView
    {

      Displayable::Displayable (const std::string& filename) :
        QAction (NULL),
        interpolation (GL_LINEAR),
        value_min (NAN),
        value_max (NAN),
        texture_mode_3D_unchanged (false),
        filename (filename) { }


      Displayable::Displayable (Window& window, const std::string& filename) :
        QAction (shorten (filename, 20, 0).c_str(), &window),
        interpolation (GL_LINEAR),
        value_min (NAN),
        value_max (NAN),
        texture_mode_3D_unchanged (false),
        filename (filename) {
          connect (this, SIGNAL(scalingChanged()), &window, SLOT(on_scaling_changed()));
      }


      Displayable::~Displayable ()
      {
      }


    }
  }
}


