/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 * 
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * For more details, see www.mrtrix.org
 * 
 */

#include "dwi/tractography/resampling/upsampler.h"

#include "math/hermite.h"


namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace Resampling {




        bool Upsampler::operator() (std::vector<Eigen::Vector3f>& in) const
        {
          if (!interp_prepare (in))
            return false;
          std::vector<Eigen::Vector3f> out;
          for (size_t i = 3; i < in.size(); ++i) {
            out.push_back (in[i-2]);
            increment (in[i]);
            temp = M * data;
            for (ssize_t row = 0; row != temp.rows(); ++row)
              out.push_back (Eigen::Vector3f (temp.row (row)));
          }
          out.push_back (in[in.size() - 2]);
          out.swap (in);
          return true;
        }



        void Upsampler::set_ratio (const size_t upsample_ratio)
        {
          if (upsample_ratio > 1) {
            const size_t dim = upsample_ratio - 1;
            Math::Hermite<float> interp (hermite_tension);
            M.resize (dim, 4);
            for (size_t i = 0; i != dim; ++i) {
              interp.set ((i+1.0) / float(upsample_ratio));
              for (size_t j = 0; j != 4; ++j)
                M(i,j) = interp.coef(j);
            }
            temp.resize (dim, 3);
          } else {
            M.resize(0,0);
            temp.resize(0,0);
          }
        }



        bool Upsampler::interp_prepare (std::vector<Eigen::Vector3f>& in) const
        {
          if (!M.rows() || in.size() < 2)
            return false;
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
          return true;
        }



        void Upsampler::increment (const Eigen::Vector3f& a) const
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


