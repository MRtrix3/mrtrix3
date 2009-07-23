/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 27/06/08.

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

#ifndef __math_fft_h__
#define __math_fft_h__

#include <vector>
#include <gsl/gsl_fft_complex.h>
#include "math/complex.h"

namespace MR {
  namespace Math {

    class FFT {
      protected:
        gsl_fft_complex_wavetable*  wavetable;
        gsl_fft_complex_workspace*  workspace;

        uint length;

      public:
        FFT () { wavetable = NULL; workspace = NULL; length = 0; }
        ~FFT () { 
          if (wavetable) gsl_fft_complex_wavetable_free (wavetable); 
          if (workspace) gsl_fft_complex_workspace_free (workspace);
        }

        void fft (std::vector<cdouble>& array, bool inverse = false) {
          if (length != array.size()) {
            if (wavetable) { gsl_fft_complex_wavetable_free (wavetable); wavetable = NULL; }
            if (workspace) { gsl_fft_complex_workspace_free (workspace); workspace = NULL; }

            length = array.size();

            if (length) {
              wavetable = gsl_fft_complex_wavetable_alloc (length);
              workspace = gsl_fft_complex_workspace_alloc (length);
            }
            else return;
          }

          if ( inverse ? 
              gsl_fft_complex_forward (&array.front().real(), 1, array.size(), wavetable, workspace) :
              gsl_fft_complex_inverse (&array.front().real(), 1, array.size(), wavetable, workspace)
             ) throw Exception ("error computing FFT");
        }
    };

  }
}



#endif



