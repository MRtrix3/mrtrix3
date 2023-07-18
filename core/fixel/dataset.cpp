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

#include "fixel/dataset.h"

#include "header.h"
#include "image_helpers.h"
#include "fixel/helpers.h"

namespace MR
{
  namespace Fixel
  {



    // TODO Don't necessarily know what this is going to be constructed from
    // Might be beneficial to have a helper class here that figures out
    //   from the user input what the fixel directory path is
    Dataset::Dataset (const std::string& path) :
        Fixel::IndexImage (Fixel::find_index_header (path)),
        directory_path (path)
    {
      directions_image_type directions_image = Fixel::find_directions_header (directory_path, nfixels()).get_image<float>();
      if (directions_image.size(0) != nfixels())
        throw Exception ("Number of fixels in directions image does not match number of fixels in index image");
      directions.resize (nfixels(), 3);
      for (auto l = MR::Loop(0) (directions_image); l; ++l)
        directions.row(ssize_t(directions_image.index(0))) = directions_image.row(1);

      auto dixelmask_header = Fixel::find_dixelmasks_header (directory_path, nfixels());
      if (dixelmask_header.valid()) {
        auto dixelmask_dirs = Fixel::find_dixelmasks_directions (dixelmask_header);
        if (dixelmask_dirs.rows()) {
          auto dixelmask_image = dixelmask_header.get_image<bool>();
          mask_directions = Math::Sphere::Set::Assigner (dixelmask_dirs);
          dixel_masks.resize (nfixels(), dixelmask_header.size(1));
          // Does not compile
          // for (auto l = MR::Loop(0) (dixelmask_image); l; ++l)
          //   dixel_masks.row (dixelmask_image.index(0)) = dixelmask_image.row(1);
          for (auto l = MR::Loop (dixelmask_header) (dixelmask_image); l; ++l)
            dixel_masks (ssize_t(dixelmask_image.index(0)), ssize_t(dixelmask_image.index(1))) = dixelmask_image.value();
        } else {
          WARN ("Dixel mask image found in fixel dataset \"" + directory_path + "\", "
                "but no corresponding set of unit directions found; "
                "these data will not be used in subsequent calculations");
        }
      }
    }



  }
}

