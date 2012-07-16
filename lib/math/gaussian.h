/*
   Copyright 2008 Brain Research Institute, Melbourne, Australia

   Written by J-Donald Tournier, 11/06/09.

   This file is part of MRtrix.

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
