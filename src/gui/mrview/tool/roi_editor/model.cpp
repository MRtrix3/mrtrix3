/*
 * Copyright (c) 2008-2018 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/
 *
 * MRtrix3 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/
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





        void ROI_Model::load (vector<std::unique_ptr<MR::Header>>& list)
        {
          beginInsertRows (QModelIndex(), items.size(), items.size()+list.size());
          for (size_t i = 0; i < list.size(); ++i) {
            MRView::GrabContext context;
            ROI_Item* roi = new ROI_Item (std::move (*list[i]));
            roi->load ();
            items.push_back (std::unique_ptr<Displayable> (roi));
          }
          endInsertRows();
        }

        void ROI_Model::create (MR::Header&& image)
        {
          beginInsertRows (QModelIndex(), items.size(), items.size()+1);
          {
            MRView::GrabContext context;
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




