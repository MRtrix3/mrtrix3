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
      if (!Path::is_dir (H.name())) 
        if (!Path::has_suffix (H.name(), ".dcm"))
          return std::unique_ptr<ImageIO::Base>();

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
