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

#include "progressbar.h"
#include "math/fft.h"
#include "image/fft.h"
#include "image/voxel.h"

namespace MR {
  namespace Image {


    namespace {
      inline bool next (Voxel& pos, const int limits[MRTRIX_MAX_NDIMS])
      {
        size_t axis = 0;
        do {
          pos[axis]++;
          if (pos[axis] < limits[axis]) return (true);
          pos[axis] = 0;
          axis++;
        } while (axis < pos.ndim());
        return (false);
      }
    }




    void FFT::fft (Voxel& dest, Voxel& source, size_t axis, bool inverse, bool shift)
    {
      int shift_dist = (source.dim(axis)+1)/2;
      int shift_up   = source.dim(axis)/2;

      std::vector<cdouble> array (source.dim (axis));

      size_t count = 1;
      int limits[MRTRIX_MAX_NDIMS];
      for (size_t n = 0; n < source.ndim(); n++) {
        if (n == axis) limits[n] = 1;
        else { limits[n] = source.dim(n); count *= limits[n]; }
      }

      ProgressBar::init (count, std::string ("performing ") + ( shift ? "shifted " : "" ) +  ( inverse ? "inverse " : "" ) 
          + "FFT along axis " + str (axis) +"...");

      do {
        for (int n = 0; n < source.dim(axis); n++) {
          if (shift && inverse) source[axis] = ( n >= shift_dist ? n - shift_dist : n + shift_up );
          else source[axis] = n;
          array[n].real() = source.real();
          array[n].imag() = source.imag();
        }

        ft.fft (array, inverse);

        for (int n = 0; n < source.dim(axis); n++) {
          if (shift && !inverse) dest[axis] = ( n >= shift_dist ? n - shift_dist : n + shift_up );
          else dest[axis] = n;
          if (dest.is_complex()) {
            dest.real() = array[n].real();
            dest.imag() = array[n].imag();
          }
          else dest.value() = abs(array[n]);
        }

        ProgressBar::inc();  
      } while (next (source, limits));

      ProgressBar::done();
    }

  }
}


