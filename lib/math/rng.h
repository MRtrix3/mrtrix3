#ifndef __math_simulation_h__
#define __math_simulation_h__

#include <gsl/gsl_rng.h>
#include <gsl/gsl_randist.h>
#include <sys/time.h>

#include "math/vector.h"

namespace MR
{
  namespace Math
  {

    class RNG
    {
      public:
        RNG () {
          generator = gsl_rng_alloc (gsl_rng_mt19937);
          struct timeval tv;
          gettimeofday (&tv, NULL);
          set (tv.tv_sec ^ tv.tv_usec);
        }
        RNG (size_t seed) {
          generator = gsl_rng_alloc (gsl_rng_mt19937);
          set (seed);
        }
        RNG (const RNG& rng) {
          generator = gsl_rng_alloc (gsl_rng_mt19937);
          set (rng.get()+1);
        }
        ~RNG ()           {
          gsl_rng_free (generator);
        }



        void set (size_t seed) {
          gsl_rng_set (generator, seed);
        }

        size_t get () const {
          return gsl_rng_get (generator);
        }


        gsl_rng*  operator() ()                    {
          return generator;
        }

        float uniform ()              {
          return gsl_rng_uniform (generator);
        }
        size_t uniform_int (size_t max) {
          return gsl_rng_uniform_int (generator, max);
        }
        float normal (float SD = 1.0) {
          return gsl_ran_gaussian (generator, SD);
        }
        float rician (float amplitude, float SD) {
          amplitude += gsl_ran_gaussian_ratio_method (generator, SD);
          float imag = gsl_ran_gaussian_ratio_method (generator, SD);
          return sqrt (amplitude*amplitude + imag*imag);
        }

        template <typename T> void shuffle (Vector<T>& V) {
          gsl_ran_shuffle (generator, V->ptr(), V.size(), sizeof (T));
        }
        template <class T> void shuffle (std::vector<T>& V) {
          gsl_ran_shuffle (generator, &V[0], V.size(), sizeof (T));
        }


      protected:
        gsl_rng* generator;
    };


    inline float cauchy (float x, float s)
    {
      x /= s;
      return (1.0 / (1.0 + x*x));
    }


  }
}

#endif

