/* Copyright (c) 2017-2019 Daan Christiaens
 *
 * MRtrix and this add-on module are distributed in the hope
 * that it will be useful, but WITHOUT ANY WARRANTY; without
 * even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.
 */

#pragma once


#include <cmath>

#include "types.h"


namespace MR::DWI::SVR
{

    /**
     *  1-D Slice Sensitivity Profile.
     */
    template <typename T = float>
    struct SSP
    {
    public:

        SSP (const T fwhm = 1)
          : n (std::floor(fwhm / scale)), values (2*n+1)
        {
            for (int z = -n; z <= n; z++)
                values[n+z] = gaussian(z, fwhm / scale);
            normalise_values();
        }

        template<typename VectorType>
        SSP (const VectorType& vec)
          : n (vec.size() / 2), values (2*n+1)
        {
            for (size_t i = 0; i < values.size(); i++)
                values[i] = vec[i];
            normalise_values();
        }
     
        inline T operator() (const int z) const {
            return values[n+z];
        }

        inline int size () const {
            return n;
        }


    private:
        int n;
        std::vector<T> values;
        static constexpr T scale = 2.35482;     // 2.sqrt(2.ln(2));
                
        inline T gaussian (T x, T sigma) const {
            T y = x / sigma;
            return std::exp(-0.5 * y*y);
        }

        inline void normalise_values () {
            T norm = 0;
            for (int z = -n; z <= n; z++)
                norm += values[n+z];
            for (int z = -n; z <= n; z++)
                values[n+z] /= norm;
        }

    };

}

