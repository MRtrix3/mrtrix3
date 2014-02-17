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




