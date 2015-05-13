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

#ifndef __gui_mrview_tool_roi_editor_model_h__
#define __gui_mrview_tool_roi_editor_model_h__

#include "memory.h"
#include "image/header.h"
#include "gui/mrview/tool/list_model_base.h"
#include "gui/mrview/tool/roi_editor/item.h"


namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {

            


        class ROI_Model : public ListModelBase
        {
          public:
            ROI_Model (QObject* parent) : 
              ListModelBase (parent) { }

            void load (std::vector<std::unique_ptr<MR::Image::Header>>& list);
            void create (MR::Image::Header& image);

            ROI_Item* get (QModelIndex& index) {
              return dynamic_cast<ROI_Item*>(items[index.row()].get());
            }
        };




      }
    }
  }
}

#endif


