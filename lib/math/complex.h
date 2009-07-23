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

#include <complex>
#include <gsl/gsl_vector_complex_double.h>
#include <gsl/gsl_vector_complex_float.h>
#include <gsl/gsl_matrix_complex_double.h>
#include <gsl/gsl_matrix_complex_float.h>

namespace MR {

  typedef std::complex<double> cdouble;
  typedef std::complex<float> cfloat;

  namespace Math {
    
#ifndef DOXYGEN_SHOULD_SKIP_THIS

    template <typename T> class GSLVector;
    template <> class GSLVector <cfloat> : public gsl_vector_complex_float { public: void set (cfloat* p) { data = (float*) p; } };
    template <> class GSLVector <cdouble> : public gsl_vector_complex { public: void set (cdouble* p) { data = (double*) p; } };

    template <typename T> class GSLMatrix;
    template <> class GSLMatrix <cfloat> : public gsl_matrix_complex_float { public: void set (cfloat* p) { data = (float*) p; } };
    template <> class GSLMatrix <cdouble> : public gsl_matrix_complex { public: void set (cdouble* p) { data = (double*) p; } };

    template <typename T> class GSLBlock;
    template <> class GSLBlock <cfloat> : public gsl_block_complex_float {
      public: 
        static gsl_block_complex_float* alloc (size_t n) { return (gsl_block_complex_float_alloc (n)); }
        static void free (gsl_block_complex_float* p) { gsl_block_complex_float_free (p); }
    };
    template <> class GSLBlock <cdouble> : public gsl_block_complex { 
      public: 
        static gsl_block_complex* alloc (size_t n) { return (gsl_block_complex_alloc (n)); }
        static void free (gsl_block_complex* p) { gsl_block_complex_free (p); }
    };

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

  }
}

#endif




