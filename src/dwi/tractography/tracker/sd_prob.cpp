/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 27/06/08.

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

#include "dwi/tractography/tracker/sd_prob.h"

namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace Tracker {


        SDProb::SDProb (Image::Object& source_image, Properties& properties) : 
          Base (source_image, properties),
          lmax (Math::SH::LforN (source.dim(3))),
          max_trials (50),
          precomputed (true),
          precomputer (NULL)
        {
          float min_curv = 1.0; 
          properties["method"] = "SD_PROB";
          if (props["min_curv"].empty()) props["min_curv"] = str (min_curv); else min_curv = to<float> (props["min_curv"]);
          if (props["max_num_tracks"].empty()) props["max_num_tracks"] = "1000";

          if (props["lmax"].empty()) props["lmax"] = str (lmax); else lmax = to<int> (props["lmax"]);
          if (props["max_trials"].empty()) props["max_trials"] = str (max_trials); else max_trials = to<int> (props["max_trials"]);
          if (props["sh_precomputed"].empty()) props["sh_precomputed"] = ( precomputed ? "1" : "0" ); else precomputed = to<int> (props["sh_precomputed"]);
          if (precomputed) precomputer = new Math::SH::PrecomputedAL<float> (lmax, 256);

          dist_spread = curv2angle (step_size, min_curv);
        }





        bool SDProb::init_direction (const Point& seed_dir)
        {
          float values [source.dim(3)];
          if (get_source_data (pos, values)) return (true);

          if (!seed_dir) {
            for (int n = 0; n < max_trials; n++) {
              dir.set (rng.normal(), rng.normal(), rng.normal());
              dir.normalise();
              float val = precomputed ? precomputer->value (values, dir) : Math::SH::value (values, dir, lmax);

              if (!isnan (val)) if (val > init_threshold) return (false);
            } 
          }
          else {
            dir = seed_dir;
            float val = precomputed ? 
              precomputer->value (values, dir) :
              Math::SH::value (values, dir, lmax);

            if (finite (val)) if (val > init_threshold) return (false);
          }

          return (true);
        }




        bool SDProb::next_point ()
        {
          float values [source.dim(3)];
          if (get_source_data (pos, values)) return (true);

          float max_val = 0.0;
          for (int n = 0; n < 12; n++) {
            Point new_dir = new_rand_dir();
            float val = precomputed ? 
              precomputer->value (values, new_dir) : 
              Math::SH::value<float> (values, new_dir, lmax);

            if (val > max_val) max_val = val;
          }

          if (isnan (max_val)) return (true);
          if (max_val < threshold) return (true);
          max_val *= 1.5;

          for (int n = 0; n < max_trials; n++) {
            Point new_dir = new_rand_dir();
            float val = precomputed ? 
              precomputer->value (values, new_dir) : 
              Math::SH::value<float> (values, new_dir, lmax);

            if (val > threshold) {
              if (val > max_val) info ("max_val exceeded!!! (val = " + str(val) + ", max_val = " + str (max_val) + ")");
              if (rng.uniform() < val/max_val) {
                dir = new_dir;
                inc_pos ();
                return (false);
              }
            }
          }

          return (true);
        }


      }
    }
  }
}


