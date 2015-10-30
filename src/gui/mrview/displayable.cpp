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

#include "gui/mrview/displayable.h"



namespace MR
{
  namespace GUI
  {
    namespace MRView
    {

      Displayable::Displayable (const std::string& filename) :
        QAction (nullptr),
        lessthan (NAN),
        greaterthan (NAN),
        display_midpoint (NAN),
        display_range (NAN),
        transparent_intensity (NAN),
        opaque_intensity (NAN),
        alpha (NAN),
        colourmap (0),
        show (true),
        show_colour_bar (true),
        filename (filename),
        value_min (NAN),
        value_max (NAN),
        flags_ (0x00000000) { 
          colour[0] = colour[1] = 255;
          colour[2] = 0;
        }



      Displayable::~Displayable ()
      {
      }



      bool Displayable::Shader::need_update (const Displayable& object) const 
      { 
        return flags != object.flags() || colourmap != object.colourmap;
      }


      void Displayable::Shader::update (const Displayable& object) 
      {
        flags = object.flags();
        colourmap = object.colourmap;
      }

    }
  }
}


