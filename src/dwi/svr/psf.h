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

    template <unsigned int p>
    constexpr float bspline (const float t, const int i = 0) {
      return (t + (p+1-i)/2.0f)/p * bspline<p-1>(t, i-1) +
             ((p+1+i)/2.0f - t)/p * bspline<p-1>(t, i+1);
    }

    template <>
    constexpr float bspline<0> (const float t, const int i) {
      return ((2*t >= i-1) && (2*t < i+1)) ? 1.0f : 0.0f;
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
    class SSP
    {  MEMALIGN(SSP);
    public:

        SSP (T t = 1) : _t(t) {  }
     
        inline T operator() (const T z) const
        {
            return gaussian(z, _t/fwhm);
        }

    private:
        const T _t;
        const T fwhm = 2 * std::sqrt(2 * M_LN2);
        
        inline T gaussian (T x, T sig = 1) const
        {
            return std::exp( -x*x / (2*sig*sig) ) / std::sqrt(2*M_PI*sig*sig);
        }

    };

  }
}


#endif


