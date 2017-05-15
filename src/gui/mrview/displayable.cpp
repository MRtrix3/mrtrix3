/* Copyright (c) 2008-2017 the MRtrix3 contributors.
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


