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
#include "file/utils.h"
#include "file/entry.h"
#include "file/nifti_utils.h"
#include "header.h"
#include "formats/list.h"
#include "image_io/default.h"

namespace MR
{
  namespace Formats
  {

    std::unique_ptr<ImageIO::Base> Analyse::read (Header& H) const
    {
      if (!Path::has_suffix (H.name(), ".img"))
        return std::unique_ptr<ImageIO::Base>();

      const std::string header_path = H.name().substr (0, H.name().size()-4) + ".hdr";
      File::MMap fmap (header_path);
      if (fmap.size() == File::NIfTI::ver1_hdr_size || fmap.size() == File::NIfTI::ver1_hdr_with_ext_size)
        File::NIfTI::read (H, * ( (const nifti_1_header*) fmap.address()));
      else if (fmap.size() == File::NIfTI::ver2_hdr_size || fmap.size() == File::NIfTI::ver2_hdr_with_ext_size)
        File::NIfTI::read (H, * ( (const nifti_2_header*) fmap.address()));
      else
        throw Exception ("Error reading NIfTI header file \"" + header_path + "\": Invalid size (" + str(fmap.size()) + ")");

      std::unique_ptr<ImageIO::Base> io_handler (new ImageIO::Default (H));
      io_handler->files.push_back (File::Entry (H.name()));

      return io_handler;
    }





    bool Analyse::check (Header& H, size_t num_axes) const
    {
      if (!Path::has_suffix (H.name(), ".img"))
        return false;

      if (num_axes < 3)
        throw Exception ("cannot create NIfTI image with less than 3 dimensions");

      if (num_axes > 7)
        throw Exception ("cannot create NIfTI image with more than 7 dimensions");

      H.ndim() = num_axes;
      File::NIfTI::check (H, false);

      return true;
    }





    std::unique_ptr<ImageIO::Base> Analyse::create (Header& H) const
    {
      if (H.ndim() > 7)
        throw Exception ("NIfTI format cannot support more than 7 dimensions for image \"" + H.name() + "\"");

      const std::string hdr_name (H.name().substr (0, H.name().size()-4) + ".hdr");
      File::OFStream out (hdr_name);

      const size_t nifti_version = File::NIfTI::version (H);
      switch (nifti_version) {
        case 1: {
          nifti_1_header NH;
          File::NIfTI::write (NH, H, false);
          out.write ( (char*) &NH, sizeof (nifti_1_header));
        }
        break;
        case 2: {
          nifti_2_header NH;
          File::NIfTI::write (NH, H, false);
          out.write ( (char*) &NH, sizeof (nifti_2_header));
        }
        break;
        default:
          throw Exception ("Error determining NIfTI version for file \"" + H.name() + "\"");
      }

      out.close();

      File::create (H.name(), footprint(H));

      std::unique_ptr<ImageIO::Base> io_handler (new ImageIO::Default (H));
      io_handler->files.push_back (File::Entry (H.name()));

      return io_handler;
    }

  }
}
