/* Copyright (c) 2008-2019 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Covered Software is provided under this License on an "as is"
 * basis, without warranty of any kind, either expressed, implied, or
 * statutory, including, without limitation, warranties that the
 * Covered Software is free of defects, merchantable, fit for a
 * particular purpose or non-infringing.
 * See the Mozilla Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
 */

#include "dwi/tractography/editing/worker.h"


namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace Editing {




        bool Worker::operator() (Streamline<>& in, Streamline<>& out) const
        {

          out.clear();
          out.index = in.index;
          out.weight = in.weight;

          if (!thresholds (in)) {
            // Want to test thresholds before wasting time on resampling
            if (inverse)
              in.swap (out);
            return true;
          }

          // Assign to ROIs
          if (properties.include.size() || properties.exclude.size()) {
            include_visited.assign (properties.include.size(), false);

            if (ends_only) {
              for (size_t i = 0; i != 2; ++i) {
                const Eigen::Vector3f& p (i ? in.back() : in.front());
                properties.include.contains (p, include_visited);
                if (properties.exclude.contains (p)) {
                  if (inverse)
                    in.swap (out);
                  return true;
                }
              }
            } else {
              for (const auto& p : in) {
                properties.include.contains (p, include_visited);
                if (properties.exclude.contains (p)) {
                  if (inverse)
                    in.swap (out);
                  return true;
                }
              }
            }

            // Make sure all of the include regions were visited
            for (const auto& i : include_visited) {
              if (!i) {
                if (inverse)
                  in.swap (out);
                return true;
              }
            }

          }

          if (properties.mask.size()) {

            // Split tck into separate tracks based on the mask
            vector<vector<Eigen::Vector3f>> cropped_tracks;
            vector<Eigen::Vector3f> temp;

            for (const auto& p : in) {
              const bool contains = properties.mask.contains (p);
              if (contains == inverse) {
                if (temp.size() >= 2)
                  cropped_tracks.push_back (temp);
                temp.clear();
              } else {
                temp.push_back (p);
              }
            }
            if (temp.size() >= 2)
              cropped_tracks.push_back (temp);

            if (cropped_tracks.empty())
              return true;

            if (cropped_tracks.size() == 1) {
              cropped_tracks[0].swap (out);
              return true;
            }

            // Stitch back together in preparation for sending down queue as a single track
            out.push_back ({ NaN, NaN, NaN });
            for (const auto& i : cropped_tracks) {
              for (const auto& p : i)
                out.push_back (p);
              out.push_back ({ NaN, NaN, NaN });
            }
            return true;

          } else {

            if (!inverse)
              in.swap (out);
            return true;

          }

        }









        Worker::Thresholds::Thresholds (Tractography::Properties& properties)
        {
          static const float Inf = std::numeric_limits<float>::infinity();

          step_size = properties.get_step_size();

          try {
            max_length = to<float>(properties.at("max_dist"));
          } catch (...) {
            max_length = Inf;
          }

          try {
            min_length = to<float>(properties.at("min_dist"));
          } catch (...) {
            min_length = 0.0f;
          }

          if (std::isfinite (step_size)) {
            // User may set these values to a precise value, which may then fail due to floating-point
            //   calculation of streamline length
            // Therefore throw a bit of error margin in here
            float error_margin = 0.5f / properties.value_or_default<float>( "downsample_factor", 5.0f );

            max_length += error_margin * step_size;
            min_length -= error_margin * step_size;
          }

          max_weight = properties.value_or_default<float>( "max_weight", Inf );
          min_weight = properties.value_or_default<float>( "min_weight", 0.0f );
        }




        Worker::Thresholds::Thresholds (const Worker::Thresholds& that) :
          max_length (that.max_length),
          min_length (that.min_length),
          max_weight (that.max_weight),
          min_weight (that.min_weight),
          step_size  (that.step_size) { }




        bool Worker::Thresholds::operator() (const Streamline<>& in) const
        {
          const float length = std::isfinite (step_size) ? in.calc_length (step_size) : in.calc_length();
          return (length <= max_length) &&
              (length >= min_length) &&
              (in.weight <= max_weight) &&
              (in.weight >= min_weight);
        }





      }
    }
  }
}

