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

#include "image/header.h"
#include "image/info.h"
#include "image/loop.h"
#include "image/voxel.h"
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
            ROI_Item (const MR::Image::Info&);

            void zero ();
            void load (const MR::Image::Header& header);

            template <class VoxelType> 
            void save (VoxelType&&, GLubyte*);

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



        template <class VoxelType>
        void ROI_Item::save (VoxelType&& vox, GLubyte* data) {
          for (auto l = MR::Image::Loop() (vox); l; ++l)
            vox.value() = data[vox[0] + vox.dim(0) * (vox[1] + vox.dim(1)*vox[2])];
          saved = true;
          filename = vox.name();
        }



      }
    }
  }
}

#endif


