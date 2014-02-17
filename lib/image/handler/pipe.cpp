#include <limits>
#include <unistd.h>

#include "app.h"
#include "image/header.h"
#include "image/handler/pipe.h"
#include "image/utils.h"

namespace MR
{
  namespace Image
  {
    namespace Handler
    {


      void Pipe::load ()
      {
        assert (files.size() == 1);
        DEBUG ("mapping piped image \"" + files[0].name + "\"...");

        segsize /= files.size();
        int64_t bytes_per_segment = (datatype.bits() * segsize + 7) / 8;

        if (double (bytes_per_segment) >= double (std::numeric_limits<size_t>::max()))
          throw Exception ("image \"" + name + "\" is larger than maximum accessible memory");

        mmap = new File::MMap (files[0], writable, !is_new, bytes_per_segment);
        addresses.resize (1);
        addresses[0] = mmap->address();
      }


      void Pipe::unload()
      {
        if (mmap) {
          mmap = NULL;
          if (is_new)
            std::cout << files[0].name << "\n";
          else {
            DEBUG ("deleting piped image file \"" + files[0].name + "\"...");
            unlink (files[0].name.c_str());
          }
          addresses[0] = NULL;
        }
      }

    }
  }
}


