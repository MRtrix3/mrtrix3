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
#include "file/nifti1_utils.h"
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
      size_t data_offset = File::NIfTI::read (H, * ( (const nifti_1_header*) fmap.address()));

      std::unique_ptr<ImageIO::Default> handler (new ImageIO::Default (H));
      handler->files.push_back (File::Entry (H.name(), data_offset));

      return std::move (handler);
    }





    bool NIfTI::check (Header& H, size_t num_axes) const
    {
      if (!Path::has_suffix (H.name(), ".nii")) return (false);
      if (num_axes < 3) throw Exception ("cannot create NIfTI-1.1 image with less than 3 dimensions");
      if (num_axes > 8) throw Exception ("cannot create NIfTI-1.1 image with more than 8 dimensions");

      H.ndim() = num_axes;
      File::NIfTI::check (H, true);

      return true;
    }





    std::unique_ptr<ImageIO::Base> NIfTI::create (Header& H) const
    {
      if (H.ndim() > 7)
        throw Exception ("NIfTI-1.1 format cannot support more than 7 dimensions for image \"" + H.name() + "\"");

      nifti_1_header NH;
      nifti1_extender extender;
      memset (extender.extension, 0x00, sizeof (nifti1_extender));
      File::NIfTI::write (NH, H, true);

      File::OFStream out (H.name(), std::ios::out | std::ios::binary);
      out.write ( (char*) &NH, 348);
      out.write (extender.extension, 4);
      out.close();

      File::resize (H.name(), 352 + footprint(H));

      std::unique_ptr<ImageIO::Default> handler (new ImageIO::Default (H));
      handler->files.push_back (File::Entry (H.name(), 352));

      return std::move (handler);
    }

  }
}

