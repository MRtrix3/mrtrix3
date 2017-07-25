/* Copyright (c) 2008-2017 the MRtrix3 contributors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
 */


#ifndef __dwi_svr_psf_h__
#define __dwi_svr_psf_h__


#include <cmath>

#include "types.h"


namespace MR
{
  namespace DWI
  {

    // B-spline (de Boor recursion formula)
    template <unsigned int p>
    constexpr float bspline (const float t) {
      return (0.5f*(p+1) + t)/p * bspline<p-1>(t+0.5f) +
             (0.5f*(p+1) - t)/p * bspline<p-1>(t-0.5f);
    }

    // Order 0 -- Nearest neighbour interpolation.
    template <>
    constexpr float bspline<0> (const float t) {
      return ((t >= -0.5f) && (t < 0.5f)) ? 1.0f : 0.0f;
    }

    // Order 1 -- Linear interpolation.
    // Template specialisation redundant but faster. 
    template <>
    constexpr float bspline<1> (const float t) {
      return (t < 0) ? ((t > -1.0f) ? 1.0f+t : 0.0f)
                     : ((t <  1.0f) ? 1.0f-t : 0.0f);
    }

    // Order 3 -- Cubic interpolation.
    // Template specialisation redundant but much faster.
    template <>
    constexpr float bspline<3> (const float t) {
      return (t < 0) ? ((t > -1.0f) ? 2.0f/3 - 0.5f*t*t*(2.0f+t) : ((t > -2.0f) ? (2.0f+t)*(2.0f+t)*(2.0f+t)/6 : 0.0f))
                     : ((t <  1.0f) ? 2.0f/3 - 0.5f*t*t*(2.0f-t) : ((t <  2.0f) ? (2.0f-t)*(2.0f-t)*(2.0f-t)/6 : 0.0f));
    }

    
    template <size_t p>
    FORCE_INLINE std::array<float,p+1> bsplineweights (const float t) {
      throw Exception("Not implemented.");
    }

    template <>
    FORCE_INLINE std::array<float,2> bsplineweights<1> (const float t) {
      assert((t >= 0.0) && (t <= 1.0));
      return {{ 1.0f-t, t }};
    }

    template <>
    FORCE_INLINE std::array<float,4> bsplineweights<3> (const float t) {
      assert((t >= 0.0) && (t <= 1.0));
      float u = 1.0f-t, t2 = t*t, u2 = u*u;
      return {{ u2*u/6, 2.0f/3-t2*(2.0f-t)/2, 2.0f/3-u2*(2.0f-u)/2, t2*t/6 }};
    }


    /**
     * 3-D Sinc Point Spread Function
     */
    template <typename T = float>
    class SincPSF
    {  MEMALIGN(SincPSF);
    public:

        SincPSF (T w, T s = 1) : _w(w), _s(s) {  }

        template <class VectorType>
        inline T operator() (const VectorType& d) const
        {
            return psf(d[0]) * psf(d[1]) * psf(d[2]);
        }

    private:
        const T _w, _s;
        const T eps  = std::numeric_limits<T>::epsilon();
        const T eps2 = std::sqrt(eps);
        const T eps4 = std::sqrt(eps2);

        inline T psf (T x) const
        {
            return sinc(x / _s) * blackman(x, _w);
        }

        inline T sinc (T x) const
        {
            T y = M_PI * x;
            if (y > eps4)
                return std::sin(y) / y;
            else if (y > eps2)
                return 1 - y*y / 6 + y*y / 120;
            else if (y > eps)
                return 1 - y*y / 6;
            else
                return 1;
        }

        inline T blackman (T x, T w) const
        {
            T y = M_PI * (x - w) / w;
            return 0.42 - 0.5 * std::cos(y) + 0.08 * std::cos(2*y);
        }

    };


    /**
     *  1-D Gaussian Slice Sensitivity Profile.
     */
    template <typename T = float, int n = 2>
    struct SSP
    {
    public:

        SSP (T fwhm = 1)
        {
            T tau = scale/fwhm;
            tau *= -tau/2;
            for (int z = -n; z <= n; z++) {
                values[n+z] = gaussian(T(z), tau);
            }
            normalise_values();
        }
     
        inline T operator() (const int z) const
        {
            return values[n+z];
        }

        inline int size () const
        {
            return n;
        }


    private:
        std::array<T,2*n+1> values;
        const T scale = 2.35482;     // 2.sqrt(2.ln(2));
                
        inline T gaussian (T x, T tau) const
        {
            return std::exp(tau * x*x);
        }

        inline void normalise_values ()
        {
            T norm = 0;
            for (int z = -n; z <= n; z++)
                norm += values[n+z];
            for (int z = -n; z <= n; z++)
                values[n+z] /= norm;
        }

    };

  }
}


#endif


