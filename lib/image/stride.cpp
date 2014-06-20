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
            if (abs (strides[i]) == abs (strides[j])) 
              strides[j] = 0;
          }
        }

        ssize_t max = 0;
        for (size_t i = 0; i < strides.size(); ++i)
          if (ref[i] && abs (strides[i]) > max)
            max = abs (strides[i]);

        assert (max > 0);

        for (size_t i = 0; i < strides.size(); ++i) {
          if (!ref[i]) {
            ++max;
            strides[i] = strides[i]<0 ? -max : max;
          }
        }
        symbolise (strides);
        return strides;
      }



    }
  }
}


