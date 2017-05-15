/* Copyright (c) 2008-2017 the MRtrix3 contributors.
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


#ifndef __math_gaussian_h__
#define __math_gaussian_h__

#include "math/math.h"
#include "math/vector.h"

namespace MR {
  namespace Math {
    namespace Gaussian {

      template <typename T> inline T lnP (const T measured, const T actual, const T one_over_noise_squared) 
      {
        return (0.5 * (one_over_noise_squared * pow2 (actual - measured) - log (one_over_noise_squared)));
      }




      template <typename T> inline T lnP (const T measured, const T actual, const T one_over_noise_squared, T& dP_dactual, T& dP_dN) 
      {
        assert (one_over_noise_squared > 0.0);
        dP_dN = pow2 (actual - measured);
        dP_dactual = one_over_noise_squared * dP_dN;
        dP_dN = 0.5 * (dP_dN - 1.0/one_over_noise_squared);
        return (0.5 * (dP_dactual - log(one_over_noise_squared)));
      }





      template <typename T> inline T lnP (const int N, const T* measured, const T* actual, const T one_over_noise_squared) 
      {
        assert (one_over_noise_squared > 0.0);

        T lnP = 0.0;
        for (int i = 0; i < N; i++) 
          lnP += pow2 (actual[i] - measured[i]);
        lnP *= one_over_noise_squared;
        lnP -= N * log (one_over_noise_squared);
        return (0.5*lnP); 
      }



      template <typename T> inline T lnP (const int N, const T* measured, const T* actual, const T one_over_noise_squared, T* dP_dactual, T& dP_dN) 
      {
        assert (one_over_noise_squared > 0.0);

        T lnP = 0.0;
        dP_dN = 0.0;
        for (int i = 0; i < N; i++) {
          T diff = actual[i] - measured[i];
          dP_dactual[i] = one_over_noise_squared * diff;
          lnP += pow2 (diff);
        }
        dP_dN = 0.5 * (lnP - T(N)/one_over_noise_squared);
        lnP *= one_over_noise_squared;
        lnP -= N * log (one_over_noise_squared);

        return (0.5*lnP); 
      }



      template <typename T> inline T lnP (const Vector<T>& measured, const Vector<T>& actual, const T one_over_noise_squared) 
      {
        assert (one_over_noise_squared > 0.0);
        assert (measured.size() == actual.size());

        T lnP = 0.0;
        for (size_t i = 0; i < measured.size(); i++) 
          lnP += pow2 (actual[i] - measured[i]);
        lnP *= one_over_noise_squared;
        lnP -= measured.size() * log (one_over_noise_squared);
        return (0.5*lnP); 
      }




      template <typename T> inline T lnP (const Vector<T>& measured, const Vector<T>& actual, const T one_over_noise_squared, Vector<T>& dP_dactual, T& dP_dN) 
      {
        assert (one_over_noise_squared > 0.0);
        assert (measured.size() == actual.size());
        assert (measured.size() == dP_dactual.size());

        T lnP = 0.0;
        dP_dN = 0.0;
        for (size_t i = 0; i < measured.size(); i++) {
          T diff = actual[i] - measured[i];
          dP_dactual[i] = one_over_noise_squared * diff;
          lnP += pow2 (diff);
        }
        dP_dN = 0.5 * (lnP - T(measured.size())/one_over_noise_squared);
        lnP *= one_over_noise_squared;
        lnP -= measured.size() * log (one_over_noise_squared);

        return (0.5*lnP); 
      }

    }
  }
}

#endif
