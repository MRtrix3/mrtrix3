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
            if (std::abs (strides[i]) == std::abs (strides[j])) 
              strides[j] = 0;
          }
        }

        ssize_t ref_max = 0;
        for (size_t i = 0; i < ref.size(); ++i)
          if (std::abs (ref[i]) > ref_max)
            ref_max = std::abs (ref[i]);

        ssize_t in_max = 0;
        for (size_t i = 0; i < strides.size(); ++i)
          if (std::abs (strides[i]) > in_max)
            in_max = std::abs (strides[i]);
        in_max += ref_max + 1;

        for (size_t i = 0; i < strides.size(); ++i) 
          if (ref[i]) 
            strides[i] = ref[i];
          else if (strides[i])
            strides[i] += strides[i] < 0 ? -ref_max : ref_max;
          else 
            strides[i] = in_max++;
        
        symbolise (strides);
        return strides;
      }



    }
  }
}


