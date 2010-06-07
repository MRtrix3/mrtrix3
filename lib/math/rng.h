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

#ifndef __math_simulation_h__
#define __math_simulation_h__

#include <gsl/gsl_rng.h>
#include <gsl/gsl_randist.h>

#include "math/vector.h"

namespace MR {
  namespace Math {

    class RNG {
      protected:
        gsl_rng* generator;

      public:
        RNG ()            { generator = gsl_rng_alloc (gsl_rng_mt19937); gsl_rng_set (generator, time (NULL)); }
        RNG (size_t seed) { generator = gsl_rng_alloc (gsl_rng_mt19937); gsl_rng_set (generator, seed); }
        ~RNG ()           { gsl_rng_free (generator); }



        void set_seed (size_t seed) { gsl_rng_set (generator, seed); }


        gsl_rng*  operator() ()                    { return (generator); }

        float uniform ()              { return (gsl_rng_uniform (generator)); }
        size_t uniform_int (size_t max) { return (gsl_rng_uniform_int (generator, max)); }
        float normal (float SD = 1.0) { return (gsl_ran_gaussian (generator, SD)); }
        float rician (float amplitude, float SD)
        {
          amplitude += gsl_ran_gaussian_ratio_method (generator, SD);
          float imag = gsl_ran_gaussian_ratio_method (generator, SD);
          return (sqrt (amplitude*amplitude + imag*imag));
        }

        template <typename T> void shuffle (Vector<T>& V) { gsl_ran_shuffle (generator, V->ptr(), V.size(), sizeof (T)); }
        template <class T> void shuffle (std::vector<T>& V) { gsl_ran_shuffle (generator, &V[0], V.size(), sizeof (T)); }
    };


    inline float cauchy (float x, float s) { x /= s; return (1.0 / (1.0 + x*x)); }


  }
}

#endif

