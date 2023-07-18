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

#ifndef __file_dicom_mapper_h__
#define __file_dicom_mapper_h__

#include "memory.h"

namespace MR {

  class Header; 
  namespace ImageIO {
    class Base;
  }

  namespace File {
    namespace Dicom {

      class Series;

      std::unique_ptr<MR::ImageIO::Base> dicom_to_mapper (MR::Header& H, vector<std::shared_ptr<Series>>& series);
      
    }
  }
}

#endif




