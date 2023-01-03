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

#ifndef __gui_mrview_tool_fixel_image4D_h__
#define __gui_mrview_tool_fixel_image4D_h__

#include "gui/mrview/tool/fixel/base_fixel.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {
        class Image4D : public FixelType<FixelImage4DType>
        { MEMALIGN (Image4D)
          public:
            Image4D (const std::string& filename, Fixel& fixel_tool) :
              FixelType (filename, fixel_tool), tracking (false)
            {
              value_types = {"Unity", "Length"};
              colour_types = {"Direction", "Length"};
              threshold_types = {"Length"};
              fixel_values[value_types[1]];
              fixel_data.reset (new FixelImage4DType (header.get_image<float> ()));

              load_image (filename);
            }

            void load_image_buffer () override;
            void reload_image_buffer ();

            void update_image_buffers () override;

            bool trackable () const {
              if (fixel_data->ndim() < 5)
                return false;
              if (fixel_data->size(4) <= 1)
                return false;
              return true;
            }
            bool tracking;
        };
    }
    }
  }
}

#endif
