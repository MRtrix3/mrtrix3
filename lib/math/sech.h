/*******************************************************************************
    Copyright (C) 2014 Brain Research Institute, Melbourne, Australia
    
    Permission is hereby granted under the Patent Licence Agreement between
    the BRI and Siemens AG from July 3rd, 2012, to Siemens AG obtaining a
    copy of this software and associated documentation files (the
    "Software"), to deal in the Software without restriction, including
    without limitation the rights to possess, use, develop, manufacture,
    import, offer for sale, market, sell, lease or otherwise distribute
    Products, and to permit persons to whom the Software is furnished to do
    so, subject to the following conditions:
    
    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.
    
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
    OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
    CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
    TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
    SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*******************************************************************************/


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
