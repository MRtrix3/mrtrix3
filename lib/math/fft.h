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


#ifndef __math_fft_h__
#define __math_fft_h__

#include <vector>
#include <gsl/gsl_fft_complex.h>
#include "math/complex.h"

namespace MR
{
  namespace Math
  {

    class FFT
    {
      public:
        FFT () : wavetable (NULL), workspace (NULL), length (0) { }
        ~FFT () {
          if (wavetable) gsl_fft_complex_wavetable_free (wavetable);
          if (workspace) gsl_fft_complex_workspace_free (workspace);
        }

        void operator() (std::vector<cdouble>& array, bool inverse = false) {
          if (length != array.size()) {
            if (wavetable) {
              gsl_fft_complex_wavetable_free (wavetable);
              wavetable = NULL;
            }
            if (workspace) {
              gsl_fft_complex_workspace_free (workspace);
              workspace = NULL;
            }

            length = array.size();

            if (length) {
              wavetable = gsl_fft_complex_wavetable_alloc (length);
              workspace = gsl_fft_complex_workspace_alloc (length);
            }
            else return;
          }

          if (inverse ?
              gsl_fft_complex_inverse (&array.front().real(), 1, array.size(), wavetable, workspace) :
              gsl_fft_complex_forward (&array.front().real(), 1, array.size(), wavetable, workspace)
             ) throw Exception ("error computing FFT");
        }

      protected:
        gsl_fft_complex_wavetable*  wavetable;
        gsl_fft_complex_workspace*  workspace;
        size_t length;
    };

  }
}



#endif



