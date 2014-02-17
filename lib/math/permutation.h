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


#ifndef __math_permutation_h__
#define __math_permutation_h__

#include <gsl/gsl_permutation.h>
#include <gsl/gsl_permute_vector.h>

#include "math/vector.h"

namespace MR
{
  namespace Math
  {
    namespace
    {
      template <typename T> inline void permute (const gsl_permutation* p, Vector<T>& V);
      template <> inline void permute (const gsl_permutation* p, Vector<double>& V)
      {
        gsl_permute_vector (p, V.gsl());
      }
      template <> inline void permute (const gsl_permutation* p, Vector<float>& V)
      {
        gsl_permute_vector_float (p, V.gsl());
      }
    }

    class Permutation
    {
      public:
        Permutation (size_t n) {
          p = gsl_permutation_alloc (n);
        }
        ~Permutation () {
          gsl_permutation_free (p);
        }

        size_t operator[] (const size_t i) const {
          return (gsl_permutation_get (p, i));
        }
        size_t& operator[] (const size_t i)       {
          return (gsl_permutation_data (p) [i]);
        }

        template <typename T> Vector<T>& apply (Vector<T>& V) const {
          permute (p, V);
          return (V);
        }
        bool   valid () const {
          return (gsl_permutation_valid (p));
        }

        gsl_permutation* gsl () {
          return (p);
        }
        const gsl_permutation* gsl () const {
          return (p);
        }
      private:
        gsl_permutation* p;
    };

  }
}

#endif

