/* Copyright (c) 2008-2026 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Covered Software is provided under this License on an "as is"
 * basis, without warranty of any kind, either expressed, implied, or
 * statutory, including, without limitation, warranties that the
 * Covered Software is free of defects, merchantable, fit for a
 * particular purpose or non-infringing.
 * See the Mozilla Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
 */

#include "dwi/shgqi/kernel.h"

#include <array>
#include <cmath>

#include "exception.h"
#include "mrtrix.h"

namespace MR {
  namespace DWI {
    namespace SH_GQI {

      namespace {

        constexpr int    GL_ORDER     = 64;
        constexpr int    SERIES_TERMS = 30;
        constexpr double SMALL_A      = 0.5;
        constexpr double FOUR_PI      = 4.0 * M_PI;

        // Gauss-Legendre nodes/weights on [0, 1] (mapped from [-1, 1] via
        // s = (x+1)/2; weights halved). Computed by Newton iteration on the
        // Legendre polynomial of order GL_ORDER.
        struct GL_table {
          std::array<double, GL_ORDER> s;   // nodes in [0, 1]
          std::array<double, GL_ORDER> w;   // weights for ds in [0, 1]
        };

        const GL_table& gl_table()
        {
          static const GL_table tbl = []() {
            GL_table t;
            const int N = GL_ORDER;
            for (int i = 1; i <= (N + 1) / 2; ++i) {
              double z = std::cos (M_PI * (i - 0.25) / (N + 0.5));
              double dP = 0.0;
              for (int it = 0; it < 100; ++it) {
                double p1 = 1.0;
                double p2 = 0.0;
                for (int k = 1; k <= N; ++k) {
                  const double p3 = p2;
                  p2 = p1;
                  p1 = ((2.0 * k - 1.0) * z * p2 - (k - 1.0) * p3) / k;
                }
                dP = N * (z * p1 - p2) / (z * z - 1.0);
                const double dz = p1 / dP;
                z -= dz;
                if (std::abs (dz) < 1e-15)
                  break;
              }
              const double x = z;            // node on [-1, 1]
              const double w_full = 2.0 / ((1.0 - x * x) * dP * dP);
              // Map x -> s = (x+1)/2 on [0, 1]; weight halves.
              t.s[i - 1]     = 0.5 * (1.0 - x);
              t.s[N - i]     = 0.5 * (1.0 + x);
              t.w[i - 1]     = 0.5 * w_full;
              t.w[N - i]     = 0.5 * w_full;
            }
            return t;
          }();
          return tbl;
        }

        inline double phase (int l)
        {
          // (-1)^{l/2} for even l only; caller is responsible for the parity check.
          return ((l / 2) & 1) ? -1.0 : 1.0;
        }

        inline double double_factorial (int n)
        {
          double out = 1.0;
          while (n > 1) {
            out *= n;
            n -= 2;
          }
          return out;
        }

        //! 30-term power series K_l(a) for small a (a <= SMALL_A).
        //  K_l(a) = 4 pi (-1)^{l/2} sum_{k=0}^{N-1} (-1)^k a^{l+2k}
        //                            / [ 2^k k! (2l+2k+1)!! (l+2k+1) ]
        double K_l_series (int l, double a)
        {
          if (a == 0.0)
            return l == 0 ? FOUR_PI : 0.0;
          double sum  = 0.0;
          double a2k  = std::pow (a, l);          // starts at a^l (k=0)
          double sign = 1.0;                       // (-1)^k
          double pow2_k_factk = 1.0;               // 2^k * k! for k=0
          for (int k = 0; k < SERIES_TERMS; ++k) {
            const double dfact = double_factorial (2 * l + 2 * k + 1);
            const double denom = pow2_k_factk * dfact * (l + 2 * k + 1);
            sum += sign * a2k / denom;
            // advance to k+1
            a2k *= a * a;
            sign = -sign;
            pow2_k_factk *= 2.0 * (k + 1);
          }
          return FOUR_PI * phase (l) * sum;
        }

