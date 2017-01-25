/* Copyright (c) 2008-2017 the MRtrix3 contributors
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

#ifndef __gui_mrview_tool_fixel_directory_h__
#define __gui_mrview_tool_fixel_directory_h__

#include "gui/mrview/tool/fixel/base_fixel.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {
        class Directory : public FixelType<FixelIndexImageType>
        {
          public:
            Directory (const std::string& filename, Fixel& fixel_tool) :
              FixelType (MR::Fixel::find_index_header (Path::dirname (filename)).name (), fixel_tool)
            {
              value_types = {"unity"};
              colour_types = {"direction"};

              fixel_data.reset (new FixelIndexImageType (header.get_image<uint32_t> ()));
              load_image (filename);
            }

            void load_image_buffer () override;
            FixelValue& get_fixel_value (const std::string& key) const override;
          protected:
            void lazy_load_fixel_value_file (const std::string& key) const;
        };
      }
    }
  }
}

#endif
