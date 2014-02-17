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

