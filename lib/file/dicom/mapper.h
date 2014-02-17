#ifndef __file_dicom_mapper_h__
#define __file_dicom_mapper_h__

#include "ptr.h"

namespace MR {

  namespace Image { 
    class Header; 
    namespace Handler {
      class Base;
    }
  }

  namespace File {
    namespace Dicom {

      class Series;

      RefPtr<MR::Image::Handler::Base> dicom_to_mapper (MR::Image::Header& H, std::vector< RefPtr<Series> >& series);
      
    }
  }
}

#endif




