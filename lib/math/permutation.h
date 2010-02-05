/*
   Copyright 2008 Brain Research Institute, Melbourne, Australia

   Written by J-Donald Tournier, 17/05/09.

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

#ifndef __math_permutation_h__
#define __math_permutation_h__

#include <gsl/gsl_permutation.h>
#include <gsl/gsl_permute_vector.h>

#include "math/vector.h"

namespace MR {
  namespace Math {
    namespace {
      template <typename T> inline void permute (const gsl_permutation* p, Vector<T>& V);
      template <> inline void permute (const gsl_permutation* p, Vector<double>& V) { gsl_permute_vector (p, V.gsl()); }
      template <> inline void permute (const gsl_permutation* p, Vector<float>& V) { gsl_permute_vector_float (p, V.gsl()); }
    }
    
    class Permutation {
      public:
	Permutation (size_t n) { p = gsl_permutation_alloc (n); }
	~Permutation () { gsl_permutation_free (p); }

        size_t operator[] (const size_t i) const { return (gsl_permutation_get (p, i)); }
        size_t& operator[] (const size_t i)       { return (gsl_permutation_data (p)[i]); }

        template <typename T> Vector<T>& apply (Vector<T>& V) const { permute (p, V); return (V); }
        bool   valid () const { return (gsl_permutation_valid (p)); }

	gsl_permutation* gsl () { return (p); }
	const gsl_permutation* gsl () const { return (p); }
      private:
	gsl_permutation* p;
    };

  }
}

#endif

