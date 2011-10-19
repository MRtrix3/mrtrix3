/*
    Copyright 2011 Brain Research Institute, Melbourne, Australia

    Written by Robert E. Smith, 2011.

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

#ifndef __dwi_tractography_mapping_resampler_h__
#define __dwi_tractography_mapping_resampler_h__


#include "math/hermite.h"
#include "math/matrix.h"


#define HERMITE_TENSION 0.1


namespace MR {
namespace DWI {
namespace Tractography {
namespace Mapping {


template <typename T>
class Resampler
{

  public:
    Resampler (const Math::Matrix<T>& interp_matrix, const size_t c) :
      M (interp_matrix),
      columns (c),
      data (4, c) { }

    ~Resampler() { }

    size_t get_columns () const { return (columns); }
    bool valid () const { return (M.is_set()); }

    void init (const T* a, const T* b, const T* c)
    {
      for (size_t i = 0; i != columns; ++i) {
        data(0,i) = 0.0;
        data(1,i) = a[i];
        data(2,i) = b[i];
        data(3,i) = c[i];
      }
    }

    void increment (const T* a)
    {
      for (size_t i = 0; i != columns; ++i) {
        data(0,i) = data(1,i);
        data(1,i) = data(2,i);
        data(2,i) = data(3,i);
        data(3,i) = a[i];
      }
    }

    MR::Math::Matrix<T>& interpolate (Math::Matrix<T>& result) const 
    {
      return (Math::mult (result, M, data));
    }

  private:
    const Math::Matrix<T>& M;
    size_t columns;
    Math::Matrix<T> data;

};



template <typename T>
Math::Matrix<T> gen_interp_matrix (const size_t os_factor)
{
  Math::Matrix<T> M;
  if (os_factor > 1) {
    const size_t dim = os_factor - 1;
    Math::Hermite<T> interp (HERMITE_TENSION);
    M.allocate (dim, 4);
    for (size_t i = 0; i != dim; ++i) {
      interp.set ((i+1.0) / float(os_factor));
      for (size_t j = 0; j != 4; ++j)
        M(i,j) = interp.coef(j);
    }
  }
  return (M);
}



}
}
}
}

#endif



