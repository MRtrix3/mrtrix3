#include "app.h"
#include "image/stride.h"

namespace MR
{
  namespace Image
  {

    namespace Stride
    {

      using namespace App;

      const OptionGroup StrideOption = OptionGroup ("stride options")
      + Option ("stride",
                "specify the strides of the output data in memory, as a comma-separated list. "
                "The actual strides produced will depend on whether the output image "
                "format can support it.")
      + Argument ("spec").type_sequence_int();


      template <class InfoType> 
        void set_from_command_line (InfoType& info, const List& default_strides) 
        {
          Options opt = get_options ("stride");
          size_t n;
          if (opt.size()) {
            std::vector<int> strides = opt[0][0];
            if (strides.size() > info.ndim())
              WARN ("too many axes supplied to -stride option - ignoring remaining strides");
            for (n = 0; n < std::min (info.ndim(), strides.size()); ++n)
              info.stride(n) = strides[n];
            for (; n < info.ndim(); ++n)
              info.stride(n) = 0;
          } 
          else if (default_strides.size()) {
            for (n = 0; n < std::min (info.ndim(), default_strides.size()); ++n) 
              info.stride(n) = default_strides[n];
            for (; n < info.ndim(); ++n)
              info.stride(n) = 0;
          }
        }

    }
  }
}


