#ifndef __file_mgh_utils_h__
#define __file_mgh_utils_h__

#include "file/mgh.h"

#define MGH_HEADER_SIZE 90
#define MGH_DATA_OFFSET 284

namespace MR
{
  namespace Image
  {
    class Header;
  }
  namespace File
  {
    namespace MGH
    {

      bool read_header  (Image::Header& H, const mgh_header& MGHH);
      void read_other   (Image::Header& H, const mgh_other& MGHO, const bool is_BE);
      void write_header (mgh_header& MGHH, const Image::Header& H);
      void write_other  (mgh_other&  MGHO, const Image::Header& H);

      void write_other_to_file (const std::string&, const mgh_other&);

    }
  }
}

#endif

