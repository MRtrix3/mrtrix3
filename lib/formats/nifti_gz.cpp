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

#include "file/utils.h"
#include "file/path.h"
#include "file/gz.h"
#include "file/nifti_utils.h"
#include "header.h"
#include "image_io/gz.h"
#include "formats/list.h"

namespace MR
{
  namespace Formats
  {


    std::unique_ptr<ImageIO::Base> NIfTI_GZ::read (Header& H) const
    {
      if (!Path::has_suffix (H.name(), ".nii.gz")) 
        return std::unique_ptr<ImageIO::Base>();

      nifti_1_header N1H;
      nifti_2_header N2H;
      size_t data_offset = 0;

      try {
        File::GZ zf (H.name(), "rb");
        zf.read (reinterpret_cast<char*> (&N1H), File::NIfTI::ver1_hdr_size);
        zf.close();
        data_offset = File::NIfTI::read (H, N1H);
      } catch (...) {
        try {
          File::GZ zf (H.name(), "rb");
          zf.read (reinterpret_cast<char*> (&N2H), File::NIfTI::ver2_hdr_size);
          zf.close();
          data_offset = File::NIfTI::read (H, N2H);
        } catch (...) {
          throw Exception ("Error opening NIfti file \"" + H.name() + "\": Unsupported version");
        }
      }

      std::unique_ptr<ImageIO::GZ> io_handler (new ImageIO::GZ (H, data_offset));
      if (data_offset == File::NIfTI::ver1_hdr_with_ext_size) {
        memcpy (io_handler.get()->header(), &N1H, File::NIfTI::ver1_hdr_size);
        memset (io_handler.get()->header() + File::NIfTI::ver1_hdr_size, 0, sizeof(nifti1_extender));
      } else {
        memcpy (io_handler.get()->header(), &N2H, File::NIfTI::ver2_hdr_size);
        memset (io_handler.get()->header() + File::NIfTI::ver2_hdr_size, 0, sizeof(nifti1_extender));
      }
      io_handler->files.push_back (File::Entry (H.name(), data_offset));

      return std::move (io_handler);
    }





    bool NIfTI_GZ::check (Header& H, size_t num_axes) const
    {
      if (!Path::has_suffix (H.name(), ".nii.gz"))
        return false;

      if (num_axes < 3)
        throw Exception ("cannot create NIfTI image with less than 3 dimensions");

      if (num_axes > 7)
        throw Exception ("cannot create NIfTI image with more than 7 dimensions");

      H.ndim() = num_axes;
      File::NIfTI::check (H, true);

      return true;
    }





    std::unique_ptr<ImageIO::Base> NIfTI_GZ::create (Header& H) const
    {
      if (H.ndim() > 7)
        throw Exception ("NIfTI format cannot support more than 7 dimensions for image \"" + H.name() + "\"");

      const size_t nifti_version = File::NIfTI::version (H);
      size_t data_offset = 0;
      switch (nifti_version) {
        case 1: data_offset = File::NIfTI::ver1_hdr_with_ext_size; break;
        case 2: data_offset = File::NIfTI::ver2_hdr_with_ext_size; break;
        default: throw Exception ("Error determining NIfTI version for file \"" + H.name() + "\"");
      }

      std::unique_ptr<ImageIO::GZ> io_handler (new ImageIO::GZ (H, data_offset));

      switch (nifti_version) {
        case 1:
          File::NIfTI::write (*reinterpret_cast<nifti_1_header*> (io_handler->header()), H, true);
          memset (io_handler->header()+File::NIfTI::ver1_hdr_size, 0, sizeof(nifti1_extender));
          break;
        case 2:
          File::NIfTI::write (*reinterpret_cast<nifti_2_header*> (io_handler->header()), H, true);
          memset (io_handler->header()+File::NIfTI::ver2_hdr_size, 0, sizeof(nifti1_extender));
          break;
        default:
          break;
      }

      File::create (H.name());
      io_handler->files.push_back (File::Entry (H.name(), data_offset));

      return std::move (io_handler);
    }



  }
}

