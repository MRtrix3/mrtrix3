/*
   Copyright 2013 Brain Research Institute, Melbourne, Australia

   Written by David Raffelt, 15/01/13.

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

#include "gui/mrview/shader.h"

#ifndef __gui_mrview_tools_tractography_shader_h__
#define __gui_mrview_tools_tractography_shader_h__

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {

        class Shader : public MRView::Shader
        {
          public:
            void set_crop_to_slab (bool crop_to_slab) {
              do_crop_to_slab = crop_to_slab;
              recompile();
            }
          protected:
            virtual void recompile ();

            bool do_crop_to_slab;

        };

      }
    }
  }
}

#endif
