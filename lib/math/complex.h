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

#include <gsl/gsl_vector_complex_double.h>
#include <gsl/gsl_vector_complex_float.h>
#include <gsl/gsl_matrix_complex_double.h>
#include <gsl/gsl_matrix_complex_float.h>

#include "math/matrix.h"

namespace MR
{

  namespace Math
  {


    //! \cond skip

    template <typename T> class GSLVector;
    template <> class GSLVector <cfloat> : public gsl_vector_complex_float
    {
      public:
        void set (cfloat* p) {
          data = (float*) p;
        }
    };
    template <> class GSLVector <cdouble> : public gsl_vector_complex
    {
      public:
        void set (cdouble* p) {
          data = (double*) p;
        }
    };

    template <typename T> class GSLMatrix;
    template <> class GSLMatrix <cfloat> : public gsl_matrix_complex_float
    {
      public:
        void set (cfloat* p) {
          data = (float*) p;
        }
    };
    template <> class GSLMatrix <cdouble> : public gsl_matrix_complex
    {
      public:
        void set (cdouble* p) {
          data = (double*) p;
        }
    };

    template <typename T> class GSLBlock;
    template <> class GSLBlock <cfloat> : public gsl_block_complex_float
    {
      public:
        static gsl_block_complex_float* alloc (size_t n) {
          return (gsl_block_complex_float_alloc (n));
        }
        static void free (gsl_block_complex_float* p) {
          gsl_block_complex_float_free (p);
        }
    };
    template <> class GSLBlock <cdouble> : public gsl_block_complex
    {
      public:
        static gsl_block_complex* alloc (size_t n) {
          return (gsl_block_complex_alloc (n));
        }
        static void free (gsl_block_complex* p) {
          gsl_block_complex_free (p);
        }
    };


    // cdouble definitions:
    
    inline gsl_complex gsl (const cdouble a) { gsl_complex b; b.dat[0] = a.real(); b.dat[1] = a.imag(); return b; }
    inline gsl_complex_float gsl (const cfloat a) { gsl_complex_float b; b.dat[0] = a.real(); b.dat[1] = a.imag(); return b; }
    inline cdouble gsl (const gsl_complex a) { return cdouble (a.dat[0], a.dat[1]); }
    inline cfloat gsl (const gsl_complex_float a) { return cfloat (a.dat[0], a.dat[1]); }


    inline void gemm (CBLAS_TRANSPOSE op_A, CBLAS_TRANSPOSE op_B, cdouble alpha, const Matrix<cdouble>& A, const Matrix<cdouble>& B, cdouble beta, Matrix<cdouble>& C)
    {
      gsl_blas_zgemm (op_A, op_B, gsl(alpha), A.gsl(), B.gsl(), gsl(beta), C.gsl());
    }

    inline void gemv (CBLAS_TRANSPOSE op_A, cdouble alpha, const Matrix<cdouble>& A, const Vector<cdouble>& x, cdouble beta, Vector<cdouble>& y)
    {
      gsl_blas_zgemv (op_A, gsl(alpha), A.gsl(), x.gsl(), gsl(beta), y.gsl());
    }

    inline void symm (CBLAS_SIDE side, CBLAS_UPLO uplo, cdouble alpha, const Matrix<cdouble>& A, const Matrix<cdouble>& B, cdouble beta, Matrix<cdouble>& C)
    {
      gsl_blas_zsymm (side, uplo, gsl(alpha), A.gsl(), B.gsl(), gsl(beta), C.gsl());
    }

    inline void trsv (CBLAS_UPLO uplo, CBLAS_TRANSPOSE op_A, CBLAS_DIAG diag, const Matrix<cdouble>& A, Vector<cdouble>& x)
    {
      gsl_blas_ztrsv (uplo, op_A, diag, A.gsl(), x.gsl());
    }

    // float definitions:

    inline void gemm (CBLAS_TRANSPOSE op_A, CBLAS_TRANSPOSE op_B, cfloat alpha, const Matrix<cfloat>& A, const Matrix<cfloat>& B, cfloat beta, Matrix<cfloat>& C)
    {
      gsl_blas_cgemm (op_A, op_B, gsl(alpha), A.gsl(), B.gsl(), gsl(beta), C.gsl());
    }

    inline void gemv (CBLAS_TRANSPOSE op_A, cfloat alpha, const Matrix<cfloat>& A, const Vector<cfloat>& x, cfloat beta, Vector<cfloat>& y)
    {
      gsl_blas_cgemv (op_A, gsl(alpha), A.gsl(), x.gsl(), gsl(beta), y.gsl());
    }

    inline void symm (CBLAS_SIDE side, CBLAS_UPLO uplo, cfloat alpha, const Matrix<cfloat>& A, const Matrix<cfloat>& B, cfloat beta, Matrix<cfloat>& C)
    {
      gsl_blas_csymm (side, uplo, gsl(alpha), A.gsl(), B.gsl(), gsl(beta), C.gsl());
    }

    inline void trsv (CBLAS_UPLO uplo, CBLAS_TRANSPOSE op_A, CBLAS_DIAG diag, const Matrix<cfloat>& A, Vector<cfloat>& x)
    {
      gsl_blas_ctrsv (uplo, op_A, diag, A.gsl(), x.gsl());
    }


    //! \endcond

  }
}

#endif




