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


#ifndef __gui_mrview_tool_fixel_legacy_h__
#define __gui_mrview_tool_fixel_legacy_h__

#include "gui/mrview/tool/fixel/base_fixel.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {
        class Legacy : public FixelType<FixelLegacyType>
        { MEMALIGN (Legacy)
          public:
            Legacy (const std::string& filename, Fixel& fixel_tool) :
              FixelType (filename, fixel_tool)
            {
              value_types = {"unity", "fixel size", "associated value"};
              colour_types = {"direction", "fixel size", "associated value"};
              threshold_types = {"fixel size", "associated value"};
              fixel_values[value_types[1]];
              fixel_values[value_types[2]];

              fixel_data.reset (new FixelLegacyType (header));
              load_image (filename);
            }

            void load_image_buffer () override;
        };
      }
    }
  }
}

#endif