        //! Spherical Bessel j_n(x) for n = 0..lmax via Miller's backward
        //  recurrence; stable for all (n, x) with x > 0. The output length
        //  must be at least lmax+1.
        void j_n_all (double x, int lmax, double* out)
        {
          if (lmax < 0)
            throw Exception ("j_n_all: lmax must be non-negative");
          if (x == 0.0) {
            out[0] = 1.0;
            for (int l = 1; l <= lmax; ++l)
              out[l] = 0.0;
            return;
          }
          // Choose a starting index well above lmax so transients decay.
          const int n_top = lmax + std::max (20, int (std::ceil (2.0 * std::sqrt (double (lmax) * x)))) + 10;
          // Backward recurrence: j_{l-1}(x) = (2l+1)/x * j_l(x) - j_{l+1}(x)
          double j_lp1 = 0.0;
          double j_l   = 1.0;
          // Buffer the values we need (l = 0 .. lmax) on the way down.
          // We store unscaled values then renormalise.
          // A fixed-size automatic buffer keeps us off the heap.
          constexpr int MAX_BUF = 256;
          if (lmax + 1 > MAX_BUF)
            throw Exception ("j_n_all: lmax too large");
          double buf[MAX_BUF];
          for (int l = n_top; l >= 1; --l) {
            const double j_lm1 = (2.0 * l + 1.0) / x * j_l - j_lp1;
            if (l - 1 <= lmax)
              buf[l - 1] = j_lm1;
            j_lp1 = j_l;
            j_l   = j_lm1;
            // Rescale if magnitudes blow up to keep within IEEE range.
            if (std::abs (j_l) > 1e100) {
              const double r = 1e-100;
              j_l *= r;
              j_lp1 *= r;
              for (int k = 0; k <= lmax; ++k)
                buf[k] *= r;
            }
          }
          const double j0_true = std::sin (x) / x;
          const double scale   = j0_true / buf[0];
          for (int l = 0; l <= lmax; ++l)
            out[l] = buf[l] * scale;
        }

        //! 64-point Gauss-Legendre quadrature of K_l(a).
        //  Substitution t = a s, s in [0, 1] gives
        //      K_l(a) = 4 pi (-1)^{l/2} * sum_n w_n * j_l(a s_n).
        double K_l_gl (int l, double a)
        {
          if (a == 0.0)
            return l == 0 ? FOUR_PI : 0.0;
          const auto& g = gl_table();
          double acc = 0.0;
          double j_buf[256];
          for (int n = 0; n < GL_ORDER; ++n) {
            j_n_all (a * g.s[n], l, j_buf);
            acc += g.w[n] * j_buf[l];
          }
          return FOUR_PI * phase (l) * acc;
        }

        //! Vectorised GL path: for one a, evaluates j_l for ALL even l up to
        //  L_max in a single recurrence per Gauss node, then returns K_l(a)
        //  for each even l. Output length = L_max/2 + 1.
        void K_all_even_gl (int L_max, double a, double* out_even)
        {
          const int n_l = L_max / 2 + 1;
          if (a == 0.0) {
            out_even[0] = FOUR_PI;
            for (int r = 1; r < n_l; ++r)
              out_even[r] = 0.0;
            return;
          }
          const auto& g = gl_table();
          for (int r = 0; r < n_l; ++r)
            out_even[r] = 0.0;
          double j_buf[256];
          for (int n = 0; n < GL_ORDER; ++n) {
            j_n_all (a * g.s[n], L_max, j_buf);
            for (int r = 0; r < n_l; ++r)
              out_even[r] += g.w[n] * j_buf[2 * r];
          }
          for (int r = 0; r < n_l; ++r)
            out_even[r] *= FOUR_PI * phase (2 * r);
        }

        void K_all_even_series (int L_max, double a, double* out_even)
        {
          for (int r = 0; r < L_max / 2 + 1; ++r)
            out_even[r] = K_l_series (2 * r, a);
        }

      }  // anonymous namespace



      double K_l (int l, double a)
      {
        if (l < 0 || (l & 1))
          throw Exception ("SH_GQI::K_l requires non-negative even l (got " + str(l) + ")");
        if (a < 0.0)
          throw Exception ("SH_GQI::K_l requires non-negative argument (got a=" + str(a) + ")");
        return (a <= SMALL_A) ? K_l_series (l, a) : K_l_gl (l, a);
      }

      Eigen::VectorXd K_l (int l, const Eigen::Ref<const Eigen::VectorXd>& a)
      {
        if (l < 0 || (l & 1))
          throw Exception ("SH_GQI::K_l requires non-negative even l (got " + str(l) + ")");
        Eigen::VectorXd out (a.size());
        for (Eigen::Index i = 0; i < a.size(); ++i) {
          const double ai = a[i];
          if (ai < 0.0)
            throw Exception ("SH_GQI::K_l requires non-negative arguments");
          out[i] = (ai <= SMALL_A) ? K_l_series (l, ai) : K_l_gl (l, ai);
        }
        return out;
      }

      Eigen::MatrixXd K_matrix (int L_max,
                                const Eigen::Ref<const Eigen::VectorXd>& a)
      {
        if (L_max < 0 || (L_max & 1))
          throw Exception ("SH_GQI::K_matrix requires non-negative even L_max (got " + str(L_max) + ")");
        const int n_l = L_max / 2 + 1;
        Eigen::MatrixXd out (n_l, a.size());
        std::vector<double> col (n_l);
        for (Eigen::Index i = 0; i < a.size(); ++i) {
          const double ai = a[i];
          if (ai < 0.0)
            throw Exception ("SH_GQI::K_matrix requires non-negative arguments");
          if (ai <= SMALL_A)
            K_all_even_series (L_max, ai, col.data());
          else
            K_all_even_gl (L_max, ai, col.data());
          for (int r = 0; r < n_l; ++r)
            out (r, i) = col[r];
        }
        return out;
      }

    }
  }
}
