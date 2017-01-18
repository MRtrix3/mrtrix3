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
#include "file/nifti1_utils.h"
#include "header.h"
#include "image_io/default.h"
#include "formats/list.h"

namespace MR
{
  namespace Formats
  {

    std::unique_ptr<ImageIO::Base> NIfTI1::read (Header& H) const
    {
      if (!Path::has_suffix (H.name(), ".nii")) 
        return std::unique_ptr<ImageIO::Base>();

      try {
        File::MMap fmap (H.name());
        const size_t data_offset = File::NIfTI1::read (H, * ( (const nifti_1_header*) fmap.address()));
        std::unique_ptr<ImageIO::Default> handler (new ImageIO::Default (H));
        handler->files.push_back (File::Entry (H.name(), data_offset));
        return std::move (handler);
      } catch (...) {
        return std::unique_ptr<ImageIO::Base>();
      }
    }





    bool NIfTI1::check (Header& H, size_t num_axes) const
    {
      if (!Path::has_suffix (H.name(), ".nii")) return (false);
      if (File::NIfTI::version (H) != 1) return false;

      if (num_axes < 3) throw Exception ("cannot create NIfTI-1.1 image with less than 3 dimensions");
      if (num_axes > 7) throw Exception ("cannot create NIfTI-1.1 image with more than 7 dimensions");

      H.ndim() = num_axes;
      File::NIfTI::check (H, false);

      return true;
    }





    std::unique_ptr<ImageIO::Base> NIfTI1::create (Header& H) const
    {
      if (H.ndim() > 7)
        throw Exception ("NIfTI-1.1 format cannot support more than 7 dimensions for image \"" + H.name() + "\"");

      nifti_1_header NH;
      File::NIfTI1::write (NH, H, true);
      File::OFStream out (H.name(), std::ios::out | std::ios::binary);
      out.write ( (char*) &NH, sizeof (nifti_1_header));
      nifti1_extender extender;
      memset (extender.extension, 0x00, sizeof (nifti1_extender));
      out.write (extender.extension, sizeof (nifti1_extender));
      out.close();

      File::resize (H.name(), File::NIfTI1::header_with_ext_size + footprint(H));

      std::unique_ptr<ImageIO::Default> handler (new ImageIO::Default (H));
      handler->files.push_back (File::Entry (H.name(), File::NIfTI1::header_with_ext_size));

      return std::move (handler);
    }



  }
}

