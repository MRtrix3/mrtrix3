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
    PAR           par_handler;
    NIfTI1        nifti1_handler;
    NIfTI2        nifti2_handler;
    NIfTI1_GZ     nifti1_gz_handler;
    NIfTI2_GZ     nifti2_gz_handler;
    XDS           xds_handler;
    DICOM         dicom_handler;
    MGH           mgh_handler;
    MGZ           mgz_handler;
#ifdef MRTRIX_TIFF_SUPPORT
    TIFF          tiff_handler;
#endif
#ifdef MRTRIX_PNG_SUPPORT
    PNG           png_handler;
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
      &nifti1_handler,
      &nifti2_handler,
      &nifti1_gz_handler,
      &nifti2_gz_handler,
      &mri_handler,
      &par_handler,
      &xds_handler,
      &mgh_handler,
      &mgz_handler,
#ifdef MRTRIX_TIFF_SUPPORT
      &tiff_handler,
#endif
#ifdef MRTRIX_PNG_SUPPORT
      &png_handler,
#endif
      &mrtrix_sparse_handler,
      nullptr
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
      ".par",
      ".mgh",
      ".mgz",
      ".mgh.gz",
      ".msf",
      ".msh",
      ".dcm",
#ifdef MRTRIX_TIFF_SUPPORT
      ".tiff",
      ".tif",
      ".TIFF",
      ".TIF",
#endif
#ifdef MRTRIX_PNG_SUPPORT
      ".png",
      ".PNG",
#endif
      nullptr
    };

  }
}



