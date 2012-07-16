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
