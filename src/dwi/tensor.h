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


#ifndef __dwi_tensor_h__
#define __dwi_tensor_h__

#include "math/matrix.h"
#include "math/vector.h"

namespace MR
{
  namespace DWI
  {

    template <typename T> inline Math::Matrix<T> grad2bmatrix (Math::Matrix<T> &bmat, const Math::Matrix<T> &grad)
    {
      bmat.allocate (grad.rows(),7);
      for (size_t i = 0; i < grad.rows(); ++i) {
        bmat (i,0) = grad (i,3) * grad (i,0) *grad (i,0);
        bmat (i,1) = grad (i,3) * grad (i,1) *grad (i,1);
        bmat (i,2) = grad (i,3) * grad (i,2) *grad (i,2);
        bmat (i,3) = grad (i,3) * 2*grad (i,0) *grad (i,1);
        bmat (i,4) = grad (i,3) * 2*grad (i,0) *grad (i,2);
        bmat (i,5) = grad (i,3) * 2*grad (i,1) *grad (i,2);
        bmat (i,6) = -1.0;
      }
      return bmat;
    }




    template <typename T> inline void dwi2tensor (const Math::Matrix<T>& binv, T* d)
    {
      VLA (logs, T, binv.columns());
      for (size_t i = 0; i < binv.columns(); ++i)
        logs[i] = d[i] > T (0.0) ? -Math::log (d[i]) : T (0.0);
      Math::Vector<T> logS (logs, binv.columns());
      Math::Vector<T> DT (d, 7);
      Math::mult (DT, binv, logS);
    }


    template <typename T> inline T tensor2ADC (T* t)
    {
      return (t[0]+t[1]+t[2]) /T (3.0);
    }


    template <typename T> inline T tensor2FA (T* t)
    {
      T trace = tensor2ADC (t);
      T a[] = { t[0]-trace, t[1]-trace, t[2]-trace };
      trace = t[0]*t[0] + t[1]*t[1] + t[2]*t[2] + T (2.0) * (t[3]*t[3] + t[4]*t[4] + t[5]*t[5]);
      return trace ?
             Math::sqrt (T (1.5) * (a[0]*a[0]+a[1]*a[1]+a[2]*a[2] + T (2.0) * (t[3]*t[3]+t[4]*t[4]+t[5]*t[5])) / trace) :
             T (0.0);
    }


    template <typename T> inline T tensor2RA (T* t)
    {
      T trace = tensor2ADC (t);
      T a[] = { t[0]-trace, t[1]-trace, t[2]-trace };
      return trace ?
             sqrt ( (a[0]*a[0]+a[1]*a[1]+a[2]*a[2]+ T (2.0) * (t[3]*t[3]+t[4]*t[4]+t[5]*t[5])) /T (3.0)) / trace :
             T (0.0);
    }

  }
}

#endif
