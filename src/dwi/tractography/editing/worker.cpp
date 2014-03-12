/*
   Copyright 2011 Brain Research Institute, Melbourne, Australia

   Written by Robert E. Smith, 2014.

   This file is part of MRtrix.

   MRtrix is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   MRtrix is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with MRtrix.  If not, see <http://www.gnu.org/licenses/>.

 */


#include "dwi/tractography/editing/worker.h"


namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace Editing {




        bool Worker::operator() (const Tractography::Streamline<>& in, Tractography::Streamline<>& out)
        {

          out.clear();
          out.index = in.index;
          out.weight = in.weight;

          if (!thresholds (in))
            return true;

          std::vector< Point<float> > tck (in);

          // Upsample track before mapping to ROIs
          if (upsampler.valid()) {
            if (!upsampler (tck))
              return false;
          }

          // Assign to ROIs
          include_visited.assign (properties.include.size(), false);
          for (std::vector< Point<float> >::const_iterator p = tck.begin(); p != tck.end(); ++p) {
            properties.include.contains (*p, include_visited);
            if (properties.exclude.contains (*p))
              return true;
          }

          // Make sure all of the include regions were visited
          for (std::vector<bool>::const_iterator i = include_visited.begin(); i != include_visited.end(); ++i) {
            if (!*i)
              return true;
          }

          if (properties.mask.size()) {

            // Split tck into separate tracks based on the mask
            std::vector< std::vector< Point<float> > > cropped_tracks;
            std::vector< Point<float> > temp;

            for (std::vector< Point<float> >::const_iterator p = tck.begin(); p != tck.end(); ++p) {
              if (properties.mask.contains (*p)) {
                temp.push_back (*p);
              } else {
                if (temp.size() >= 2)
                  cropped_tracks.push_back (temp);
                temp.clear();
              }
            }

            if (cropped_tracks.empty())
              return true;

            // Apply downsampler independently to each
            for (std::vector< std::vector< Point<float> > >::iterator i = cropped_tracks.begin(); i != cropped_tracks.end(); ++i)
              downsampler (*i);

            if (cropped_tracks.size() == 1) {
              cropped_tracks[0].swap (out);
              return true;
            }

            // Stitch back together in preparation for sending down queue as a single track
            out.push_back (Point<float>());
            for (std::vector< std::vector< Point<float> > >::const_iterator i = cropped_tracks.begin(); i != cropped_tracks.end(); ++i) {
              for (std::vector< Point<float> >::const_iterator p = i->begin(); p != i->end(); ++p)
                out.push_back (*p);
              out.push_back (Point<float>());
            }
            return true;

          } else {
            downsampler (tck);
            tck.swap (out);
            return true;
          }

        }








        Worker::Thresholds::Thresholds (Tractography::Properties& properties) :
          max_num_points (std::numeric_limits<size_t>::max()),
          min_num_points (0),
          max_weight (std::numeric_limits<float>::infinity()),
          min_weight (0.0)
        {

          std::string step_size_string;
          if (properties.find ("output_step_size") == properties.end())
            step_size_string = ((properties.find ("step_size") == properties.end()) ? "0.0" : properties["step_size"]);
          else
            step_size_string = properties["output_step_size"];

          const bool max_dist_set = (properties.find ("max_dist") != properties.end());
          const bool min_dist_set = (properties.find ("min_dist") != properties.end());

          if (step_size_string == "variable" && (max_dist_set || min_dist_set))
            throw Exception ("Cannot apply length threshold; step size is inconsistent between input track files");

          const float step_size = to<float>(step_size_string);

          if (max_dist_set) {
            if (!step_size || !std::isfinite (step_size))
              throw Exception ("Cannot apply length threshold; step size information is incomplete");
            const float maxlength = to<float>(properties["max_dist"]);
            max_num_points = Math::round (maxlength / step_size) + 1;
          }

          if (min_dist_set) {
            if (!step_size || !std::isfinite (step_size))
              throw Exception ("Cannot apply length threshold; step size information is incomplete");
            const float minlength = to<float>(properties["min_dist"]);
            min_num_points = std::max (2, round (minlength/step_size) + 1);
          }

          if (properties.find ("max_weight") != properties.end())
            max_weight = to<float>(properties["max_weight"]);

          if (properties.find ("min_weight") != properties.end())
            min_weight = to<float>(properties["min_weight"]);

        }




        Worker::Thresholds::Thresholds (const Worker::Thresholds& that) :
          max_num_points (that.max_num_points),
          min_num_points (that.min_num_points),
          max_weight (that.max_weight),
          min_weight (that.min_weight) { }




        bool Worker::Thresholds::operator() (const Tractography::Streamline<>& in) const
        {
          return ((in.size() <= max_num_points) &&
              (in.size() >= min_num_points) &&
              (in.weight <= max_weight) &&
              (in.weight >= min_weight));
        }





      }
    }
  }
}

