/* Copyright (c) 2008-2025 the MRtrix3 contributors.
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

#pragma once

#include "mrview/tool/fixel/base_fixel.h"

namespace MR::GUI::MRView::Tool {
class Directory : public FixelType<FixelIndexImageType> {
public:
  Directory(const std::filesystem::path &filename, Fixel &fixel_tool)
      : FixelType(MR::Fixel::find_index_header(filename.parent_path()).name(), fixel_tool) {
    value_types = {"unity"};
    colour_types = {"direction"};

    fixel_data.reset(new FixelIndexImageType(header.get_image<uint32_t>()));
    load_image(filename);
  }

  void load_image_buffer() override;
  FixelValue &get_fixel_value(const std::string &key) const override;

protected:
  void lazy_load_fixel_value_file(const std::string &key) const;
};
} // namespace MR::GUI::MRView::Tool
