/* Copyright (c) 2008-2017 the MRtrix3 contributors.
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


#include "registration/transform/convergence_check.h"
#include "debug.h"

namespace MR
{
  using namespace MR::Math;
  namespace Registration
  {
    namespace Transform
    {
          bool DoubleExpSmoothSlopeCheck::go_on (const Eigen::Matrix<default_type, Eigen::Dynamic, 1>& element) {
            assert (is_initialised);
            ++iter_count;
            // initialise
            if (len == 0) {
              if (!x0.size()) {
                x0 = element;
                return true;
              } else {
                ds.emplace_back(element);
                db.emplace_back(element - x0);
                if (check_all(db.back()))
                  ++stop_cnt;
                else
                  stop_cnt = 0;
                ++len;
                return true;
              }
            }
            // add smoothed elements
            ds.emplace_back(alpha * element + (1.0-alpha) * (ds.back() + db.back()));
            db.emplace_back(beta * (ds.at(len) - ds.at(len - 1)) + (1.0-beta) * db.at(len-1));
            DEBUG ("Smooth check b: " + str(db.back().transpose()));
            DEBUG ("Smooth check t: " + str(thresh.transpose()));
            if (check_all(db.back()))
              ++stop_cnt;
            else
              stop_cnt = 0;

            // trim if buffer full
            if (len == buffer_len) {
              ds.pop_front();
              db.pop_front();
              if (stop_cnt > buffer_len) --stop_cnt;
            } else {
              ++len;
            }
            return (stop_cnt != buffer_len) or (iter_count < min_iter);
          }

          void DoubleExpSmoothSlopeCheck::set_parameters (
              const Eigen::Matrix<default_type, Eigen::Dynamic, 1>& slope_threshold,
              default_type alpha_in,
              default_type beta_in,
              size_t buffer_length,
              size_t min_iter_in) {
            // set parameters and reset iter_count. only reset iter_count, keeps previously filled buffers
            thresh = slope_threshold;
            alpha = alpha_in;
            beta = beta_in;
            buffer_len = buffer_length;
            min_iter = min_iter_in;
            is_initialised = true;
            iter_count = 0;
          }

          bool DoubleExpSmoothSlopeCheck::last_b (Eigen::Matrix<default_type, Eigen::Dynamic, 1>& b) const {
            assert (is_initialised);
            if (!len) return false;
            b = db.back();
            return true;
          }

          bool DoubleExpSmoothSlopeCheck::last_s (Eigen::Matrix<default_type, Eigen::Dynamic, 1>& s) const {
            assert (is_initialised);
            if (!len) return false;
            s = ds.back();
            return true;
          }

          void DoubleExpSmoothSlopeCheck::debug (const Eigen::Matrix<default_type, Eigen::Dynamic, 1>& control_points_vec) const {
            if (!is_initialised) {
              WARN ("DoubleExpSmoothSlopeCheck not initialised");
              return;
            }
            std::cout << str(control_points_vec.transpose()) << std::endl;
            if (len == 0) {
              INFO ("DoubleExpSmoothSlopeCheck did not run");
              return;
            }

            std::cout << "#b " + str(db.back().transpose()) << std::endl;
            std::cout << "#s " + str(ds.back().transpose()) << std::endl;
            DEBUG ("bmax : " + str(db.back().array().abs().maxCoeff()));
          }
              //! @}
    }
  }
}
