/* Copyright (c) 2008-2019 the MRtrix3 contributors.
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

    std::unique_ptr<ImageIO::Base> NIfTI1::read (Header& H) const
    {
      return File::NIfTI::read<1> (H);
    }



    bool NIfTI1::check (Header& H, size_t num_axes) const
    {
      const vector<std::string> suffixes { ".nii", ".img" };
      //CONF option: IMGOutputsAnalyze
      //CONF default: 0 (false)
      //CONF A boolean value to indicate whether newly-created images with a
      //CONF `.img` suffix should be treated as Analyze format, or as NIfTI.
      //CONF For reference: Analyze images produced by MRtrix3 are NIfTI-1
      //CONF compliant, but use standard ordering (LAS or RAS depending on the
      //CONF Analyse.LeftToRight configutation file option)
      const bool is_analyze = File::Config::get_bool ("IMGOutputsAnalyze", false) ?
        Path::has_suffix (H.name(), ".img") :
        false;
      return File::NIfTI::check (H, num_axes, is_analyze, suffixes, 1, is_analyze ? "Analyze" : "NIfTI-1.1");
    }



    std::unique_ptr<ImageIO::Base> NIfTI1::create (Header& H) const
    {
      return File::NIfTI::create<1> (H);
    }



  }
}

