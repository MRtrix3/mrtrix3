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

#include "math/hermite.h"


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
        // Abandoned curvature-based extrapolation - badly posed when step size is not guaranteed to be consistent,
        //   and probably makes little difference anyways
        const size_t s = in.size();
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






      bool Resampler::operator() (std::vector<Eigen::Vector3f>& tck) const
      {
        Math::Hermite<float> interp (0.1);
        std::vector<Eigen::Vector3f> output;
        // Extensions required to enable Hermite interpolation in last streamline segment at either end
        const size_t s = tck.size();
        tck.insert    (tck.begin(), tck[ 0 ] + (tck[1] - tck[0]));
        tck.push_back (             tck[ s ] + (tck[s] - tck[s-1]));
        const ssize_t midpoint = tck.size()/2;
        output.push_back (tck[midpoint]);
        // Generate from the midpoint to the start, reverse, then generate from midpoint to the end
        for (ssize_t step = -1; step <= 1; step += 2) {

          ssize_t index = midpoint;
          float mu_lower = 0.0f;

          // Loop to generate points
          do {

            // If we don't have to step along the input track, can keep the mu from the previous
            //   interpolation point as the lower bound
            while (index > 1 && index < ssize_t(tck.size()-2) && (output.back() - tck[index+step]).norm() < step_size) {
              index += step;
              mu_lower = 0.0f;
            }
            // Always preserve the termination points, regardless of resampling
            if (index == 1) {
              output.push_back (tck[1]);
              std::reverse (output.begin(), output.end());
            } else if (index == ssize_t(tck.size()-2)) {
              output.push_back (tck[s]);
            } else {

              // Perform binary search
              Eigen::Vector3f p_lower = tck[index], p, p_upper = tck[index+step];
              float mu_upper = 1.0f;
              float mu = 0.5 * (mu_lower + mu_upper);
              do {
                mu = 0.5 * (mu_lower + mu_upper);
                interp.set (mu);
                p = interp.value (tck[index-step], tck[index], tck[index+step], tck[index+2*step]);
                if ((p - output.back()).norm() < step_size) {
                  mu_lower = mu;
                  p_lower = p;
                } else {
                  mu_upper = mu;
                  p_upper = p;
                }
              } while ((p_upper - p_lower).norm() > 0.001 * step_size);
              output.push_back (p);

            }

            // Loop until an endpoint has been added
          } while (index > 1 && index < ssize_t(tck.size()-2));

        }

        std::swap (tck, output);
        return true;
      }






    }
  }
}


