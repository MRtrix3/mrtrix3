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


    17-12-2008 J-Donald Tournier <d.tournier@brain.org.au>
    * minor changes to tidy up the code

*/

#ifndef __dwi_tensor_h__
#define __dwi_tensor_h__

#include "math/matrix.h"
#include "math/vector.h"

namespace MR {
  namespace DWI {

    inline float tensor2ADC (float *t) { return ((t[0]+t[1]+t[2])/3.0); }


    inline float tensor2FA (float *t)
    {
      float trace = tensor2ADC (t);
      float a[] = { t[0]-trace, t[1]-trace, t[2]-trace };
      trace = t[0]*t[0] + t[1]*t[1] + t[2]*t[2] + 2.0*( t[3]*t[3] + t[4]*t[4] + t[5]*t[5] );
      return (trace ? sqrt((3/2)*(a[0]*a[0]+a[1]*a[1]+a[2]*a[2] + 2.0*(t[3]*t[3]+t[4]*t[4]+t[5]*t[5])) / trace) : 0.0);
    }


    inline float tensor2RA (float *t)
    {
      float trace = tensor2ADC (t);
      float a[] = { t[0]-trace, t[1]-trace, t[2]-trace };
      return (trace ? sqrt((a[0]*a[0]+a[1]*a[1]+a[2]*a[2]+ 2.0*(t[3]*t[3]+t[4]*t[4]+t[5]*t[5]))/3.0) / trace : 0.0);
    }

  }
}

#endif
