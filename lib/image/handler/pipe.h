#ifndef __image_handler_pipe_h__
#define __image_handler_pipe_h__

#include "image/handler/base.h"
#include "file/mmap.h"

namespace MR
{
  namespace Image
  {

    namespace Handler
    {

      class Pipe : public Base
      {
        public:
          Pipe (Base& handler) : Base (handler) { }
          ~Pipe () { close(); }

        protected:
          Ptr<File::MMap> mmap;

          virtual void load ();
          virtual void unload ();
      };

    }
  }
}

#endif


