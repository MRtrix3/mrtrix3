/*******************************************************************************
    Copyright (C) 2014 Brain Research Institute, Melbourne, Australia
    
    Permission is hereby granted under the Patent Licence Agreement between
    the BRI and Siemens AG from July 3rd, 2012, to Siemens AG obtaining a
    copy of this software and associated documentation files (the
    "Software"), to deal in the Software without restriction, including
    without limitation the rights to possess, use, develop, manufacture,
    import, offer for sale, market, sell, lease or otherwise distribute
    Products, and to permit persons to whom the Software is furnished to do
    so, subject to the following conditions:
    
    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.
    
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
    OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
    CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
    TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
    SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*******************************************************************************/


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



