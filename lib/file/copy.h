#ifndef __file_copy_h__
#define __file_copy_h__

#include "file/utils.h"
#include "file/mmap.h"


namespace MR
{
  namespace File
  {


    inline void copy (const std::string& source, const std::string& destination)
    {
      DEBUG ("copying file \"" + source + "\" to \"" + destination + "\"...");
      MMap input (source);
      create (destination, input.size());
      MMap output (destination, true);
      ::memcpy (output.address(), input.address(), input.size());
    }


  }
}

#endif


