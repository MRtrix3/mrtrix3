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

#include "gui/mrview/window.h"
#include "gui/mrview/tool/roi_editor/model.h"


namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {
            




        void ROI_Model::load (std::vector<std::unique_ptr<MR::Header>>& list)
        {
          beginInsertRows (QModelIndex(), items.size(), items.size()+list.size());
          for (size_t i = 0; i < list.size(); ++i) {
            Window::GrabContext context;
            MR::Header H (*list[i]);
            ROI_Item* roi = new ROI_Item (std::move(H));
            roi->load (*list[i]);
            items.push_back (std::unique_ptr<Displayable> (roi));
          }
          endInsertRows();
        }

        void ROI_Model::create (MR::Header&& image)
        {
          beginInsertRows (QModelIndex(), items.size(), items.size()+1);
          {
            Window::GrabContext context;
            ROI_Item* roi = new ROI_Item (std::move (image));
            roi->zero ();
            items.push_back (std::unique_ptr<Displayable> (roi));
          }
          endInsertRows();
        }





      }
    }
  }
}




