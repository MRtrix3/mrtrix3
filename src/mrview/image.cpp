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

#include <QMenu>

#include "mrview/image.h"
#include "mrview/window.h"

namespace MR {
  namespace Viewer {

    Image::Image (Window& parent, MR::Image::Header* header) :
      QAction (header->name().c_str(), &parent),
      window (parent),
      H (header) 
    {
      assert (header);
      setCheckable (true);
      window.image_group->addAction (this);
      window.image_menu->addAction (this);
    }

    Image::~Image () { }

    void Image::reset_windowing () { TEST; }
  }
}



