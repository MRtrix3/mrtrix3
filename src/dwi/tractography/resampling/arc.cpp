/* Copyright (c) 2008-2017 the MRtrix3 contributors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
 */


#include "dwi/tractography/resampling/arc.h"

#include "math/math.h"


namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace Resampling {



        bool Arc::operator() (const Streamline<>& in, Streamline<>& out) const
        {
          assert (in.size());
          assert (planes.size());
          out.clear();
          out.index = in.index;
          out.weight = in.weight;

          // Determine which points on the streamline correspond to the endpoints of the arc
          idx_start = idx_end = 0;
          size_t a (0), b (0);

          int prev_s = -1;
          for (size_t i = 0; i < in.size(); ++i) {
            int s = state (in[i]);
            if (i) {
              if (prev_s == -1 && s == 0) a = i-1;
              if (prev_s == 0 && s == -1) a = i;
              if (prev_s == 1 && s == 2)  b = i;
              if (prev_s == 2 && s == 1)  b = i-1;

              if (a && b) {
                if (b - a > idx_end - idx_start) {
                  idx_start = a;
                  idx_end = b;
                }
                a = b = 0;
              }
            }
            prev_s = s;
          }
          ++idx_end;

          if (!(idx_start && idx_end))
            return false;

          const bool reverse = idx_start > idx_end;
          size_t i = idx_start;

          for (size_t n = 0; n < nsamples; n++) {
            while (i != idx_end) {
              const value_type d = planes[n].dist (in[i]);
              if (d > 0.0) {
                const value_type f = d / (d - planes[n].dist (in[reverse ? i+1 : i-1]));
                out.push_back (f*in[i-1] + (1.0f-f)*in[i]);
                break;
              }
              reverse ? --i : ++i;
            }
          }
          return true;
        }



        void Arc::init_line()
        {
          start_dir = (end - start).normalized();
          mid_dir = end_dir = start_dir;
          for (size_t n = 0; n < nsamples; n++) {
            value_type f = value_type(n) / value_type (nsamples-1);
            planes.push_back (Plane ((1.0f-f) * start + f * end, (1.0f-f) * start_dir + f * end_dir));
          }
        }



        void Arc::init_arc (const point_type& waypoint)
        {
          mid_dir = (end - start).normalized();

          Eigen::Matrix3f M;

          M(0,0) = start[0] - mid[0];
          M(0,1) = start[1] - mid[1];
          M(0,2) = start[2] - mid[2];

          M(1,0) = end[0] - mid[0];
          M(1,1) = end[1] - mid[1];
          M(1,2) = end[2] - mid[2];

          point_type n ((start-mid).cross (end-mid));
          M(2,0) = n[0];
          M(2,1) = n[1];
          M(2,2) = n[2];

          point_type a;
          a[0] = 0.5 * (start+mid).dot(start-mid);
          a[1] = 0.5 * (end+mid).dot(end-mid);
          a[2] = start.dot(n);

          point_type c = M.fullPivLu().solve(a);

          point_type x (start-c);
          value_type R = x.norm();

          point_type y (mid-c);
          y -= y.dot(x)/(x.norm()*y.norm()) * x;
          y *= R / y.norm();

          point_type e (end-c);
          value_type ex (x.dot (e)), ey (y.dot (e));

          value_type angle = std::atan2 (ey, ex);
          if (angle < 0.0) angle += 2.0 * Math::pi;

          for (size_t n = 0; n < nsamples; n++) {
            value_type f = angle * value_type(n) / value_type (nsamples-1);
            planes.push_back (Plane (c + x*cos(f) + y*sin(f), y*cos(f) - x*sin(f)));
          }

          start_dir = y;
          end_dir = y*cos(angle) - x*sin(angle);
        }



        int Arc::state (const point_type& p) const
        {
          const bool after_start = start_dir.dot (p - start) >= 0;
          const bool after_mid = mid_dir.dot (p - mid) > 0.0;
          const bool after_end = end_dir.dot (p - end) >= 0.0;
          if (!after_start && !after_mid) return -1; // before start
          if (after_start && !after_mid) return 0; // after start
          if (after_mid && !after_end) return 1; // before end
          return 2; // after end
        }



      }
    }
  }
}


