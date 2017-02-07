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


#include "file/ofstream.h"
#include "file/path.h"
#include "file/utils.h"
#include "file/nifti1.h"
#include "file/nifti_utils.h"
#include "file/nifti2_utils.h"
#include "header.h"
#include "image_io/default.h"
#include "formats/list.h"

namespace MR
{
  namespace Formats
  {

    std::unique_ptr<ImageIO::Base> NIfTI2::read (Header& H) const
    {
      if (!Path::has_suffix (H.name(), ".nii") && !Path::has_suffix (H.name(), ".img"))
        return std::unique_ptr<ImageIO::Base>();

      const std::string header_path = Path::has_suffix (H.name(), ".nii") ?
                                      H.name() :
                                      H.name().substr (0, H.name().size()-4) + ".hdr";

      File::MMap fmap (header_path);

      try {
        const size_t data_offset = File::NIfTI2::read (H, * ( (const nifti_2_header*) fmap.address()));
        std::unique_ptr<ImageIO::Default> handler (new ImageIO::Default (H));
        handler->files.push_back (File::Entry (H.name(), data_offset));
        return std::move (handler);
      } catch (...) {
        return std::unique_ptr<ImageIO::Base>();
      }
    }





    bool NIfTI2::check (Header& H, size_t num_axes) const
    {
      if (!Path::has_suffix (H.name(), ".nii") && !Path::has_suffix (H.name(), ".img")) return (false);
      if (File::NIfTI::version (H) != 2) return false;

      if (num_axes < 3) throw Exception ("cannot create NIfTI-2 image with less than 3 dimensions");
      if (num_axes > 7) throw Exception ("cannot create NIfTI-2 image with more than 7 dimensions");

      H.ndim() = num_axes;
      // Even though the file may be split across .img/.hdr files, there's no risk
      //   of it being interpreted as an Analyse image because it's NIfTI-2
      File::NIfTI::check (H, false);

      return true;
    }





    std::unique_ptr<ImageIO::Base> NIfTI2::create (Header& H) const
    {
      if (H.ndim() > 7)
        throw Exception ("NIfTI-2 format cannot support more than 7 dimensions for image \"" + H.name() + "\"");

      const bool single_file = Path::has_suffix (H.name(), ".nii");
      const std::string header_path = single_file ? H.name() : H.name().substr (0, H.name().size()-4) + ".hdr";

      nifti_2_header NH;
      File::NIfTI2::write (NH, H, true);
      File::OFStream out (header_path, std::ios::out | std::ios::binary);
      out.write ( (char*) &NH, sizeof (nifti_2_header));
      nifti1_extender extender;
      memset (extender.extension, 0x00, sizeof (nifti1_extender));
      out.write (extender.extension, sizeof (nifti1_extender));
      out.close();

      const size_t data_offset = single_file ? File::NIfTI2::header_with_ext_size : 0;

      if (single_file)
        File::resize (H.name(), data_offset + footprint(H));
      else
        File::create (H.name(), footprint(H));

      std::unique_ptr<ImageIO::Default> handler (new ImageIO::Default (H));
      handler->files.push_back (File::Entry (H.name(), data_offset));

      return std::move (handler);
    }



  }
}

