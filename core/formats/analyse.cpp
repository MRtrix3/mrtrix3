/* Copyright (c) 2008-2017 the MRtrix3 contributors.
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


#include "file/ofstream.h"
#include "file/utils.h"
#include "file/entry.h"
#include "file/nifti_utils.h"
#include "file/nifti1_utils.h"
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

      try {
        File::NIfTI1::read (H, * ( (const nifti_1_header*) fmap.address()));
        std::unique_ptr<ImageIO::Base> io_handler (new ImageIO::Default (H));
        io_handler->files.push_back (File::Entry (H.name()));
        return io_handler;
      } catch (...) {
        return std::unique_ptr<ImageIO::Base>();
      }
    }





    bool Analyse::check (Header& H, size_t num_axes) const
    {
      if (!Path::has_suffix (H.name(), ".img")) return false;
      if (File::NIfTI::version (H) != 1) return false;

      if (num_axes < 3) throw Exception ("cannot create Analyse / NIfTI image with less than 3 dimensions");
      if (num_axes > 7) throw Exception ("cannot create Analyse / NIfTI image with more than 7 dimensions");

      H.ndim() = num_axes;
      File::NIfTI::check (H, true);

      return true;
    }





    std::unique_ptr<ImageIO::Base> Analyse::create (Header& H) const
    {
      if (H.ndim() > 7)
        throw Exception ("Analyse / NIfTI format cannot support more than 7 dimensions for image \"" + H.name() + "\"");

      const std::string hdr_name (H.name().substr (0, H.name().size()-4) + ".hdr");
      File::OFStream out (hdr_name);
      nifti_1_header NH;
      File::NIfTI1::write (NH, H, false);
      out.write ( (char*) &NH, sizeof (nifti_1_header));
      out.close();

      File::create (H.name(), footprint(H));
      std::unique_ptr<ImageIO::Base> io_handler (new ImageIO::Default (H));
      io_handler->files.push_back (File::Entry (H.name()));

      return io_handler;
    }

  }
}
