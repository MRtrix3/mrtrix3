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


#ifndef __math_complex_h__
#define __math_complex_h__

#include <gsl/gsl_complex.h>
#include "types.h"

namespace MR
{
  namespace Math
  {

    inline gsl_complex gsl (const cdouble a) { gsl_complex b; b.dat[0] = a.real(); b.dat[1] = a.imag(); return b; }
    inline gsl_complex_float gsl (const cfloat a) { gsl_complex_float b; b.dat[0] = a.real(); b.dat[1] = a.imag(); return b; }
    inline cdouble gsl (const gsl_complex a) { return cdouble (a.dat[0], a.dat[1]); }
    inline cfloat gsl (const gsl_complex_float a) { return cfloat (a.dat[0], a.dat[1]); }

  }
}

#endif




