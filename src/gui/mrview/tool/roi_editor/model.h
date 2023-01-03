/* Copyright (c) 2008-2023 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Covered Software is provided under this License on an "as is"
 * basis, without warranty of any kind, either expressed, implied, or
 * statutory, including, without limitation, warranties that the
 * Covered Software is free of defects, merchantable, fit for a
 * particular purpose or non-infringing.
 * See the Mozilla Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
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


