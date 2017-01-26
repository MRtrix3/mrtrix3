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









        Worker::Thresholds::Thresholds (Tractography::Properties& properties) :
          max_length (std::numeric_limits<float>::infinity()),
          min_length (0.0f),
          max_weight (std::numeric_limits<float>::infinity()),
          min_weight (0.0f),
          step_size (NaN)
        {

          std::string step_size_string;
          if (properties.find ("output_step_size") == properties.end())
            step_size_string = ((properties.find ("step_size") == properties.end()) ? "0.0" : properties["step_size"]);
          else
            step_size_string = properties["output_step_size"];

          if (properties.find ("max_dist") != properties.end()) {
            try {
              max_length = to<float>(properties["max_dist"]);
            } catch (...) { }
          }
          if (properties.find ("min_dist") != properties.end()) {
            try {
              min_length = to<float>(properties["min_dist"]);
            } catch (...) { }
          }

          try {
            step_size = to<float>(step_size_string);
            // User may set these values to a precise value, which may then fail due to floating-point
            //   calculation of streamline length
            // Therefore throw a bit of error margin in here
            float error_margin = 0.1;
            if (properties.find ("downsample_factor") != properties.end())
              error_margin = 0.5 / to<float>(properties["downsample_factor"]);
            max_length += error_margin * step_size;
            min_length -= error_margin * step_size;
          } catch (...) { }

          if (properties.find ("max_weight") != properties.end())
            max_weight = to<float>(properties["max_weight"]);

          if (properties.find ("min_weight") != properties.end())
            min_weight = to<float>(properties["min_weight"]);

        }




        Worker::Thresholds::Thresholds (const Worker::Thresholds& that) :
          max_length (that.max_length),
          min_length (that.min_length),
          max_weight (that.max_weight),
          min_weight (that.min_weight),
          step_size  (that.step_size) { }




        bool Worker::Thresholds::operator() (const Streamline<>& in) const
        {
          const float length = (std::isfinite (step_size) ? in.calc_length (step_size) : in.calc_length());
          return ((length <= max_length) &&
              (length >= min_length) &&
              (in.weight <= max_weight) &&
              (in.weight >= min_weight));
        }





      }
    }
  }
}

