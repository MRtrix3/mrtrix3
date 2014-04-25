/*
    Copyright 2013 KU Leuven, Dept. Electrical Engineering, Medical Image Computing
    Herestraat 49 box 7003, 3000 Leuven, Belgium
    
    Written by Daan Christiaens, 16/04/14.
    
    This file is part of the Global Tractography module for MRtrix.
    
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

#ifndef __math_lsnonneg_h__
#define __math_lsnonneg_h__

#include <vector>

#include "math/vector.h"
#include "math/matrix.h"


namespace MR {
  namespace Math {
    
    template <typename ValueType> 
    inline Vector<ValueType>& solve_LS_nonneg (Vector<ValueType>& x, const Matrix<ValueType>& M, const Vector<ValueType>& b)
    {
      Matrix<ValueType> H;
      mult(H, 1.0, CblasTrans, M, CblasNoTrans, M);
      Vector<ValueType> f;
      mult(f, -1.0, CblasTrans, M, b);
      return solve_LS_nonneg_Hf (x, H, f);
    }
    
    /**
     * Non-negative Least-Squares solver,
     * based on Franc, Navara, and Hlavac, "Sequential coordinate-wise algorithm 
     * for non-negative least squares problem." (2005)
     * 
     * Solves A x = b, s.t. x >= 0, given H = A^T A and f = -A^T b.
     * Note the minus sign in f!
     * 
     * The input/output argument x can be used to set the initialization.
     */
    template <typename ValueType> 
    inline Vector<ValueType>& solve_LS_nonneg_Hf (Vector<ValueType>& x, const Matrix<ValueType>& H, const Vector<ValueType>& f)
    {
      Vector<ValueType> mu (f);
      if (!x.is_set()) {
        x.allocate(f);
        x.zero();
      }
      else {
        mult(mu, 1.0, 1.0, CblasTrans, H, x);
      }
      for (int it = 0; it < 1000; it++)
      {
        for (int k = 0; k < f.size(); k++)
        {
          ValueType t = x[k] - (mu[k] / H(k,k));
          ValueType x1 = (t < 0.0) ? 0.0 : t;
          if (x1 != x[k]) {
            ValueType d = x1 - x[k];
            x[k] = x1;
            for (int l = 0; l < f.size(); l++)
              mu[l] += d * H(l,k);
          }
        }
        if (Math::absmax(mu) < 1e-4) {
          break;
        }
      }
      return x;
    }
    
//    /**
//     * Non-negative Least-Squares solver,
//     * optimized or the case of a symmetric 3x3 matrix H = A^T A.
//     * 
//     * Solves A x = b, s.t. x >= 0, given H = A^T A and f = -A^T b.
//     * 
//     * Input L is the cholesky decomposition of H (lower triangular matrix).
//     */
//    template <typename ValueType> 
//    inline Vector<ValueType>& solve_LS_nonneg3_Hf (Vector<ValueType>& x, const Matrix<ValueType>& L, const Vector<ValueType>& f)
//    {
//      x.allocate(3);
//      ValueType f0 (f[0]), f1 (f[1]), f2 (f[2]), l00 (L(0,0)), l10 (L(1,0)), l11 (L(1,1)), l20 (L(2,0)), l21 (L(2,1)), l22 (L(2,2));
//      ValueType y0 = f0 / l00;
//      ValueType y1 = (f1 - l10 * f0) / l11;
//      ValueType y2 = (f2 - l20 * f0 - l21 * f1) / l22;
//      ValueType x2 = y2 / l22;
//      ValueType x1 = (y1 - l21 * y2) / l11;
//      ValueType x0 = (y0 - l20 * y2 - l10 * y1) / l00;
//      if ((x0 >= 0.0) && (x1 >= 0.0) && (x2 >= 0.0)) {
//        x[0] = x0;
//        x[1] = x1;
//        x[2] = x2;
//        return x;
//      }
//      // else
      
