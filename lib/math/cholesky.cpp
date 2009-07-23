/* Cholesky Decomposition
 *
 * Copyright (C) 2000  Thomas Walter
 *
 * 03 May 2000: Modified for GSL by Brian Gough
 * 29 Jul 2005: Additions by Gerard Jungman
 * Copyright (C) 2000,2001, 2002, 2003, 2005, 2007 Brian Gough, Gerard Jungman
 *
 * This is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 3, or (at your option) any
 * later version.
 *
 * This source is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 */

/*
 * Cholesky decomposition of a symmetrix positive definite matrix.
 * This is useful to solve the matrix arising in
 *    periodic cubic splines
 *    approximating splines
 *
 * This algorithm does:
 *   A = L * L'
 * with
 *   L  := lower left triangle matrix
 *   L' := the transposed form of L.
 *
 */

/* modified 17/05/2009 J-Donald Tournier (d.tournier@brain.org.au) 
   to provide single-precision equivalent functions */

#include <gsl/gsl_math.h>
#include <gsl/gsl_errno.h>
#include <gsl/gsl_vector_float.h>
#include <gsl/gsl_matrix_float.h>
#include <gsl/gsl_blas.h>
#include <gsl/gsl_linalg.h>

namespace MR {
  namespace Math {
    namespace Cholesky {

      /* avoids runtime error, for checking matrix for positive definiteness */
      static inline float quiet_sqrt (float x)  { return (x >= 0) ? sqrtf(x) : GSL_NAN; }

      int gsl_linalg_cholesky_decomp (gsl_matrix_float * A)
      {
        const size_t M = A->size1;
        const size_t N = A->size2;

        if (M != N)
        {
          GSL_ERROR("cholesky decomposition requires square matrix", GSL_ENOTSQR);
        }
        else
        {
          size_t i,j,k;
          int status = 0;

          /* Do the first 2 rows explicitly.  It is simple, and faster.  And
           * one can return if the matrix has only 1 or 2 rows.  
           */

          float A_00 = gsl_matrix_float_get (A, 0, 0);

          float L_00 = quiet_sqrt(A_00);

          if (A_00 <= 0)
          {
            status = GSL_EDOM ;
          }

          gsl_matrix_float_set (A, 0, 0, L_00);

          if (M > 1)
          {
            float A_10 = gsl_matrix_float_get (A, 1, 0);
            float A_11 = gsl_matrix_float_get (A, 1, 1);

            float L_10 = A_10 / L_00;
            float diag = A_11 - L_10 * L_10;
            float L_11 = quiet_sqrt(diag);

            if (diag <= 0)
            {
              status = GSL_EDOM;
            }

            gsl_matrix_float_set (A, 1, 0, L_10);        
            gsl_matrix_float_set (A, 1, 1, L_11);
          }

          for (k = 2; k < M; k++)
          {
            float A_kk = gsl_matrix_float_get (A, k, k);

            for (i = 0; i < k; i++)
            {
              float sum = 0;

              float A_ki = gsl_matrix_float_get (A, k, i);
              float A_ii = gsl_matrix_float_get (A, i, i);

              gsl_vector_float_view ci = gsl_matrix_float_row (A, i);
              gsl_vector_float_view ck = gsl_matrix_float_row (A, k);

              if (i > 0) {
                gsl_vector_float_view di = gsl_vector_float_subvector(&ci.vector, 0, i);
                gsl_vector_float_view dk = gsl_vector_float_subvector(&ck.vector, 0, i);

                gsl_blas_sdot (&di.vector, &dk.vector, &sum);
              }

              A_ki = (A_ki - sum) / A_ii;
              gsl_matrix_float_set (A, k, i, A_ki);
            } 

            {
              gsl_vector_float_view ck = gsl_matrix_float_row (A, k);
              gsl_vector_float_view dk = gsl_vector_float_subvector (&ck.vector, 0, k);

              float sum = gsl_blas_snrm2 (&dk.vector);
              float diag = A_kk - sum * sum;

              float L_kk = quiet_sqrt(diag);

              if (diag <= 0)
              {
                status = GSL_EDOM;
              }

              gsl_matrix_float_set (A, k, k, L_kk);
            }
          }

          /* Now copy the transposed lower triangle to the upper triangle,
           * the diagonal is common.  
           */

          for (i = 1; i < M; i++)
          {
            for (j = 0; j < i; j++)
            {
              float A_ij = gsl_matrix_float_get (A, i, j);
              gsl_matrix_float_set (A, j, i, A_ij);
            }
          } 

          if (status == GSL_EDOM)
          {
            GSL_ERROR ("matrix must be positive definite", GSL_EDOM);
          }

          return GSL_SUCCESS;
        }
      }




      int gsl_linalg_cholesky_solve (const gsl_matrix_float * LLT, const gsl_vector_float * b, gsl_vector_float * x)
      {
        if (LLT->size1 != LLT->size2)
        {
          GSL_ERROR ("cholesky matrix must be square", GSL_ENOTSQR);
        }
        else if (LLT->size1 != b->size)
        {
          GSL_ERROR ("matrix size must match b size", GSL_EBADLEN);
        }
        else if (LLT->size2 != x->size)
        {
          GSL_ERROR ("matrix size must match solution size", GSL_EBADLEN);
        }
        else
        {
          /* Copy x <- b */

          gsl_vector_float_memcpy (x, b);

          /* Solve for c using forward-substitution, L c = b */

          gsl_blas_strsv (CblasLower, CblasNoTrans, CblasNonUnit, LLT, x);

          /* Perform back-substitution, U x = c */

          gsl_blas_strsv (CblasUpper, CblasNoTrans, CblasNonUnit, LLT, x);


          return GSL_SUCCESS;
        }
      }





