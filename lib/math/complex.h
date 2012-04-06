/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 27/06/08.

    This file is part of MRtrix.

    MRtrix is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    MRtrix is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with MRtrix.  If not, see <http://www.gnu.org/licenses/>.

*/

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




