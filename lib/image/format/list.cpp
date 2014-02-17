#include <cstdlib>

#include "image/format/list.h"

namespace MR
{
  namespace Image
  {
    namespace Format
    {

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


      const Base* handlers[] = {
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
        NULL
      };

    }
  }
}



