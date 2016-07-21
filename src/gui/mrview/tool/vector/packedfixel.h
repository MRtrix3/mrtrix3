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
#ifndef __gui_mrview_tool_vector_packedfixel_h__
#define __gui_mrview_tool_vector_packedfixel_h__

#include "gui/mrview/tool/vector/fixel.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {
        class PackedFixel : public FixelType<FixelPackedImageType>
        {
          public:
            PackedFixel (const std::string& filename, Vector& fixel_tool) :
              FixelType (filename, fixel_tool)
            {
              value_types = {"Unity", "Length"};
              colour_types = {"Direction", "Length"};
              threshold_types = {"Length"};
              fixel_values[value_types[1]];
              fixel_data.reset (new FixelPackedImageType (header.get_image<float> ()));

              load_image (filename);
            }

            void load_image_buffer () override;
        };
      }
    }
  }
}

#endif
