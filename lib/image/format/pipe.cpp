#include "file/utils.h"
#include "file/path.h"
#include "image/header.h"
#include "image/handler/pipe.h"
#include "image/format/list.h"

namespace MR
{
  namespace Image
  {
    namespace Format
    {

      RefPtr<Handler::Base> Pipe::read (Header& H) const
      {
        if (H.name() != "-") 
          return RefPtr<Handler::Base>();

        std::string name;
        getline (std::cin, name);
        H.name() = name;

        if (H.name().empty())
          throw Exception ("no filename supplied to standard input (broken pipe?)");

        if (!Path::has_suffix (H.name(), ".mif"))
          throw Exception ("MRtrix only supports the .mif format for command-line piping");

        RefPtr<Handler::Base> original_handler (mrtrix_handler.read (H));
        RefPtr<Handler::Pipe> handler (new Handler::Pipe (*original_handler));
        return handler;
      }





      bool Pipe::check (Header& H, size_t num_axes) const
      {
        if (H.name() != "-")
          return false;

        H.name() = File::create_tempfile (0, "mif");

        return mrtrix_handler.check (H, num_axes);
      }




      RefPtr<Handler::Base> Pipe::create (Header& H) const
      {
        RefPtr<Handler::Base> original_handler (mrtrix_handler.create (H));
        RefPtr<Handler::Pipe> handler (new Handler::Pipe (*original_handler));
        return handler;
      }


    }
  }
}

