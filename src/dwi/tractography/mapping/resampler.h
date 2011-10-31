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


template <class Datatype, typename T>
class Resampler
{

  public:
    Resampler (const Math::Matrix<T>& interp_matrix, const size_t c) :
      M       (interp_matrix),
      columns (c),
      data    (4, c) { }

    Resampler (const Resampler<Datatype, T>& that) :
      M       (that.M),
      columns (that.columns),
      data    (4, columns) { }

    ~Resampler() { }


    size_t get_os_ratio() const { return (M.rows() + 1); }
    size_t get_columns () const { return (columns); }
    bool valid () const { return (M.is_set()); }


    void interpolate (std::vector<Datatype>& in)
    {
      std::vector<Datatype> out;
      Math::Matrix<T> temp (M.rows(), columns);
      interp_prepare (in);
      for (size_t i = 3; i < in.size(); ++i) {
        out.push_back (in[i-2]);
        increment (in[i]);
        Math::mult (temp, M, data);
        for (size_t row = 0; row != temp.rows(); ++row)
          out.push_back (Datatype (temp.row (row)));
      }
      out.push_back (in[in.size() - 2]);
      out.swap (in);
    }



  private:
    const Math::Matrix<T>& M;
    const size_t columns;
    Math::Matrix<T> data;


    void interp_prepare (std::vector<Datatype>& in)
    {
      const size_t s = in.size();
      if (s > 2) {
        in.insert    (in.begin(), in[ 0 ] + (float(2.0) * (in[ 0 ] - in[ 1 ])) - (in[ 1 ] - in[ 2 ]));
        in.push_back (            in[ s ] + (float(2.0) * (in[ s ] - in[s-1])) - (in[s-1] - in[s-2]));
      } else {
        in.push_back (            in[1] + (in[1] - in[0]));
        in.insert    (in.begin(), in[0] + (in[0] - in[1]));
      }
      for (size_t i = 0; i != columns; ++i) {
        data(0,i) = 0.0;
        data(1,i) = (in[0])[i];
        data(2,i) = (in[1])[i];
        data(3,i) = (in[2])[i];
      }
    }

    void increment (const Datatype& a)
    {
      for (size_t i = 0; i != columns; ++i) {
        data(0,i) = data(1,i);
        data(1,i) = data(2,i);
        data(2,i) = data(3,i);
        data(3,i) = a[i];
      }
    }


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