//    }
    
    /**
     * Non-negative Least-Squares solver,
     * optimized or the case of a symmetric 3x3 matrix H = A^T A.
     * 
     * Solves A x = b, s.t. x >= 0, given H = A^T A and f = A^T b.
     * 
     */
    template <typename ValueType> 
    inline Vector<ValueType>& solve_LS_nonneg3_Hf (Vector<ValueType>& x, const Matrix<ValueType>& H, const Matrix<ValueType>& Hinv, const Vector<ValueType>& f)
    {
      x.allocate(3);
      mult(x, Hinv, f);
      if ((x[0] >= 0.0) && (x[1] >= 0.0) && (x[2] >= 0.0))
        return x;
      // else
      ValueType mu = 1.0 / 0.0; // +Infinity
      ValueType a, b, d, s, mu0, x0, x1;
      // case x=0
      a = H(1,1);
      b = H(2,1);
      d = H(2,2);
      s = 1.0 / (a*d - b*b);
      x0 = s * ( d*f[1] - b*f[2] );
      x1 = s * (-b*f[1] + a*f[2] );
//      mu0 = f[0] - H(1,0)*x0 - H(2,0)*x1;
//      mu0 = mu0*mu0;
      mu0 = -(x0 * f[1] + x1 * f[2])/2;
      if ((x0 >= 0.0) && (x1 >= 0.0)) { // && (mu0 >= 0.0)) {
        x[0] = 0.0;
        x[1] = x0;
        x[2] = x1;
        mu = mu0;
      }
      // case y=0
      a = H(0,0);
      b = H(2,0);
      d = H(2,2);
      s = 1.0 / (a*d - b*b);
      x0 = s * ( d*f[0] - b*f[2] );
      x1 = s * (-b*f[0] + a*f[2] );
//      mu0 = f[1] - H(1,1)*x0 - H(2,1)*x1;
//      mu0 = mu0*mu0;
      mu0 = -(x0 * f[0] + x1 * f[2])/2;
      if ((x0 >= 0.0) && (x1 >= 0.0) && (mu0 < mu)) {
        x[0] = x0;
        x[1] = 0.0;
        x[2] = x1;
        mu = mu0;
      }
      // case z=0
      a = H(0,0);
      b = H(1,0);
      d = H(1,1);
      s = 1.0 / (a*d - b*b);
      x0 = s * ( d*f[0] - b*f[1] );
      x1 = s * (-b*f[0] + a*f[1] );
//      mu0 = f[2] - H(2,1)*x0 - H(2,2)*x1;
//      mu0 = mu0*mu0;
      mu0 = -(x0 * f[0] + x1 * f[1])/2;
      if ((x0 >= 0.0) && (x1 >= 0.0) && (mu0 < mu)) {
        x[0] = x0;
        x[1] = x1;
        x[2] = 0.0;
        mu = mu0;
      }
      //
//      if ((x[0] >= 0.0) && (x[1] >= 0.0) && (x[2] >= 0.0))
//        return x;
      // else
      // case x=0, y=0
      x0 = f[2] / H(2,2);
//      mu0 = f[0] - H(2,0)*x0;
//      mu1 = f[1] - H(2,1)*x0;
//      mu0 = -mu0;
//      mu1 = -mu1;
      mu0 = -x0 * f[2] / 2;
      if ((x0 >= 0.0) && (mu0 < mu)) { // && (mu0 >= 0.0) && (mu1 >= 0.0)) {
        x[0] = 0.0;
        x[1] = 0.0;
        x[2] = x0;
        mu = mu0; //(mu0*mu0 + mu1*mu1);
      }
      // case x=0, z=0
      x0 = f[1] / H(1,1);
//      mu0 = f[0] - H(1,0)*x0;
//      mu1 = f[2] - H(2,1)*x0;
//      mu0 = -mu0;
//      mu1 = -mu1;
      mu0 = -x0 * f[1] / 2;
      if ((x0 >= 0.0) && (mu0 < mu)) { //((mu0*mu0 + mu1*mu1) < mu)) {
        x[0] = 0.0;
        x[1] = x0;
        x[2] = 0.0;
        mu = mu0;//(mu0*mu0 + mu1*mu1);
      }
      // case y=0, z=0
      x0 = f[0] / H(0,0);
//      mu0 = f[1] - H(1,0)*x0;
//      mu1 = f[2] - H(2,0)*x0;
//      mu0 = -mu0;
//      mu1 = -mu1;
      mu0 = -x0 * f[0] / 2;
      if ((x0 >= 0.0) && (mu0 < mu)) {
        x[0] = x0;
        x[1] = 0.0;
        x[2] = 0.0;
        mu = mu0;//(mu0*mu0 + mu1*mu1);
      }
      //
      if ((x[0] >= 0.0) && (x[1] >= 0.0) && (x[2] >= 0.0))
        return x;
      // finally
      x = 0.0;
      return x;
    }
    
    
    
    
    
