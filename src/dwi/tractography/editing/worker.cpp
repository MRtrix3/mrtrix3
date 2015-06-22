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




        bool Worker::operator() (const Tractography::Streamline<>& in, Tractography::Streamline<>& out) const
        {

          out.clear();
          out.index = in.index;
          out.weight = in.weight;

          if (!thresholds (in)) {
            // Want to test thresholds before wasting time on upsampling; but if -inverse is set,
            //   still need to apply both the upsampler and downsampler before writing to output
            if (inverse) {
              std::vector< Point<float> > tck (in);
              upsampler (tck);
              downsampler (tck);
              tck.swap (out);
            }
            return true;
          }

          // Upsample track before mapping to ROIs
          std::vector< Point<float> > tck (in);
          upsampler (tck);

          // Assign to ROIs
          if (properties.include.size() || properties.exclude.size()) {

            include_visited.assign (properties.include.size(), false);

            if (ends_only) {
              for (size_t i = 0; i != 2; ++i) {
                const Point<float>& p (i ? tck.back() : tck.front());
                properties.include.contains (p, include_visited);
                if (properties.exclude.contains (p)) {
                  if (inverse) {
                    downsampler (tck);
                    tck.swap (out);
                  }
                  return true;
                }
              }
            } else {
              for (std::vector< Point<float> >::const_iterator p = tck.begin(); p != tck.end(); ++p) {
                properties.include.contains (*p, include_visited);
                if (properties.exclude.contains (*p)) {
                  if (inverse) {
                    downsampler (tck);
                    tck.swap (out);
                  }
                  return true;
                }
              }
            }

            // Make sure all of the include regions were visited
            for (std::vector<bool>::const_iterator i = include_visited.begin(); i != include_visited.end(); ++i) {
              if (!*i) {
                if (inverse) {
                  downsampler (tck);
                  tck.swap (out);
                }
                return true;
              }
            }

          }

          if (properties.mask.size()) {

            // Split tck into separate tracks based on the mask
            std::vector< std::vector< Point<float> > > cropped_tracks;
            std::vector< Point<float> > temp;

            for (std::vector< Point<float> >::const_iterator p = tck.begin(); p != tck.end(); ++p) {
              const bool contains = properties.mask.contains (*p);
              if (contains == inverse) {
                if (temp.size() >= 2)
                  cropped_tracks.push_back (temp);
                temp.clear();
              } else {
                temp.push_back (*p);
              }
            }
            if (temp.size() >= 2)
              cropped_tracks.push_back (temp);

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
            out.push_back (Point<float>());
            return true;

          } else {

            if (!inverse) {
              downsampler (tck);
              tck.swap (out);
            }
            return true;

          }

        }









        Worker::Thresholds::Thresholds (Tractography::Properties& properties) :
          max_length (std::numeric_limits<float>::infinity()),
          min_length (0.0f),
          max_weight (std::numeric_limits<float>::infinity()),
          min_weight (0.0f),
          step_size (NAN)
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
            max_length += 0.1 * step_size;
            min_length -= 0.1 * step_size;
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




        bool Worker::Thresholds::operator() (const Tractography::Streamline<>& in) const
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

