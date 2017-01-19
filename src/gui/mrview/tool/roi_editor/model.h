/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 * 
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * For more details, see www.mrtrix.org
 * 
 */

#ifndef __gui_mrview_tool_roi_editor_model_h__
#define __gui_mrview_tool_roi_editor_model_h__

#include "header.h"
#include "memory.h"
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
        { MEMALIGN(ROI_Model)
          public:
            ROI_Model (QObject* parent) : 
              ListModelBase (parent) { }

            void load (vector<std::unique_ptr<MR::Header>>&);
            void create (MR::Header&&);

            ROI_Item* get (QModelIndex& index) {
              return dynamic_cast<ROI_Item*>(items[index.row()].get());
            }
        };




      }
    }
  }
}

#endif


