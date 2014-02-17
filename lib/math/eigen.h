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


#ifndef __math_eigen_h__
#define __math_eigen_h__

#include <gsl/gsl_eigen.h>
#include <gsl/gsl_sort_vector.h>
#include "math/matrix.h"
#include "math/permutation.h"

namespace MR
{
  namespace Math
  {
    namespace Eigen
    {

      /** @addtogroup linalg
        @{ */

      /** @defgroup eigen Eigensystems
        @{ */

      //! Eigenvalue decomposition for real symmetric matrices
      /** \note the diagonal and lower part of \a A will be destroyed during the decomposition */
      template <typename T> class Symm
      {
        public:
          Symm (size_t n) : work (gsl_eigen_symm_alloc (n)) { }
          ~Symm () {
            gsl_eigen_symm_free (work);
          }

          Vector<T>& operator () (Vector<T>& eval, Matrix<T>& A) {
            assert (A.rows() == A.columns());
            assert (A.rows() == eval.size());
            assert (A.rows() == work->size);
            int status = gsl_eigen_symm (A.gsl(), eval.gsl(), work);
            if (status) throw Exception (std::string ("eigenvalue decomposition failed: ") + gsl_strerror (status));
            return (eval);
          }
        protected:
          gsl_eigen_symm_workspace* work;
      };



      //! Eigenvalue and eigenvector decomposition for real symmetric matrices
      /** \note the diagonal and lower part of \a A will be destroyed during the decomposition */
      template <typename T> class SymmV
      {
        public:
          SymmV (size_t n) : work (gsl_eigen_symmv_alloc (n)), N (n) { }
          SymmV (const SymmV& SV) : work (gsl_eigen_symmv_alloc (SV.size())), N (SV.size()) { }
          ~SymmV () {
            gsl_eigen_symmv_free (work);
          }

          Vector<T>& operator () (Vector<T>& eval, Matrix<T>& A, Matrix<T>& evec) {
            assert (A.rows() == A.columns());
            assert (A.rows() == evec.rows());
            assert (evec.rows() == evec.columns());
            assert (A.rows() == eval.size());
            assert (A.rows() == work->size);
            int status = gsl_eigen_symmv (A.gsl(), eval.gsl(), evec.gsl(), work);
            if (status) throw Exception (std::string ("eigenvalue decomposition failed: ") + gsl_strerror (status));
            return (eval);
          }

          size_t size () const {
            return N;
          }

        protected:
          gsl_eigen_symmv_workspace* work;
          const size_t N;
      };


      //! Eigenvalue sorting
      inline Vector<double>& sort (Vector<double>& eval)
      {
        gsl_sort_vector (eval.gsl());
        return (eval);
      }
      //! Eigenvalue sorting
      template <typename T> inline Vector<T>& sort (Vector<T>& eval, Matrix<T>& evec)
      {
        gsl_eigen_symmv_sort (eval.gsl(), evec.gsl(), GSL_EIGEN_SORT_VAL_ASC);
        return (eval);
      }

      /** @} */
      /** @} */
    }
  }
}

#endif







