/*
   Copyright 2008 Brain Research Institute, Melbourne, Australia

   Written by J-Donald Tournier, 27/06/08.

   This file is part of MRtrix.

   MRtrix is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   MRtrix is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with MRtrix.  If not, see <http://www.gnu.org/licenses/>.

 */

#include <cstdlib>

#include "image/format/list.h"

namespace MR {
  namespace Image {
    namespace Format {

      namespace {
        Format::MRtrix     mrtrix_handler;
        Format::MRI        mri_handler;
        Format::NIfTI      nifti_handler;
        Format::Analyse    analyse_handler;
        Format::XDS        xds_handler;
        Format::DICOM      dicom_handler;
      }

      const Base* handlers[] = {
        &mrtrix_handler,
        &mri_handler,
        &nifti_handler,
        &analyse_handler,
        &xds_handler,
        &dicom_handler,
        NULL
      };



      const char* known_extensions[] = {
        ".mih",
        ".mif",
        ".img",
        ".nii",
        ".bfloat",
        ".bshort",
        ".mri",
        NULL
      };

    }
  }
}



