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

    /**
     *  3-D Point Spread Function in (scanner) voxel coordinates.
     */
    template <typename T = float>
    class PSF
    {  MEMALIGN(PSF);
    public:

        PSF (T w, T sz = 1, T sxy = 1) : _sxy(sxy), _sz(sz), _w(w) {  }

        template <class VectorType>
        T operator() (const VectorType& d) const
        {
            return psf_xy(d[0]) * psf_xy(d[1]) * psf_z(d[2]);
        }

    private:
        const T _sxy, _sz, _w;
        static constexpr T fwhm = 2 * std::sqrt(2 * M_LN2); 

        inline T psf_xy (T x) const
        {
            return sinc(x / _sxy) * blackman(x, _w);
        }

        inline T psf_z (T z) const
        {
            return gaussian(z, _sz/fwhm) * blackman(z, _w);
        }

        inline T gaussian (T x, T sig = 1) const
        {
            return std::exp( -x*x / (2*sig*sig) ) / std::sqrt(2*M_PI*sig*sig);
        }

        inline T sinc (T x) const
        {
            constexpr T eps  = std::numeric_limits<T>::epsilon;
            constexpr T eps2 = std::sqrt(eps);
            constexpr T eps4 = std::sqrt(eps2);

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

  }
}


#endif


