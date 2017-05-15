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


#ifndef __math_one_over_cosh_h__
#define __math_one_over_cosh_h__

#include "math/math.h"
#include "math/vector.h"

namespace MR {
  namespace Math {
    namespace Sech {

      template <typename T> inline T lnP (const T measured, const T actual, const T one_over_noise_squared) 
      {
        T n = sqrt (one_over_noise_squared);
        T e = n * (actual - measured);
        if (e < -20.0) e = -e;
        else if (e <= 20.0) { e = exp(e); e = log (e + 1.0/e); }
        return (e - 0.5*log (one_over_noise_squared));
      }




      template <typename T> inline T lnP (const T measured, const T actual, const T one_over_noise_squared, T& dP_dactual, T& dP_dN) 
      {
        assert (one_over_noise_squared > 0.0);
        T n = sqrt(one_over_noise_squared);
        T e = n * (actual - measured);
        T lnP;
        if (e < -20.0) { lnP = -e; e = -1.0; }
        else if (e <= 20.0) { e = exp (e); lnP = e + 1.0/e; e = (e - 1.0/e) / lnP; lnP = log (lnP); }
        else { lnP = e; e = 1.0; }

        dP_dactual = n * e;
        dP_dN = 0.5 * ((actual - measured) * e / n - 1.0/one_over_noise_squared);
        return (lnP - 0.5*log (one_over_noise_squared));
      }





      template <typename T> inline T lnP (const int N, const T* measured, const T* actual, const T one_over_noise_squared) 
      {
        assert (one_over_noise_squared > 0.0);

        T n = sqrt (one_over_noise_squared);
        T lnP = 0.0;
        for (int i = 0; i < N; i++) {
          T e = n * (actual[i] - measured[i]);
          if (e < -20.0) e = -e;
          else if (e <= 20.0) { e = exp (e); e = log (e + 1.0/e); }
          lnP += e;
        }
        return (lnP - 0.5*N*log (one_over_noise_squared)); 
      }




      template <typename T> inline T lnP (const Math::Vector<T>& measured, const Math::Vector<T>& actual, const T one_over_noise_squared) 
      {
        assert (one_over_noise_squared > 0.0);
        assert (measured.size() == actual.size());

        T n = sqrt (one_over_noise_squared);
        T lnP = 0.0;
        for (size_t i = 0; i < actual.size(); i++) {
          T e = n * (actual[i] - measured[i]);
          if (e < -20.0) e = -e;
          else if (e <= 20.0) { e = exp (e); e = log (e + 1.0/e); }
          lnP += e;
        }
        return (lnP - 0.5*actual.size()*log (one_over_noise_squared)); 
      }



      template <typename T> inline T lnP (const int N, const T* measured, const T* actual, const T one_over_noise_squared, T* dP_dactual, T& dP_dN) 
      {
        assert (one_over_noise_squared > 0.0);

        T n = sqrt (one_over_noise_squared);
        T lnP = 0.0;
        dP_dN = 0.0;
        for (int i = 0; i < N; i++) {
          T e = n * (actual[i] - measured[i]);
          T t;
          if (e < -20.0) { t = -e; e = -1.0; }
          else if (e <= 20.0) { e = exp (e); t = e + 1.0/e; e = (e - 1.0/e) / t; t = log (t); }
          else { t = e; e = 1.0; }
          lnP += t;
          dP_dactual[i] = n * e;
          dP_dN += (actual[i] - measured[i]) * e;
        }
        dP_dN = 0.5 * (dP_dN / n - N / one_over_noise_squared);
        return (lnP - 0.5*N*log (one_over_noise_squared)); 
      }





      template <typename T> inline T lnP (const Math::Vector<T>& measured, const Math::Vector<T>& actual, const T one_over_noise_squared, Math::Vector<T>& dP_dactual, T& dP_dN) 
      {
        assert (one_over_noise_squared > 0.0);
        assert (measured.size() == actual.size());

        T n = sqrt (one_over_noise_squared);
        T lnP = 0.0;
        dP_dN = 0.0;
        for (size_t i = 0; i < actual.size(); i++) {
          T e = n * (actual[i] - measured[i]);
          T t;
          if (e < -20.0) { t = -e; e = -1.0; }
          else if (e <= 20.0) { e = exp (e); t = e + 1.0/e; e = (e - 1.0/e) / t; t = log (t); }
          else { t = e; e = 1.0; }
          lnP += t;
          dP_dactual[i] = n * e;
          dP_dN += (actual[i] - measured[i]) * e;
        }
        dP_dN = 0.5 * (dP_dN / n - actual.size() / one_over_noise_squared);
        return (lnP - 0.5*actual.size()*log (one_over_noise_squared)); 
      }


    }
  }
}

#endif
