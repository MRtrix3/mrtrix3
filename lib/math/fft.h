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