//    /**
//     * Active set Non-negative Least-Squares solver.
//     * 
//     * H = A^T A and c = A^T b
//     */
//    template <typename ValueType> 
//    inline Vector<ValueType>& solve_LS_nonneg_AS (Vector<ValueType>& x, const Matrix<ValueType>& H, const Vector<ValueType>& c)
//    {
//      size_t n = H.columns();
//      std::vector<bool> P (n, false);
//      size_t p = 0;
//      // set x to origin
//      x.allocate(n);
//      x.zero();
//      // compute minus gradient at origin
//      Vector<ValueType> w (c);
//      mult(w, 1.0, -1.0, CblasNoTrans, H, x);
//      // find max index
//      size_t argmax = 0;
//      ValueType maxval = w[0];
//      for (size_t k = 0; k < n; k++) {
//        if (!P[k] && (w[k] > maxval)) {
//          maxval = w[k];
//          argmax = k;
//        }
//      }
//      // temporary variables
//      Math::Matrix<ValueType> HP;
//      Math::Vector<ValueType> cP, sP;
//      size_t i, j;
//      // main loop
//      TRACE;
//      while ((p < n) && (maxval > 1e-3))
//      {
//        // include idx in P
//        P[argmax] = true;
//        p++;
//        // calc sP
//        i = 0;
//        HP.allocate(p, p);
//        cP.allocate(p);
//        for (size_t k = 0; k < n; k++) {
//          if (P[k]) {
//            cP[i] = c[k];
//            j = 0;
//            for (size_t l = 0; l < n; l++) {
//              if (P[l])
//                HP(i,j++) = H(k,l);
//            }
//            i++;
//          }
//        }
//        Cholesky::decomp(HP);
//        sP.allocate(p);
//        Cholesky::solve(sP, HP, cP);
//        // inner loop
//        while (min(sP) <= 0.0) {
//          // calc alpha
//          ValueType alpha = 0.0, t;
//          size_t i = 0;
//          for (size_t k = 0; k < n; k++) {
//            if (P[k]) {
//              t = x[k]/(x[k]-sP[i]);
//              alpha = (i++) ? ((t < alpha) ? t : alpha) : t;
//            }
//          }
//          // update x
//          i = 0;
//          for (size_t k = 0; k < n; k++) {
//            if (P[k])
//              x[k] -= alpha*(sP[i++]-x[k]);
//            else
//              x[k] += alpha*x[k];
//          }
//          // update passive set P
//          i = 0;
//          for (size_t k = 0; k < n; k++) {
//            if (P[k] && (x[k] == 0.0)) {
//              P[k] = false;
//              p--;
//            }
//          }
//          // update sP
//          i = 0, j = 0;
//          HP.allocate(p, p);
//          cP.allocate(p);
//          for (size_t k = 0; k < n; k++) {
//            if (P[k]) {
//              cP[i] = c[k];
//              j = 0;
//              for (size_t l = 0; l < n; l++) {
//                if (P[l])
//                  HP(i,j++) = H(k,l);
//              }
//              i++;
//            }
//          }
//          sP.allocate(p);
//          Cholesky::decomp(HP);
//          Cholesky::solve(sP, HP, cP);
//        }
//        // update x
//        i = 0;
//        for (size_t k = 0; k < n; k++)
//          x[k] = (P[k]) ? sP[i++] : 0.0;
//        // compute minus gradient
//        w = c;
//        mult(w, 1.0, -1.0, CblasNoTrans, H, x);
//        // find max index
//        argmax = 0;
//        maxval = w[0];
//        for (size_t k = 0; k < n; k++) {
//          if (!P[k] && (w[k] > maxval)) {
//            maxval = w[k];
//            argmax = k;
//          }
//        }
        
