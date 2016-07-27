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

#include "file/ofstream.h"
#include "file/path.h"
#include "file/utils.h"
#include "file/nifti_utils.h"
#include "header.h"
#include "image_io/default.h"
#include "formats/list.h"

namespace MR
{
  namespace Formats
  {

    std::unique_ptr<ImageIO::Base> NIfTI::read (Header& H) const
    {
      if (!Path::has_suffix (H.name(), ".nii")) 
        return std::unique_ptr<ImageIO::Base>();

      File::MMap fmap (H.name());
      size_t data_offset = 0;
      try {
        data_offset = File::NIfTI::read (H, * ( (const nifti_1_header*) fmap.address()));
      } catch (...) {
        try {
          data_offset = File::NIfTI::read (H, * ( (const nifti_2_header*) fmap.address()));
        } catch (...) {
          throw Exception ("Error opening NIfti file \"" + H.name() + "\": Unsupported version");
        }
      }

      std::unique_ptr<ImageIO::Default> handler (new ImageIO::Default (H));
      handler->files.push_back (File::Entry (H.name(), data_offset));

      return std::move (handler);
    }





    bool NIfTI::check (Header& H, size_t num_axes) const
    {
      if (!Path::has_suffix (H.name(), ".nii")) return (false);
      if (num_axes < 3) throw Exception ("cannot create NIfTI image with less than 3 dimensions");
      if (num_axes > 7) throw Exception ("cannot create NIfTI image with more than 7 dimensions");

      H.ndim() = num_axes;
      File::NIfTI::check (H, true);

      return true;
    }





    std::unique_ptr<ImageIO::Base> NIfTI::create (Header& H) const
    {
      if (H.ndim() > 7)
        throw Exception ("NIfTI format cannot support more than 7 dimensions for image \"" + H.name() + "\"");

      const size_t nifti_version = File::NIfTI::version (H);
      size_t data_offset = 0;
      File::OFStream out (H.name(), std::ios::out | std::ios::binary);
      switch (nifti_version) {
        case 1: {
          nifti_1_header NH;
          File::NIfTI::write (NH, H, true);
          out.write ( (char*) &NH, sizeof (nifti_1_header));
          data_offset = File::NIfTI::ver1_hdr_with_ext_size;
          DEBUG ("Image \"" + H.name() + "\" being created with NIfTI version 1.1");
        }
        break;
        case 2: {
          nifti_2_header NH;
          File::NIfTI::write (NH, H, true);
          out.write ( (char*) &NH, sizeof (nifti_2_header));
          data_offset = File::NIfTI::ver2_hdr_with_ext_size;
          DEBUG ("Image \"" + H.name() + "\" being created with NIfTI version 2");
        }
        break;
        default:
          throw Exception ("Error determining NIfTI version for file \"" + H.name() + "\"");
      }

      nifti1_extender extender;
      memset (extender.extension, 0x00, sizeof (nifti1_extender));
      out.write (extender.extension, sizeof (nifti1_extender));
      out.close();

      File::resize (H.name(), data_offset + footprint(H));

      std::unique_ptr<ImageIO::Default> handler (new ImageIO::Default (H));
      handler->files.push_back (File::Entry (H.name(), data_offset));

      return std::move (handler);
    }



  }
}

