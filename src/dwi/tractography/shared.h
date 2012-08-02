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
#include "image/buffer_preload.h"
#include "image/header.h"
#include "dwi/tractography/file.h"
#include "dwi/tractography/roi.h"

#define MAX_TRIALS 1000

namespace MR {
  namespace DWI {
    namespace Tractography {

      namespace {
        std::vector<ssize_t> strides_by_volume () {
          std::vector<ssize_t> S (4, 0);
          S[3] = 1;
          return S;
        }

      }

      typedef Image::BufferPreload<float> SourceBufferType;
      typedef SourceBufferType::value_type value_type;

      template <class VoxelType>
        class Interpolator {
          public:
            typedef Image::Interp::Linear<VoxelType> type;
        };


      class SharedBase {
        public:

          SharedBase (const std::string& source_name, DWI::Tractography::Properties& property_set) :
            source_buffer (source_name, strides_by_volume()),
            source_voxel (source_buffer),
            properties (property_set), 
            max_num_tracks (1000),
            max_angle (NAN),
            max_angle_rk4 (NAN),
            cos_max_angle (NAN),
            cos_max_angle_rk4 (NAN),
            step_size (NAN),
            threshold (0.1), 
            unidirectional (false),
            rk4 (false) {

              properties.set (threshold, "threshold");
              properties.set (unidirectional, "unidirectional");
              properties.set (max_num_tracks, "max_num_tracks");
              properties.set (rk4, "rk4");

              properties["source"] = source_buffer.name();

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

            }


          value_type vox () const 
          {
            return Math::pow (source_buffer.vox(0)*source_buffer.vox(1)*source_buffer.vox(2), value_type (1.0/3.0));
          }

          void set_step_size (value_type stepsize) 
          {
            step_size = stepsize * vox();
            properties.set (step_size, "step_size");
            INFO ("step size = " + str (step_size) + " mm");

            value_type max_dist = 100.0 * vox();
            properties.set (max_dist, "max_dist");
            max_num_points = round (max_dist/step_size);

            value_type min_dist = 5.0 * vox();
            properties.set (min_dist, "min_dist");
            min_num_points = round (min_dist/step_size);

            max_angle = 90.0 * step_size / vox();
            properties.set (max_angle, "max_angle");
            INFO ("maximum deviation angle = " + str (max_angle) + "°");
            max_angle *= M_PI / 180.0;
            cos_max_angle = Math::cos (max_angle);

            if (rk4) {
              max_angle_rk4 = max_angle;
              cos_max_angle_rk4 = cos_max_angle;
              max_angle = M_PI;
              cos_max_angle = 0.0;
            }
          }

          SourceBufferType source_buffer;
          SourceBufferType::voxel_type source_voxel;
          DWI::Tractography::Properties& properties;
          Point<value_type> init_dir;
          size_t max_num_tracks, max_num_attempts, min_num_points, max_num_points;
          value_type max_angle, max_angle_rk4, cos_max_angle, cos_max_angle_rk4;
          value_type step_size, threshold, init_threshold;
          bool unidirectional;
          bool rk4;

      };


    }
  }
}

#endif

