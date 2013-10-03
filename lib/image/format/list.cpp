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

namespace MR
{
  namespace Image
  {
    namespace Format
    {
#ifdef MRTRIX_AS_R_LIBRARY
      Format::RAM        RAM_handler;
#endif

      Format::Pipe          pipe_handler;
      Format::MRtrix        mrtrix_handler;
      Format::MRtrix_GZ     mrtrix_gz_handler;
      Format::MRI           mri_handler;
      Format::NIfTI         nifti_handler;
      Format::NIfTI_GZ      nifti_gz_handler;
      Format::Analyse       analyse_handler;
      Format::XDS           xds_handler;
      Format::DICOM         dicom_handler;
      Format::MGH           mgh_handler;
      Format::MGZ           mgz_handler;
      Format::MRtrix_sparse mrtrix_sparse_handler;


      const Base* handlers[] = {
#ifdef MRTRIX_AS_R_LIBRARY
        &RAM_handler,
#endif
        &pipe_handler,
        &dicom_handler,
        &mrtrix_handler,
        &mrtrix_gz_handler,
        &nifti_handler,
        &nifti_gz_handler,
        &analyse_handler,
        &mri_handler,
        &xds_handler,
        &mgh_handler,
        &mgz_handler,
        &mrtrix_sparse_handler,
        NULL
      };



      const char* known_extensions[] = {
        ".mih",
        ".mif",
        ".mif.gz"
        ".img",
        ".nii",
        ".nii.gz",
        ".bfloat",
        ".bshort",
        ".mri",
        ".mgh",
        ".mgz",
        ".mgh.gz",
        ".msif",
        ".msih",
        NULL
      };

    }
  }
}



