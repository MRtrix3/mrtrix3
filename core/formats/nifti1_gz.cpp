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

#include "file/nifti_utils.h"
#include "formats/list.h"

namespace MR
{
  namespace Formats
  {


    std::unique_ptr<ImageIO::Base> NIfTI1_GZ::read (Header& H) const
    {
      return File::NIfTI::read_gz<1> (H);
    }



    bool NIfTI1_GZ::check (Header& H, size_t num_axes) const
    {
      return File::NIfTI::check (1, H, num_axes, { ".nii.gz" });
    }



    std::unique_ptr<ImageIO::Base> NIfTI1_GZ::create (Header& H) const
    {
      return File::NIfTI::create_gz<1> (H);
    }



  }
}

