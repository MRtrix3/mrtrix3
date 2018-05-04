/*
 * Copyright (c) 2008-2018 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/
 *
 * MRtrix3 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/
 */


#include "dwi/tractography/resampling/upsampler.h"

#include "math/hermite.h"


namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace Resampling {




        bool Upsampler::operator() (const Streamline<>& in, Streamline<>& out) const
        {
          if (get_ratio() == 1 || in.size() < 2) {
            out = in;
            return true;
          }
          out.clear();
          out.index = in.index;
          out.weight = in.weight;
          Streamline<> in_padded (in);
          interp_prepare (in_padded);
          for (size_t i = 3; i < in_padded.size(); ++i) {
            out.push_back (in_padded[i-2]);
            increment (in_padded[i]);
            temp = M * data;
            for (ssize_t row = 0; row != temp.rows(); ++row)
              out.push_back (Eigen::Vector3f (temp.row (row)));
          }
          out.push_back (in_padded[in_padded.size() - 2]);
          return true;
        }



        void Upsampler::set_ratio (const size_t upsample_ratio)
        {
          if (upsample_ratio > 1) {
            const size_t dim = upsample_ratio - 1;
            Math::Hermite<value_type> interp (hermite_tension);
            M.resize (dim, 4);
            for (size_t i = 0; i != dim; ++i) {
              interp.set ((i+1.0) / value_type(upsample_ratio));
              for (size_t j = 0; j != 4; ++j)
                M(i,j) = interp.coef(j);
            }
            temp.resize (dim, 3);
          } else {
            M.resize(0,0);
            temp.resize(0,0);
          }
        }



        void Upsampler::interp_prepare (Streamline<>& in) const
        {
          assert (in.size() >= 2);
          // Abandoned curvature-based extrapolation - badly posed when step size is not guaranteed to be consistent,
          //   and probably makes little difference anyways
          const size_t s = in.size();
          in.insert    (in.begin(), in[0] + (in[0] - in[ 1 ]));
          in.push_back (            in[s] + (in[s] - in[s-1]));
          for (size_t i = 0; i != 3; ++i) {
            data(0,i) = 0.0;
            data(1,i) = (in[0])[i];
            data(2,i) = (in[1])[i];
            data(3,i) = (in[2])[i];
          }
        }



        void Upsampler::increment (const point_type& a) const
        {
          for (size_t i = 0; i != 3; ++i) {
            data(0,i) = data(1,i);
            data(1,i) = data(2,i);
            data(2,i) = data(3,i);
            data(3,i) = a[i];
          }
        }




      }
    }
  }
}


