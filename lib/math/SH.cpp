#include "math/SH.h"

namespace MR
{
  namespace Math
  {
    namespace SH
    {

      const char* encoding_description =
        "The spherical harmonic coefficients are stored as follows. First, since the "
        "signal attenuation profile is real, it has conjugate symmetry, i.e. Y(l,-m) "
        "= Y(l,m)* (where * denotes the complex conjugate). Second, the diffusion "
        "profile should be antipodally symmetric (i.e. S(x) = S(-x)), implying that "
        "all odd l components should be zero. Therefore, only the even elements are "
        "computed.\n"
        "Note that the spherical harmonics equations used here differ slightly from "
        "those conventionally used, in that the (-1)^m factor has been omitted. This "
        "should be taken into account in all subsequent calculations.\n"
        "Each study in the output image corresponds to a different spherical harmonic "
        "component. Each study will correspond to the following:\n"
        "study 0: l = 0, m = 0\n"
        "study 1: l = 2, m = 0\n"
        "study 2: l = 2, m = 1, real part\n"
        "study 3: l = 2, m = 1, imaginary part\n"
        "study 4: l = 2, m = 2, real part\n"
        "study 5: l = 2, m = 2, imaginary part\n"
        "etc...\n";


    }
  }
}

