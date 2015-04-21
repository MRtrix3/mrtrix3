/*
   Copyright 2011 Brain Research Institute, Melbourne, Australia

   Written by J-Donald Tournier and Robert E. Smith, 2011.

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

#ifndef __dwi_tractography_tracking_shared_h__
#define __dwi_tractography_tracking_shared_h__

#include <vector>


#include "point.h"
#include "memory.h"
#include "image/nav.h"
#include "image/header.h"
#include "image/transform.h"
#include "dwi/tractography/properties.h"
#include "dwi/tractography/resample.h"
#include "dwi/tractography/roi.h"
#include "dwi/tractography/ACT/shared.h"
#include "dwi/tractography/tracking/types.h"

#define MAX_TRIALS 1000


// If this is enabled, images will be output in the current directory showing the density of streamline terminations due to different termination mechanisms throughout the brain
//#define DEBUG_TERMINATIONS


namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {
      namespace Tracking
      {




        namespace {
          std::vector<ssize_t> strides_by_volume () {
            std::vector<ssize_t> S (4, 0);
            S[3] = 1;
            return S;
          }
        }




        class SharedBase {

          public:

            SharedBase (const std::string& diff_path, DWI::Tractography::Properties& property_set) :

              source_buffer (diff_path, strides_by_volume()),
              source_voxel (source_buffer),
              properties (property_set),
              max_num_tracks (1000),
              min_num_points (0),
              max_num_points (0),
              max_angle (NAN),
              max_angle_rk4 (NAN),
              cos_max_angle (NAN),
              cos_max_angle_rk4 (NAN),
              step_size (NAN),
              threshold (0.1),
              unidirectional (false),
              rk4 (false),
              stop_on_all_include (false),
              downsampler ()
#ifdef DEBUG_TERMINATIONS
            , debug_header (properties.find ("act") == properties.end() ? diff_path : properties["act"]),
              transform  (debug_header)
#endif
              {

                properties.set (threshold, "threshold");
                properties.set (unidirectional, "unidirectional");
                properties.set (max_num_tracks, "max_num_tracks");
                properties.set (rk4, "rk4");
                properties.set (stop_on_all_include, "stop_on_all_include");

                properties["source"] = source_buffer.name();

                init_threshold = threshold;
                properties.set (init_threshold, "init_threshold");

                max_num_attempts = 100 * max_num_tracks;
                properties.set (max_num_attempts, "max_num_attempts");

                assert (properties.seeds.num_seeds());
                max_seed_attempts = properties.seeds[0]->get_max_attempts();
                properties.set (max_seed_attempts, "max_seed_attempts");

                if (properties.find ("init_direction") != properties.end()) {
                  std::vector<float> V = parse_floats (properties["init_direction"]);
                  if (V.size() != 3) throw Exception (std::string ("invalid initial direction \"") + properties["init_direction"] + "\"");
                  init_dir[0] = V[0];
                  init_dir[1] = V[1];
                  init_dir[2] = V[2];
                  init_dir.normalise();
                }

                if (properties.find ("act") != properties.end()) {
                  act_shared_additions.reset (new ACT::ACT_Shared_additions (properties["act"], property_set));
                  if (act().backtrack() && stop_on_all_include)
                    throw Exception ("Cannot use -stop option if ACT backtracking is enabled");
                }

                if (properties.find ("downsample_factor") != properties.end())
                  downsampler.set_ratio (to<int> (properties["downsample_factor"]));

                for (size_t i = 0; i != TERMINATION_REASON_COUNT; ++i)
                  terminations[i] = 0;
                for (size_t i = 0; i != REJECTION_REASON_COUNT; ++i)
                  rejections[i] = 0;

#ifdef DEBUG_TERMINATIONS
                debug_header.set_ndim (3);
                debug_header.datatype() = DataType::UInt32;
                for (size_t i = 0; i != TERMINATION_REASON_COUNT; ++i) {
                  std::string name;
                  switch (i) {
                    case CONTINUE:              name = "undefined";      break;
                    case ENTER_CGM:             name = "enter_cgm";      break;
                    case CALIBRATE_FAIL:        name = "calibrate_fail"; break;
                    case EXIT_IMAGE:            name = "exit_image";     break;
                    case ENTER_CSF:             name = "enter_csf";      break;
                    case BAD_SIGNAL:            name = "bad_signal";     break;
                    case HIGH_CURVATURE:        name = "curvature";      break;
                    case LENGTH_EXCEED:         name = "max_length";     break;
                    case TERM_IN_SGM:           name = "term_in_sgm";    break;
                    case EXIT_SGM:              name = "exit_sgm";       break;
                    case EXIT_MASK:             name = "exit_mask";      break;
                    case ENTER_EXCLUDE:         name = "enter_exclude";  break;
                    case TRAVERSE_ALL_INCLUDE:  name = "all_include";    break;
                  }
                  debug_images[i] = new Image::Buffer<uint32_t>("terms_" + name + ".mif", debug_header);
                }
#endif

              }


            virtual ~SharedBase()
            {

              size_t sum_terminations = 0;
              for (size_t i = 0; i != TERMINATION_REASON_COUNT; ++i)
                sum_terminations += terminations[i];
              INFO ("Total number of track terminations: " + str (sum_terminations));
              INFO ("Termination reason probabilities:");
              for (size_t i = 0; i != TERMINATION_REASON_COUNT; ++i) {
                std::string term_type;
                bool to_print = false;
                switch (i) {
                  case CONTINUE:             term_type = "Unknown";                       to_print = false;    break;
                  case ENTER_CGM:            term_type = "Entered cortical grey matter";  to_print = is_act(); break;
                  case CALIBRATE_FAIL:       term_type = "Calibrator failed";             to_print = true;     break;
                  case EXIT_IMAGE:           term_type = "Exited image";                  to_print = true;     break;
                  case ENTER_CSF:            term_type = "Entered CSF";                   to_print = is_act(); break;
                  case BAD_SIGNAL:           term_type = "Bad diffusion signal";          to_print = true;     break;
                  case HIGH_CURVATURE:       term_type = "Excessive curvature";           to_print = true;     break;
                  case LENGTH_EXCEED:        term_type = "Max length exceeded";           to_print = true;     break;
                  case TERM_IN_SGM:          term_type = "Terminated in subcortex";       to_print = is_act(); break;
                  case EXIT_SGM:             term_type = "Exiting sub-cortical GM";       to_print = is_act(); break;
                  case EXIT_MASK:            term_type = "Exited mask";                   to_print = properties.mask.size(); break;
                  case ENTER_EXCLUDE:        term_type = "Entered exclusion region";      to_print = properties.exclude.size(); break;
                  case TRAVERSE_ALL_INCLUDE: term_type = "Traversed all include regions"; to_print = stop_on_all_include; break;
                }
                if (to_print)
                  INFO ("  " + term_type + ": " + str (100.0 * terminations[i] / (double)sum_terminations) + "\%");
              }

              INFO ("Track rejection counts:");
              for (size_t i = 0; i != REJECTION_REASON_COUNT; ++i) {
                std::string reject_type;
                bool to_print = false;
                switch (i) {
                  case TRACK_TOO_SHORT:           reject_type = "Shorter than minimum length";     to_print = true;     break;
                  case TRACK_TOO_LONG:            reject_type = "Longer than maximum length";      to_print = is_act(); break;
                  case ENTER_EXCLUDE_REGION:      reject_type = "Entered exclusion region";        to_print = properties.exclude.size(); break;
                  case MISSED_INCLUDE_REGION:     reject_type = "Missed inclusion region";         to_print = properties.include.size(); break;
                  case ACT_POOR_TERMINATION:      reject_type = "Poor structural termination";     to_print = is_act(); break;
                  case ACT_FAILED_WM_REQUIREMENT: reject_type = "Failed to traverse white matter"; to_print = is_act(); break;
                }
                if (to_print)
                  INFO ("  " + reject_type + ": " + str (rejections[i]));
              }

#ifdef DEBUG_TERMINATIONS
              for (size_t i = 0; i != TERMINATION_REASON_COUNT; ++i) {
                delete debug_images[i];
                debug_images[i] = NULL;
              }
#endif

            }


            SourceBufferType source_buffer;
            SourceBufferType::voxel_type source_voxel;
            DWI::Tractography::Properties& properties;
            Point<value_type> init_dir;
            size_t max_num_tracks, max_num_attempts, min_num_points, max_num_points;
            value_type max_angle, max_angle_rk4, cos_max_angle, cos_max_angle_rk4;
            value_type step_size, threshold, init_threshold;
            size_t max_seed_attempts;
            bool unidirectional, rk4, stop_on_all_include;
            Downsampler downsampler;

            // Additional members for ACT
            bool is_act() const { return bool (act_shared_additions); }
            const ACT::ACT_Shared_additions& act() const { return *act_shared_additions; }



            value_type vox () const
            {
              return std::pow (source_buffer.vox(0)*source_buffer.vox(1)*source_buffer.vox(2), value_type (1.0/3.0));
            }

            void set_step_size (value_type stepsize)
            {
              step_size = stepsize * vox();
              properties.set (step_size, "step_size");
              INFO ("step size = " + str (step_size) + " mm");

              if (downsampler.get_ratio() > 1)
                properties["output_step_size"] = str (step_size * downsampler.get_ratio());

              value_type max_dist = 100.0 * vox();
              properties.set (max_dist, "max_dist");
              max_num_points = std::round (max_dist/step_size) + 1;

              value_type min_dist = is_act() ? (2.0 * vox()) : (5.0 * vox());
              properties.set (min_dist, "min_dist");
              min_num_points = std::fmax (2, std::round (min_dist/step_size) + 1);

              max_angle = 90.0 * step_size / vox();
              properties.set (max_angle, "max_angle");
              INFO ("maximum deviation angle = " + str (max_angle) + " deg");
              max_angle *= Math::pi / 180.0;
              cos_max_angle = std::cos (max_angle);

              if (rk4) {
                max_angle_rk4 = max_angle;
                cos_max_angle_rk4 = cos_max_angle;
                max_angle = Math::pi;
                cos_max_angle = 0.0;
              }
            }


            // This gets overloaded for iFOD2, as each sample is output rather than just each step, and there are
            //   multiple samples per step
            virtual float internal_step_size() const { return step_size; }


            void add_termination (const term_t i)   const { ++terminations[i]; }
            void add_rejection   (const reject_t i) const { ++rejections[i]; }


#ifdef DEBUG_TERMINATIONS
            void add_termination (const term_t i, const Point<value_type>& p) const
            {
              ++terminations[i];
              auto voxel debug_images[i]->voxel();
              const Point<value_type> pv = transform.scanner2voxel (p);
              const Point<int> v (std::round (pv[0]), std::round (pv[1]), std::round (pv[2]));
              if (Image::Nav::within_bounds (voxel, v)) {
                Image::Nav::set_pos (voxel, v);
                voxel.value() += 1;
              }
            }
#endif


          private:
            mutable size_t terminations[TERMINATION_REASON_COUNT];
            mutable size_t rejections  [REJECTION_REASON_COUNT];

            std::unique_ptr<ACT::ACT_Shared_additions> act_shared_additions;

#ifdef DEBUG_TERMINATIONS
            Image::Header debug_header;
            Image::Buffer<uint32_t>* debug_images[TERMINATION_REASON_COUNT];
            const Image::Transform transform;
#endif


        };

      }
    }
  }
}

#endif

