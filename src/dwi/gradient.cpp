#include "dwi/gradient.h"

namespace MR
{
  namespace DWI
  {

    using namespace App;

    const OptionGroup GradOption = OptionGroup ("DW gradient encoding options")
      + Option ("grad",
          "specify the diffusion-weighted gradient scheme used in the acquisition. "
          "The program will normally attempt to use the encoding stored in the image "
          "header. This should be supplied as a 4xN text file with each line is in "
          "the format [ X Y Z b ], where [ X Y Z ] describe the direction of the "
          "applied gradient, and b gives the b-value in units of s/mm^2.")
        + Argument ("encoding").type_file();

  }
}


