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

#include "dwi/tractography/resample.h"


namespace MR {
  namespace DWI {
    namespace Tractography {






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
          // Hermite tension = 0.1;
          // cubic interpolation (tension = 0.0) looks 'bulgy' between control points
          Math::Hermite<float> interp (0.1);
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
        const size_t s = in.size();
        // Abandoned curvature-based extrapolation - badly posed when step size is not guaranteed to be consistent,
        //   and probably makes little difference anyways
        in.insert    (in.begin(), in[ 0 ] + (in[1] - in[0]));
        in.push_back (            in[ s ] + (in[s] - in[s-1]));
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



      bool Downsampler::operator() (Tracking::GeneratedTrack& tck) const
      {
        if (ratio <= 1 || tck.empty())
          return false;
        size_t index_old = ratio;
        if (tck.get_seed_index()) {
          index_old = (((tck.get_seed_index() - 1) % ratio) + 1);
          tck.set_seed_index (1 + ((tck.get_seed_index() - index_old) / ratio));
        }
        size_t index_new = 1;
        while (index_old < tck.size() - 1) {
          tck[index_new++] = tck[index_old];
          index_old += ratio;
        }
        tck[index_new] = tck.back();
        tck.resize (index_new + 1);
        return true;
      }




      bool Downsampler::operator() (std::vector<Eigen::Vector3f>& tck) const
      {
        if (ratio <= 1 || tck.empty())
          return false;
        const size_t midpoint = tck.size()/2;
        size_t index_old = (((midpoint - 1) % ratio) + 1);
        size_t index_new = 1;
        while (index_old < tck.size() - 1) {
          tck[index_new++] = tck[index_old];
          index_old += ratio;
        }
        tck[index_new] = tck.back();
        tck.resize (index_new + 1);
        return true;
      }






    }
  }
}


