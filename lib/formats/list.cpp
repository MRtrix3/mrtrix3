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

#include <cstdlib>

#include "formats/list.h"

namespace MR
{
  namespace Formats
  {
#ifdef MRTRIX_AS_R_LIBRARY
    RAM        RAM_handler;
#endif

    Pipe          pipe_handler;
    MRtrix        mrtrix_handler;
    MRtrix_GZ     mrtrix_gz_handler;
    MRI           mri_handler;
    NIfTI         nifti_handler;
    NIfTI_GZ      nifti_gz_handler;
    Analyse       analyse_handler;
    XDS           xds_handler;
    DICOM         dicom_handler;
    MGH           mgh_handler;
    MGZ           mgz_handler;
#ifdef MRTRIX_TIFF_SUPPORT
    TIFF          tiff_handler;
#endif
    MRtrix_sparse mrtrix_sparse_handler;


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
#ifdef MRTRIX_TIFF_SUPPORT
      &tiff_handler,
#endif
      &mrtrix_sparse_handler,
      NULL
    };



    const char* known_extensions[] = {
      ".mih",
      ".mif",
      ".mif.gz",
      ".img",
      ".nii",
      ".nii.gz",
      ".bfloat",
      ".bshort",
      ".mri",
      ".mgh",
      ".mgz",
      ".mgh.gz",
      ".msf",
      ".msh",
      ".dcm",
      ".tiff",
      ".tif",
      ".TIFF",
      ".TIF",
      NULL
    };

  }
}



