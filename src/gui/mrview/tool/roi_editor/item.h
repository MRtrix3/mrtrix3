/*
   Copyright 2009 Brain Research Institute, Melbourne, Australia

   Written by J-Donald Tournier, 2014.

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

#ifndef __gui_mrview_tool_roi_editor_item_h__
#define __gui_mrview_tool_roi_editor_item_h__


#include <array>
#include <vector>

#include "header.h"
#include "algo/loop.h"
#include "gui/mrview/volume.h"
#include "gui/mrview/tool/roi_editor/undoentry.h"


namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {
            


       namespace {
         constexpr std::array<std::array<GLubyte,3>,6> preset_colours = { {
           { { 255, 255, 0 } },
           { { 255, 0, 255 } },
           { { 0, 255, 255 } },
           { { 255, 0, 0 } },
           { { 0, 255, 255 } },
           { { 0, 0, 25 } }
         } };
       }



       class ROI_Item : public Volume {
          public:
            ROI_Item (MR::Header&&);

            void zero ();
            void load (MR::Header&);

            template <class ImageType>
            void save (ImageType&&, GLubyte*);

            bool has_undo () { return current_undo >= 0; }
            bool has_redo () { return current_undo < int(undo_list.size()-1); }

            ROI_UndoEntry& current () { return undo_list[current_undo]; }
            void start (ROI_UndoEntry&& entry);

            void undo ();
            void redo ();

            bool saved;
            float min_brush_size, max_brush_size, brush_size;

          private:
            std::vector<ROI_UndoEntry> undo_list;
            int current_undo;

            static int number_of_undos;
            static int current_preset_colour;
            static int new_roi_counter;
        };



        template <class ImageType>
        void ROI_Item::save (ImageType&& out, GLubyte* data)
        {
          for (auto l = Loop(out) (out); l; ++l)
            out.value() = data[out.index(0) + out.size(0) * (out.index(1) + out.size(1)*out.index(2))];
          saved = true;
          filename = out.name();
        }



      }
    }
  }
}

#endif


