#ifndef __file_nifti1_utils_h__
#define __file_nifti1_utils_h__

#include "file/nifti1.h"

namespace MR
{
  namespace Image
  {
    class Header;
  }
  namespace File
  {
    namespace NIfTI
    {

      void check (Image::Header& H, bool single_file);
      //! \todo add straight Analyse support
      size_t read (Image::Header& H, const nifti_1_header& NH);
      void check (Image::Header& H, bool single_file);
      //! \todo need to double-check new transform handling code
      void write (nifti_1_header& NH, const Image::Header& H, bool single_file);

    }
  }
}

#endif

