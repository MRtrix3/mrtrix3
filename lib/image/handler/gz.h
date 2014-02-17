#ifndef __image_handler_gz_h__
#define __image_handler_gz_h__

#include "image/handler/base.h"
#include "file/mmap.h"

namespace MR
{
  namespace Image
  {

    namespace Handler
    {

      class GZ : public Base
      {
        public:
          GZ (Header& header, size_t file_header_size) :
            Base (header), lead_in_size (file_header_size) {
            lead_in = file_header_size ? new uint8_t [file_header_size] : NULL ;
          }
          ~GZ () { 
            close();
            delete [] lead_in;
          }

          uint8_t* header () {
            return lead_in;
          }

        protected:
          int64_t  bytes_per_segment;
          uint8_t* lead_in;
          size_t   lead_in_size;

          virtual void load ();
          virtual void unload ();
      };

    }
  }
}

#endif


