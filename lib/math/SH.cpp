/*******************************************************************************
    Copyright (C) 2014 Brain Research Institute, Melbourne, Australia
    
    Permission is hereby granted under the Patent Licence Agreement between
    the BRI and Siemens AG from July 3rd, 2012, to Siemens AG obtaining a
    copy of this software and associated documentation files (the
    "Software"), to deal in the Software without restriction, including
    without limitation the rights to possess, use, develop, manufacture,
    import, offer for sale, market, sell, lease or otherwise distribute
    Products, and to permit persons to whom the Software is furnished to do
    so, subject to the following conditions:
    
    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.
    
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
    OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
    CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
    TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
    SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*******************************************************************************/


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
        "Each volume in the output image corresponds to a different spherical harmonic "
        "component. Each volume will correspond to the following:\n"
        "volume 0: l = 0, m = 0\n"
        "volume 1: l = 2, m = -2 (imaginary part of m=2 SH)\n"
        "volume 2: l = 2, m = -1 (imaginary part of m=1 SH)\n"
        "volume 3: l = 2, m = 0\n"
        "volume 4: l = 2, m = 1 (real part of m=1 SH)\n"
        "volume 5: l = 2, m = 2 (real part of m=2 SH)\n"
        "etc...\n";


    }
  }
}

