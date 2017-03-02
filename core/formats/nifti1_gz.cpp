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


#include "file/utils.h"
#include "file/path.h"
#include "file/gz.h"
#include "file/nifti_utils.h"
#include "file/nifti1_utils.h"
#include "header.h"
#include "image_io/gz.h"
#include "formats/list.h"

namespace MR
{
  namespace Formats
  {


    std::unique_ptr<ImageIO::Base> NIfTI1_GZ::read (Header& H) const
    {
      if (!Path::has_suffix (H.name(), ".nii.gz")) 
        return std::unique_ptr<ImageIO::Base>();

      nifti_1_header NH;
      File::GZ zf (H.name(), "rb");
      zf.read (reinterpret_cast<char*> (&NH), File::NIfTI1::header_size);
      zf.close();

      try {
        const size_t data_offset = File::NIfTI1::read (H, NH);
        std::unique_ptr<ImageIO::GZ> io_handler (new ImageIO::GZ (H, data_offset));
        memcpy (io_handler.get()->header(), &NH, File::NIfTI1::header_size);
        memset (io_handler.get()->header() + File::NIfTI1::header_size, 0, sizeof(nifti1_extender));
        io_handler->files.push_back (File::Entry (H.name(), data_offset));
        return std::move (io_handler);
      } catch (...) {
        return std::unique_ptr<ImageIO::Base>();
      }
    }





    bool NIfTI1_GZ::check (Header& H, size_t num_axes) const
    {
      if (!Path::has_suffix (H.name(), ".nii.gz")) return false;
      if (File::NIfTI::version (H) != 1) return false;

      if (num_axes < 3) throw Exception ("cannot create NIfTI-1.1 image with less than 3 dimensions");
      if (num_axes > 7) throw Exception ("cannot create NIfTI-1.1 image with more than 7 dimensions");

      H.ndim() = num_axes;
      File::NIfTI::check (H, false);

      return true;
    }





    std::unique_ptr<ImageIO::Base> NIfTI1_GZ::create (Header& H) const
    {
      if (H.ndim() > 7)
        throw Exception ("NIfTI-1.1 format cannot support more than 7 dimensions for image \"" + H.name() + "\"");

      std::unique_ptr<ImageIO::GZ> io_handler (new ImageIO::GZ (H, File::NIfTI1::header_with_ext_size));

      File::NIfTI1::write (*reinterpret_cast<nifti_1_header*> (io_handler->header()), H, true);
      memset (io_handler->header()+File::NIfTI1::header_size, 0, sizeof(nifti1_extender));

      File::create (H.name());
      io_handler->files.push_back (File::Entry (H.name(), File::NIfTI1::header_with_ext_size));

      return std::move (io_handler);
    }



  }
}

