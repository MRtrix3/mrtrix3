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




