/* Copyright (c) 2008-2023 the MRtrix3 contributors.
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
          out.set_index (in.get_index());
          out.weight = in.weight;

          // Need to track exclusion separately, since we may still need to apply
          //   mask (or, more accurately, their inverse) afterwards if -inverse is specified
          bool exclude = false;

          if (!thresholds (in)) {
            exclude = true;
          } else if (include_visitation.size() || properties.exclude.size()) {

            // Assign to ROIs
            include_visitation.reset();

            if (ends_only) {
              for (size_t i = 0; i != 2; ++i) {
                const Eigen::Vector3f& p (i ? in.back() : in.front());
                include_visitation (p);
                if (properties.exclude.contains (p)) {
                  exclude = true;
                  break;
                }
              }
            } else {
              for (const auto& p : in) {
                include_visitation (p);
                if (properties.exclude.contains (p)) {
                  exclude = true;
                  break;
                }
              }
            }

            // Make sure all of the include regions were visited
            if (!include_visitation)
               exclude = true;

          } else if (inverse) {

            // If no thresholds are specified, and no include / exclude ROIs are defined, then
            //   it's still possible that one or more masks have been provided;
            //   if this is the case, then we want to continue processing this streamline,
            //   regardless of whether or not -inverse has been specified
            exclude = true;

          }

          // In default usage, pass the empty track down the queue if track is excluded
          // If inverse selection is sought, pass the empty track if it did not fail any criteria
          if (exclude != inverse)
            return true;

          if (!properties.mask.size()) {
            std::swap (in, out);
            return true;
          }

          // Split tck into separate tracks based on the mask
          vector<vector<Eigen::Vector3f>> cropped_tracks;
          vector<Eigen::Vector3f> temp;

          for (const auto& p : in) {
            const bool contains = properties.mask.contains (p);
            // "Inverse" applies to masks in addition to selection criteria
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
        }









        Worker::Thresholds::Thresholds (Tractography::Properties& properties) :
          max_length (std::numeric_limits<float>::infinity()),
          min_length (0.0f),
          max_weight (std::numeric_limits<float>::infinity()),
          min_weight (0.0f),
          step_size (properties.get_stepsize())
        {
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

          if (std::isfinite (step_size)) {
            // User may set these values to a precise value, which may then fail due to floating-point
            //   calculation of streamline length
            // Therefore throw a bit of error margin in here
            float error_margin = 0.1;
            if (properties.find ("downsample_factor") != properties.end())
              error_margin = 0.5 / to<float>(properties["downsample_factor"]);
            max_length += error_margin * step_size;
            min_length -= error_margin * step_size;
          }

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
          const float length = Tractography::length (in);
          return ((length <= max_length) &&
              (length >= min_length) &&
              (in.weight <= max_weight) &&
              (in.weight >= min_weight));
        }





      }
    }
  }
}
