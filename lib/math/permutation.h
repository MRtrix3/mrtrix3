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