      int gsl_linalg_cholesky_svx (const gsl_matrix_float * LLT, gsl_vector_float * x)
      {
        if (LLT->size1 != LLT->size2)
        {
          GSL_ERROR ("cholesky matrix must be square", GSL_ENOTSQR);
        }
        else if (LLT->size2 != x->size)
        {
          GSL_ERROR ("matrix size must match solution size", GSL_EBADLEN);
        }
        else
        {
          /* Solve for c using forward-substitution, L c = b */

          gsl_blas_strsv (CblasLower, CblasNoTrans, CblasNonUnit, LLT, x);

          /* Perform back-substitution, U x = c */

          gsl_blas_strsv (CblasUpper, CblasNoTrans, CblasNonUnit, LLT, x);

          return GSL_SUCCESS;
        }
      }

      /*
         gsl_linalg_cholesky_invert()
         Compute the inverse of a symmetric positive definite matrix in
         Cholesky form.

Inputs: LLT - matrix in cholesky form on input
A^{-1} = L^{-t} L^{-1} on output

Return: success or error
       */

      int gsl_linalg_cholesky_invert(gsl_matrix_float * LLT)
      {
        if (LLT->size1 != LLT->size2)
        {
          GSL_ERROR ("cholesky matrix must be square", GSL_ENOTSQR);
        }
        else
        {
          size_t N = LLT->size1;
          size_t i, j;
          float sum;
          gsl_vector_float_view v1, v2;

          /* invert the lower triangle of LLT */
          for (i = 0; i < N; ++i)
          {
            float ajj;

            j = N - i - 1;

            gsl_matrix_float_set(LLT, j, j, 1.0 / gsl_matrix_float_get(LLT, j, j));
            ajj = -gsl_matrix_float_get(LLT, j, j);

            if (j < N - 1)
            {
              gsl_matrix_float_view m;

              m = gsl_matrix_float_submatrix(LLT, j + 1, j + 1,
                  N - j - 1, N - j - 1);
              v1 = gsl_matrix_float_subcolumn(LLT, j, j + 1, N - j - 1);

              gsl_blas_strmv(CblasLower, CblasNoTrans, CblasNonUnit,
                  &m.matrix, &v1.vector);

              gsl_blas_sscal(ajj, &v1.vector);
            }
          } /* for (i = 0; i < N; ++i) */

          /*
           * The lower triangle of LLT now contains L^{-1}. Now compute
           * A^{-1} = L^{-t} L^{-1}
           *
           * The (ij) element of A^{-1} is column i of L^{-1} dotted into
           * column j of L^{-1}
           */

          for (i = 0; i < N; ++i)
          {
            for (j = i + 1; j < N; ++j)
            {
              v1 = gsl_matrix_float_subcolumn(LLT, i, j, N - j);
              v2 = gsl_matrix_float_subcolumn(LLT, j, j, N - j);

              /* compute Ainv_{ij} = sum_k Linv_{ki} Linv_{kj} */
              gsl_blas_sdot(&v1.vector, &v2.vector, &sum);

              /* store in upper triangle */
              gsl_matrix_float_set(LLT, i, j, sum);
            }

            /* now compute the diagonal element */
            v1 = gsl_matrix_float_subcolumn(LLT, i, i, N - i);
            gsl_blas_sdot(&v1.vector, &v1.vector, &sum);
            gsl_matrix_float_set(LLT, i, i, sum);
          }

          /* copy the transposed upper triangle to the lower triangle */

          for (j = 1; j < N; j++)
          {
            for (i = 0; i < j; i++)
            {
              float A_ij = gsl_matrix_float_get (LLT, i, j);
              gsl_matrix_float_set (LLT, j, i, A_ij);
            }
          } 

          return GSL_SUCCESS;
        }
      } /* gsl_linalg_cholesky_invert() */





      int gsl_linalg_cholesky_decomp_unit(gsl_matrix_float * A, gsl_vector_float * D)
      {
        const size_t N = A->size1;
        size_t i, j;

        /* initial Cholesky */
        int stat_chol = gsl_linalg_cholesky_decomp(A);

        if(stat_chol == GSL_SUCCESS)
        {
          /* calculate D from diagonal part of initial Cholesky */
          for(i = 0; i < N; ++i)
          {
            const float C_ii = gsl_matrix_float_get(A, i, i);
            gsl_vector_float_set(D, i, C_ii*C_ii);
          }

          /* multiply initial Cholesky by 1/sqrt(D) on the right */
          for(i = 0; i < N; ++i)
          {
            for(j = 0; j < N; ++j)
            {
              gsl_matrix_float_set(A, i, j, gsl_matrix_float_get(A, i, j) / sqrtf(gsl_vector_float_get(D, j)));
            }
          }

          /* Because the initial Cholesky contained both L and transpose(L),
             the result of the multiplication is not symmetric anymore;
             but the lower triangle _is_ correct. Therefore we reflect
             it to the upper triangle and declare victory.
           */
          for(i = 0; i < N; ++i)
            for(j = i + 1; j < N; ++j)
              gsl_matrix_float_set(A, i, j, gsl_matrix_float_get(A, j, i));
        }

        return stat_chol;

      }
    }
  }
}
