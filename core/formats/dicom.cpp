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

#include <memory>

#include "file/path.h"
#include "file/config.h"
#include "file/dicom/mapper.h"
#include "file/dicom/tree.h"
#include "formats/list.h"
#include "header.h"
#include "image_io/base.h"

namespace MR
{
  namespace Formats
  {

    std::unique_ptr<ImageIO::Base> DICOM::read (Header& H) const
    {
      if (Path::is_dir (H.name())) {
        INFO ("Image path \"" + H.name() + "\" is a directory; will attempt to parse as DICOM series");
      } else if (!Path::has_suffix (H.name(), ".dcm")) {
        return std::unique_ptr<ImageIO::Base>();
      }

      File::Dicom::Tree dicom;

      dicom.read (H.name());
      dicom.sort();

      auto series = File::Dicom::select_func (dicom);
      if (series.empty())
        throw Exception ("no DICOM series selected");

      return dicom_to_mapper (H, series);
    }


    bool DICOM::check (Header& H, size_t num_axes) const
    {
      return false;
    }

    std::unique_ptr<ImageIO::Base> DICOM::create (Header& H) const
    {
      assert (0);
      return std::unique_ptr<ImageIO::Base>();
    }


  }
}