//        VAR(P);
//        VAR(w);
//        VAR(x);
//        VAR(maxval);
//        VAR(argmax);
//      }
      
//      return x;
//    }
    
    /**
     * Active set Non-negative Least-Squares solver.
     * 
     * H = A^T A and c = A^T b
     */
    template <typename ValueType> 
    inline Vector<ValueType>& solve_LS_nonneg_AS (Vector<ValueType>& x, const Matrix<ValueType>& H, const Vector<ValueType>& c)
    {
      size_t n = H.columns();
      std::vector<bool> P (n, false);
      size_t p = 0;
      // set x to origin
      x.allocate(n);
      x.zero();
      // compute minus gradient at origin
      Vector<ValueType> w (c);
      mult(w, 1.0, -1.0, CblasNoTrans, H, x);
      // find max index
      size_t argmax = 0;
      ValueType maxval = w[0];
      for (size_t k = 0; k < n; k++) {
        if (!P[k] && (w[k] > maxval)) {
          maxval = w[k];
          argmax = k;
        }
      }
      // temporary variables
      Math::Matrix<ValueType> HP;
      Math::Vector<ValueType> cP, sP;
      size_t i, j;
      // main loop
      TRACE;
      while ((p < n) && (maxval > 1e-5))
      {
        // include idx in P
        P[argmax] = true;
        p++;
        // calc sP
        i = 0;
        HP.allocate(p, p);
        cP.allocate(p);
        for (size_t k = 0; k < n; k++) {
          if (P[k]) {
            cP[i] = c[k];
            j = 0;
            for (size_t l = 0; l < n; l++) {
              if (P[l])
                HP(i,j++) = H(k,l);
            }
            i++;
          }
        }
        Cholesky::decomp(HP);
        sP.allocate(p);
        Cholesky::solve(sP, HP, cP);
        // inner loop
        while (min(sP) <= 0.0) {
            
          VAR(P);
          VAR(sP);
          VAR(x);
            
          // calc alpha
          ValueType alpha = 0.0, t;
          size_t i = 0;
          for (size_t k = 0; k < n; k++) {
            if (P[k]) {
              if (sP[i] < 0.0) {
                t = x[k]/(x[k]-sP[i]);
                alpha = (i++) ? ((t < alpha) ? t : alpha) : t;
              }
            }
          }
          // update x
          i = 0;
          for (size_t k = 0; k < n; k++) {
            if (P[k])
              x[k] += alpha*(sP[i++]-x[k]);
            else
              x[k] -= alpha*x[k];
          }
          // update passive set P
          i = 0;
          for (size_t k = 0; k < n; k++) {
            if (P[k]) {
              if ((sP[i++] < 0.0) && (x[k] == 0.0)) {
                P[k] = false;
                p--;
              }
            }
          }
          
          VAR(P);
          
          // update sP
          i = 0, j = 0;
          HP.allocate(p, p);
          cP.allocate(p);
          for (size_t k = 0; k < n; k++) {
            if (P[k]) {
              cP[i] = c[k];
              j = 0;
              for (size_t l = 0; l < n; l++) {
                if (P[l])
                  HP(i,j++) = H(k,l);
              }
              i++;
            }
          }
          sP.allocate(p);
          Cholesky::decomp(HP);
          Cholesky::solve(sP, HP, cP);
        }
        // update x
        i = 0;
        for (size_t k = 0; k < n; k++)
          x[k] = (P[k]) ? sP[i++] : 0.0;
        // compute minus gradient
        w = c;
        mult(w, 1.0, -1.0, CblasNoTrans, H, x);
        // find max index
        argmax = 0;
        maxval = w[0];
        for (size_t k = 0; k < n; k++) {
          if (!P[k] && (w[k] > maxval)) {
            maxval = w[k];
            argmax = k;
          }
        }
        
        VAR(P);
        VAR(w);
        VAR(x);
        VAR(maxval);
        VAR(argmax);
      }
      
      return x;
    }
    
    
  }
}

#endif // __math_lsnonneg_h__
