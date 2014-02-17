#ifndef __image_handler_default_h__
#define __image_handler_default_h__

#include "types.h"
#include "image/handler/base.h"
#include "file/mmap.h"

namespace MR
{
  namespace Image
  {

    namespace Handler
    {

      class Default : public Base
      {
        public:
          Default (const Image::Header& header) : 
            Base (header),
            bytes_per_segment (0) { }
          ~Default () { close(); }

        protected:
          std::vector<RefPtr<File::MMap> > mmaps;
          int64_t bytes_per_segment;

          virtual void load ();
          virtual void unload ();

          void map_files ();
          void copy_to_mem ();
          void copy_to_files ();

      };

    }
  }
}

#endif


