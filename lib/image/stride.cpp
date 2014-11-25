#include "image/stride.h"

namespace MR
{
  namespace Image
  {

    namespace Stride
    {

      using namespace App;

      const OptionGroup StrideOption = OptionGroup ("Stride options")
      + Option ("stride",
                "specify the strides of the output data in memory, as a comma-separated list. "
                "The actual strides produced will depend on whether the output image "
                "format can support it.")
      + Argument ("spec").type_sequence_int();






      List& sanitise (List& strides, const List& ref)
      {
        // remove duplicates
        for (size_t i = 0; i < strides.size()-1; ++i) {
          if (!strides[i]) continue;
          for (size_t j = i+1; j < strides.size(); ++j) {
            if (!strides[j]) continue;
            if (Math::abs (strides[i]) == Math::abs (strides[j]))
              strides[j] = 0;
          }
        }

        ssize_t max = 0;
        for (size_t i = 0; i < strides.size(); ++i)
          if (Math::abs (strides[i]) > max)
            max = Math::abs (strides[i]);

        assert (max > 0);

        for (size_t i = 0; i < strides.size(); ++i) {
          if (!strides[i])
            strides[i] = (Math::abs (ref[i]) + max) * (ref[i] < 0 ? -1 : 1);
        }
        symbolise (strides);
        return strides;
      }



    }
  }
}


