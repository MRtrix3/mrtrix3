/* Copyright (c) 2008-2023 the MRtrix3 contributors.
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


#include "math/zstatistic.h"

#include "math/betainc.h"
#include "math/erfinv.h"
#include "math/hermite.h"
#include "math/math.h"

#include "debug.h"




namespace MR
{
  namespace Math
  {





    namespace
    {
      default_type F2z_upper (const default_type F, const size_t rank, const default_type dof)
      {
        assert (F >= 1.0);
        const default_type x = (dof/F) / (dof/F + default_type(rank));
        return Math::sqrt2 * Math::erfcinv (2.0 *
#ifdef MRTRIX_HAVE_EIGEN_UNSUPPORTED_SPECIAL_FUNCTIONS
              Math::betaincreg (Eigen::Array<default_type, Eigen::Dynamic, 1>::Constant (1, 0.5 * dof),
                                Eigen::Array<default_type, Eigen::Dynamic, 1>::Constant (1, 0.5 * default_type(rank)),
                                Eigen::Array<default_type, Eigen::Dynamic, 1>::Constant (1, x)) (0, 0)
#else
              Math::betaincreg (0.5 * dof, 0.5 * default_type(rank), x)
#endif
                                            );
      }

      default_type F2z_lower (const default_type oneoverF, const size_t rank, const default_type dof)
      {
        assert (oneoverF >= 1.0);
        const default_type x = (default_type(rank)/oneoverF) / (default_type(rank)/oneoverF + dof);
        return Math::sqrt2 * Math::erfinv (2.0 *
#ifdef MRTRIX_HAVE_EIGEN_UNSUPPORTED_SPECIAL_FUNCTIONS
              Math::betaincreg (Eigen::Array<default_type, Eigen::Dynamic, 1>::Constant (1, 0.5 * default_type(rank)),
                                Eigen::Array<default_type, Eigen::Dynamic, 1>::Constant (1, 0.5 * dof),
                                Eigen::Array<default_type, Eigen::Dynamic, 1>::Constant (1, x)) (0, 0)
#else
              Math::betaincreg (0.5 * default_type(rank), 0.5 * dof, x)
#endif
                                                  - 1.0);
      }
    }






    default_type t2z (const default_type stat, const default_type dof)
    {
      const default_type x = dof / (Math::pow2 (stat) + dof);
      return Math::sqrt2 * Math::erfcinv (
#ifdef MRTRIX_HAVE_EIGEN_UNSUPPORTED_SPECIAL_FUNCTIONS
            Math::betaincreg (Eigen::Array<default_type, Eigen::Dynamic, 1>::Constant (1, 0.5 * dof),
                              Eigen::Array<default_type, Eigen::Dynamic, 1>::Constant (1, 0.5),
                              Eigen::Array<default_type, Eigen::Dynamic, 1>::Constant (1, x)) (0, 0)
#else
            Math::betaincreg (0.5 * dof, 0.5, x)
#endif
                                           ) * (stat < 0.0 ? -1.0 : 1.0);
    }



    default_type F2z (const default_type F, const size_t rank, const default_type dof)
    {
      if (F >= 1.0)
        return F2z_upper (F, rank, dof);
      else
        return F2z_lower (1.0/F, rank, dof);
    }












    default_type Zstatistic::t2z (const default_type t, const size_t dof)
    {
      auto it = t2z_data.find (dof);
      if (it == t2z_data.end()) {
        std::lock_guard<std::mutex> lock (mutex);
        // Need to check again for presence of lookup table
        //   now that we have the lock
        it = t2z_data.find (dof);
        if (it == t2z_data.end())
          it = t2z_data.emplace (dof, Lookup_t2z (dof)).first;
      }
      return (it->second) (t);
    }



    default_type Zstatistic::F2z (const default_type F, const size_t rank, const size_t dof)
    {
      const auto pair = std::make_pair (rank, dof);
      auto it = F2z_data.find (pair);
      if (it == F2z_data.end()) {
        std::lock_guard<std::mutex> lock (mutex);
        // Need to check again for presence of lookup table
        //   now that we have the lock
        it = F2z_data.find (pair);
        if (it == F2z_data.end())
          it = F2z_data.emplace (pair, Lookup_F2z (rank, dof)).first;
      }
      return (it->second) (F);
    }



    default_type Zstatistic::v2z (const default_type v, const default_type dof)
    {
      return Math::t2z (v, dof);
    }



    default_type Zstatistic::G2z (const default_type G, const size_t rank, const default_type dof)
    {
      return Math::F2z (G, rank, dof);
    }












    // Function that will determine an interpolated value using
    //   the lookup table if the value lies within the pre-calculated
    //   range, or perform an explicit calculation if that is not
    //   the case
    default_type Zstatistic::LookupBase::interp (const default_type stat,
                                                 const default_type offset,
                                                 const default_type scale,
                                                 const array_type& data,
                                                 std::function<default_type(default_type)> func) const
    {
      const default_type index_float = (stat - offset) * scale;
      if (index_float >= 1.0 && index_float < data.size()-2) {
        const ssize_t index_int (ssize_t (std::floor (index_float)));
        const default_type mu (index_float - default_type (index_int));
        Math::Hermite<default_type> hermite (0.0);
        hermite.set (mu);
        return hermite.value (data[index_int-1],
                              data[index_int],
                              data[index_int+1],
                              data[index_int+2]);
      }
      return func (stat);
    }








    // Build table mapping t-statistic to z-statistic
    // Support inputs from -10.0 to +10.0 in 0.001 increments
    //   (extending range by one value at each end to enable hermite interpolation):
    // - Input of -10.001 maps to index 0
    // - Input of -10.000 maps to index 1
    // - Input of 0.000 maps to index 10001
    // - Input of +10.000 maps to index 20001
    // - Input of +10.001 maps to index 20002
    constexpr default_type t2z_max = 10.0;
    constexpr default_type t2z_step = 0.001;
    // Do not permit this division to be a non-integer value!
    constexpr ssize_t t2z_halfdomain = ssize_t(t2z_max / t2z_step);

    Zstatistic::Lookup_t2z::Lookup_t2z (const size_t dof) :
        dof (dof),
        offset (-t2z_max - t2z_step),
        scale (1.0/t2z_step)
    {
      auto t2x = [&] (const default_type t) { return dof / (Math::pow2 (t) + dof); };
      // Extra point at either end for interpolation, plus the shared point at t=0
#ifdef NDEBUG
      array_type x (3 + 2*t2z_halfdomain);
#else
      array_type x (array_type::Constant (3 + 2*t2z_halfdomain, NaN));
#endif
      x[0] = t2x (-t2z_max - t2z_step);
      for (ssize_t i = -t2z_halfdomain; i <= t2z_halfdomain; ++i) {
        const default_type t = t2z_step * i;
        x[1+i+t2z_halfdomain] = t2x (t);
      }
      x[x.size()-1] = t2x (t2z_max + t2z_step);
      assert (x.allFinite());
      // Bypasses p:
      //   essentially calculates 2q = 2(1-p) using the regularised incomplete beta function,
      //   then converts straight to Z-score using the inverse complementary error function
#ifdef MRTRIX_HAVE_EIGEN_UNSUPPORTED_SPECIAL_FUNCTIONS
      data = Math::sqrt2 * (Math::betaincreg (array_type::Constant (x.size(), 0.5 * dof),
                                              array_type::Constant (x.size(), 0.5),
                                              x.array()
                                             ).unaryExpr (&Math::erfcinv));
#else
      data.resize (x.size());
      for (ssize_t i = 0; i != x.size(); ++i)
        data[i] = Math::sqrt2 * Math::erfcinv (Math::betaincreg (0.5 * dof, 0.5, x[i]));
#endif
      // Need to negate Z-scores for which t-statistic is negative
      data.topRows(1+t2z_halfdomain) *= -1.0;
    }


    default_type Zstatistic::Lookup_t2z::operator() (const default_type t) const
    {
      auto func = [&] (const default_type in) { return Math::t2z (in, dof); };
      return interp (t, offset, scale, data, func);
    }










    // Build table mapping F-statistic to z-statistic
    // - For statistics > 1.0, use a linear lookup table
    //   - Value 1.0 maps to index 0
    //   - Value of 100.0 maps to index 9900
    // - For statistics < 1.0, take reciprocal of the value, and then
    //     refer that to a second lookup table
    //   (reciprocal should be computationally fastest of inverse transformation relationships)
    //   - Value of 1.0 maps to index 0
    //   - Value of 100.0 (input value 0.01) maps to index 9900
    // Extend ranges by 1 at either end to enable Hermite interpolation:
    // - Upper:
    //   - Value 0.99 maps to index 0
    //   - Value 1.00 maps to index 1
    //   - Value 100.00 maps to index 9901
    //   - Value 100.01 maps to index 9902
    // - Lower:
    //   - Value (1.00/0.99) maps to index 0
    //   - Value 1.00 maps to index 1
    //   - Value 0.01 maps to index 9901
    //   - Value (1.0/100.01) maps to index 9902
    constexpr default_type F2z_max = 100.0;
    constexpr default_type F2z_step = 0.01;
    // Do not permit this division to be a non-integer value!
    constexpr ssize_t F2z_halfdomain = size_t((F2z_max - 1.0) / F2z_step);

    Zstatistic::Lookup_F2z::Lookup_F2z (const size_t rank, const size_t dof) :
        rank (rank),
        dof (dof),
        offset_upper (1.0 - F2z_step),
        scale_upper (1.0 / F2z_step),
        offset_lower (1.0 - F2z_step),
        scale_lower (1.0 / F2z_step)
    {
      // F-distribution constants: d_one = rank; d_two = dof;
      // HOWEVER: p_{F(v,w)} (x) = 1.0 - p_{F(w,v)} (1.0/x)
      // We want to compute the latter in order to dodge numerical precision issues
      const array_type d_one (array_type::Constant (3 + F2z_halfdomain, rank));
      const array_type d_two (array_type::Constant (d_one.size(), dof));
#ifdef NDEBUG
      array_type F (d_one.size());
      array_type oneoverF (d_one.size());
#else
      array_type F (array_type::Constant (d_one.size(), NaN));
      array_type oneoverF (array_type::Constant (d_one.size(), NaN));
#endif
      F[0] = 1.0 - F2z_step;
      oneoverF[0] = 1.0 / (1.0 - F2z_step);
      for (ssize_t i = 0; i <= F2z_halfdomain; ++i) {
        F[i+1] = 1.0 + (default_type(i) * F2z_step);
        oneoverF[i+1] = 1.0 / F[i+1];
      }
      F[F.size()-1] = F2z_max + F2z_step;
      oneoverF[oneoverF.size()-1] = 1.0 / F[F.size()-1];
      assert (F.allFinite());
      assert (oneoverF.allFinite());
      const array_type x_upper ((d_two*oneoverF) / (d_two*oneoverF + d_one));
      // Bypasses p:
      //   calculates q = 1-p using the regularised incomplete beta function,
      //   then converts straight to Z-score using the inverse complementary error function
#ifdef MRTRIX_HAVE_EIGEN_UNSUPPORTED_SPECIAL_FUNCTIONS
      data_upper = Math::sqrt2 * (2.0 * Math::betaincreg (0.5 * d_two,
                                                          0.5 * d_one,
                                                          x_upper)
                                 ).unaryExpr (&Math::erfcinv);
#else
      data_upper.resize (x_upper.size());
      for (ssize_t i = 0; i != x_upper.size(); ++i)
        data_upper[i] = Math::sqrt2 * Math::erfcinv (2.0 * Math::betaincreg (0.5 * dof, 0.5 * rank, x_upper[i]));
#endif

      // Construct lookup table for F <= 1.0
      // This should involve avoiding the symmetry relationship gymnastics used in the first case
      // - Betaincreg (0.5*d1, 0.5*dof, x) should give p
      const array_type x_lower ((d_one*oneoverF) / (d_one*oneoverF + d_two));
#ifdef MRTRIX_HAVE_EIGEN_UNSUPPORTED_SPECIAL_FUNCTIONS
      data_lower = Math::sqrt2 * (2.0 * Math::betaincreg (0.5 * d_one,
                                                          0.5 * d_two,
                                                          x_lower) - 1.0
                                 ).unaryExpr (&Math::erfinv);
#else
      data_lower.resize (x_lower.size());
      for (ssize_t i = 0; i != x_lower.size(); ++i)
        data_lower[i] = Math::sqrt2 * Math::erfinv (2.0 * Math::betaincreg (0.5 * rank, 0.5 * dof, x_lower[i]) - 1.0);
#endif
    }



    default_type Zstatistic::Lookup_F2z::operator() (const default_type F) const
    {
      auto func_upper = [&] (const default_type in) { return F2z_upper (in, rank, default_type(dof)); };
      auto func_lower = [&] (const default_type in) { return F2z_lower (in, rank, default_type(dof)); };
      if (F >= 1.0)
        return interp (F, offset_upper, scale_upper, data_upper, func_upper);
      else
        return interp (1.0/F, offset_lower, scale_lower, data_lower, func_lower);
    }










  }
}

