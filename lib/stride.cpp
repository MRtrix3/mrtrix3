#include "stride.h"

namespace MR
{

  namespace Stride
  {

    using namespace App;

    const OptionGroup Options = OptionGroup ("Stride options")
      + Option ("stride",
          "specify the strides of the output data in memory, as a comma-separated list. "
          "The actual strides produced will depend on whether the output image "
          "format can support it.")
      + Argument ("spec").type_sequence_int();






    List& sanitise (List& current, const List& desired)
    {
      // remove duplicates
      for (size_t i = 0; i < current.size()-1; ++i) {
        if (!current[i]) continue;
        for (size_t j = i+1; j < current.size(); ++j) {
          if (!current[j]) continue;
          if (std::abs (current[i]) == std::abs (current[j]))
            current[j] = 0;
        }
      }

      ssize_t desired_max = 0;
      for (size_t i = 0; i < desired.size(); ++i)
        if (std::abs (desired[i]) > desired_max)
          desired_max = std::abs (desired[i]);

      ssize_t in_max = 0;
      for (size_t i = 0; i < current.size(); ++i)
        if (std::abs (current[i]) > in_max)
          in_max = std::abs (current[i]);
      in_max += desired_max + 1;

      for (size_t i = 0; i < current.size(); ++i) 
        if (desired[i]) 
          current[i] = desired[i];
        else if (current[i])
          current[i] += current[i] < 0 ? -desired_max : desired_max;
        else 
          current[i] = in_max++;

      symbolise (current);
      return current;
    }




  }
}


