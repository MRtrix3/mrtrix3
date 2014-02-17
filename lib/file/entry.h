#ifndef __file_entry_h__
#define __file_entry_h__

#include <string>

#include "types.h"

namespace MR
{
  namespace File
  {

    class Entry
    {
      public:
        Entry (const std::string& fname, int64_t offset = 0) :
          name (fname), start (offset) { }

        std::string name;
        int64_t start;
    };


    inline std::ostream& operator<< (std::ostream& stream, const Entry& e)
    {
      stream << "File::Entry { \"" << e.name << "\", offset " << e.start << " }";
      return stream;
    }
  }
}

#endif

