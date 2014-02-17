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
