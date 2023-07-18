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

#ifndef __file_nifti_utils_h__
#define __file_nifti_utils_h__

#include "types.h"
#include "raw.h"
#include "header.h"
#include "file/config.h"
#include "file/nifti1.h"
#include "file/nifti2.h"

namespace MR
{
  class Header;

  namespace File
  {
    namespace NIfTI
    {
      extern bool right_left_warning_issued;

      void axes_on_write (const Header& H, vector<size_t>& order, vector<bool>& flip);
      transform_type adjust_transform (const Header& H, vector<size_t>& order);

      bool check (int VERSION, Header& H, const size_t num_axes, const vector<std::string>& suffixes);

      template <int VERSION> std::unique_ptr<ImageIO::Base> read (Header& H);
      template <int VERSION> std::unique_ptr<ImageIO::Base> read_gz (Header& H);

      template <int VERSION> std::unique_ptr<ImageIO::Base> create (Header& H);
      template <int VERSION> std::unique_ptr<ImageIO::Base> create_gz (Header& H);

      int version (Header& H);
      std::string get_json_path (const std::string & nifti_path);

    }
  }
}

#endif

