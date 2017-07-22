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

#define FP_FAST_FMAF


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
    template <typename T = float>
    struct SSP
    {  NOMEMALIGN;
    public:

        SSP (T t = 1)
         : _t(t),
           tau (-fwhm*fwhm/(2*_t*_t)),
           norm (fwhm / (_t * std::sqrt(2*M_PI)))
        {  }
     
        inline T operator() (const T z) const
        {
            return gaussian(z);
        }

    private:
        const T fwhm = 2 * std::sqrt(2 * M_LN2);
        const T _t;
        const T tau;
        const T norm;
        
        inline T gaussian (T x) const
        {
            return norm * std::exp(tau * x*x);
        }

    };

  }
}


#endif


