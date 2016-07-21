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
#ifndef __gui_mrview_tool_vector_sparsefixel_h__
#define __gui_mrview_tool_vector_sparsefixel_h__

#include "gui/mrview/tool/vector/fixel.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {
        class SparseFixel : public FixelType<FixelSparseImageType>
        {
          public:
            SparseFixel (const std::string& filename, Vector& fixel_tool) :
              FixelType (filename, fixel_tool)
            {
              value_types = {"Unity", "Fixel size", "Associated value"};
              colour_types = {"Direction", "Fixel size", "Associated value"};
              threshold_types = {"Fixel size", "Associated value"};
              fixel_values[value_types[1]];
              fixel_values[value_types[2]];

              fixel_data.reset (new FixelSparseImageType (header));
              load_image (filename);
            }

            void load_image_buffer () override;
        };
      }
    }
  }
}

#endif
