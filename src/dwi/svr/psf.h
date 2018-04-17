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
    namespace SVR
    {

    /**
     *  1-D Slice Sensitivity Profile.
     */
    template <typename T = float, int n = 2>
    struct SSP
    {
    public:

        SSP (const T fwhm = 1)
        {
            if (fwhm <= std::numeric_limits<T>::epsilon()) {
                for (int z = -n; z <= n; z++)
                    values[n+z] = (z==0) ? 1 : 0;
            } else {
                T tau = scale/fwhm;
                tau *= -tau/2;
                for (int z = -n; z <= n; z++) {
                    values[n+z] = gaussian(T(z), tau);
                }
                normalise_values();
            }
        }

        template<typename VectorType>
        SSP (const VectorType& vec)
        {
            assert (vec.size() == values.size());
            for (size_t i = 0; i < values.size(); i++)
                values[i] = vec[i];
            normalise_values();
        }
     
        inline T operator() (const int z) const
        {
            return values[n+z];
        }

        constexpr int size () const
        {
            return n;
        }


    private:
        std::array<T,2*n+1> values;
        static constexpr T scale = 2.35482;     // 2.sqrt(2.ln(2));
                
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
}


#endif


