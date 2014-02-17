#ifndef __dwi_tractography_algorithms_iFOD_calibrator_h__
#define __dwi_tractography_algorithms_iFOD_calibrator_h__

#include "point.h"
#include "math/SH.h"
#include "dwi/tractography/tracking/shared.h"
#include "dwi/tractography/tracking/types.h"

#define SQRT_3_OVER_2 0.866025403784439
#define NUM_CALIBRATE 1000

namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace Algorithms {

      using namespace MR::DWI::Tractography::Tracking;

      template <typename value_type>
        std::vector<Point<value_type> > direction_grid (value_type max_angle, value_type spacing)
        {
          using namespace Math;
          const value_type maxR = pow2 (max_angle / spacing);
          std::vector<Point<value_type> > list;
          ssize_t extent = ceil (max_angle / spacing);

          for (ssize_t i = -extent; i <= extent; ++i) {
            for (ssize_t j = -extent; j <= extent; ++j) {
              value_type x = i + 0.5*j, y = SQRT_3_OVER_2*j;
              value_type n = pow2(x) + pow2(y);
              if (n > maxR) 
                continue;
              n = spacing*Math::sqrt (n);
              value_type z = Math::cos (n);
              if (n) n = spacing * Math::sin (n) / n;
              list.push_back (Point<value_type> (n*x, n*y, z));
            }
          }

          return (list);
        }

      namespace {
        class Pair {
          public:
            Pair (value_type elevation, value_type amplitude) : el (elevation), amp (amplitude) { }
            value_type el, amp;
        };
      }

      template <class Method> 
        void calibrate (Method& method) 
        {
          typename Method::Calibrate calibrate_func (method);
          const value_type sqrt3 = Math::sqrt (3.0);

          std::vector<Pair> amps;
          for (value_type el = 0.0; el < method.S.max_angle; el += 0.001) {
            amps.push_back (Pair (el, calibrate_func (el)));
            if (!std::isfinite (amps.back().amp) || amps.back().amp <= 0.0) break;
          }
          value_type zero = amps.back().el;

          value_type N_min = INFINITY, theta_min = NAN, ratio = NAN;
          for (size_t i = 1; i < amps.size(); ++i) {
            value_type N = Math::pow2 (method.S.max_angle);
            value_type Ns =  N * (1.0+amps[0].amp/amps[i].amp)/(2.0*Math::pow2(zero));
            std::vector<Point<value_type> > dirs = direction_grid<value_type> (method.S.max_angle + amps[i].el, sqrt3*amps[i].el);
            N = Ns+dirs.size();
            //std::cout << amps[i].el << " " << amps[i].amp << " " << Ns << " " << dirs.size() << " " << Ns+dirs.size() << "\n";
            if (N > 0.0 && N < N_min) {
              N_min = N;
              theta_min = amps[i].el;
              ratio = amps[0].amp / amps[i].amp;
            }
          }

          method.calibrate_list = direction_grid (method.S.max_angle+theta_min, sqrt3*theta_min);
          method.calibrate_ratio = ratio;

          INFO ("rejection sampling will use " + str (method.calibrate_list.size()) 
              + " directions with a ratio of " + str (method.calibrate_ratio) + " (predicted number of samples per step = " + str (N_min) + ")");
        }


      }
    }
  }
}

#endif




