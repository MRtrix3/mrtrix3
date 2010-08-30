/*
    Copyright 2009 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 25/10/09.

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

#ifndef __dwi_tractography_shared_h__
#define __dwi_tractography_shared_h__

#include "point.h"
#include "image/header.h"
#include "image/buffer.h"
#include "dwi/tractography/file.h"
#include "dwi/tractography/roi.h"

namespace MR {
  namespace DWI {
    namespace Tractography {

      typedef float value_type;
      typedef Image::Buffer<value_type> StorageType;

      class SharedBase {
        public:

          SharedBase (Image::Header& source_header, DWI::Tractography::Properties& property_set) :
            H (source_header),
            source (H, strides_by_volume(), 4),
            properties (property_set), 
            max_num_tracks (1000),
            max_angle (45.0),
            step_size (0.1),
            threshold (0.1), 
            unidirectional (false) 
        {
          value_type max_dist = 200.0;
          value_type min_dist = 10.0;

          properties["source"] = source.name();

          properties.set (step_size, "step_size");
          properties.set (threshold, "threshold");
          properties.set (max_angle, "max_angle");
          properties.set (unidirectional, "unidirectional");
          properties.set (max_num_tracks, "max_num_tracks");
          properties.set (max_dist, "max_dist");
          properties.set (min_dist, "min_dist");

          init_threshold = 2.0*threshold;
          properties.set (init_threshold, "init_threshold");

          max_num_attempts = 100 * max_num_tracks;
          properties.set (max_num_attempts, "max_num_attempts");

          if (properties["init_direction"].size()) {
            std::vector<float> V = parse_floats (properties["init_direction"]);
            if (V.size() != 3) throw Exception (std::string ("invalid initial direction \"") + properties["init_direction"] + "\"");
            init_dir[0] = V[0];
            init_dir[1] = V[1];
            init_dir[2] = V[2];
            init_dir.normalise();
          }

          max_num_points = round (max_dist/step_size);
          min_num_points = round (min_dist/step_size);

          max_angle *= M_PI / 180.0;
        }

          Image::Header& H;
          StorageType source;
          DWI::Tractography::Properties& properties;
          Point<value_type> init_dir;
          size_t max_num_tracks, max_num_attempts, min_num_points, max_num_points;
          value_type max_angle, step_size, threshold, init_threshold;
          bool unidirectional;

        private:
          static const std::vector<ssize_t>& strides_by_volume ();
      };

    }
  }
}

#endif

