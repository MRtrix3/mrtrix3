/*
 * Copyright (c) 2008-2018 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/
 *
 * MRtrix3 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/
 */


#include "dwi/tractography/tracking/shared.h"


namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {
      namespace Tracking
      {



        SharedBase::SharedBase (const std::string& diff_path, Properties& property_set) :
            source (Image<float>::open (diff_path).with_direct_io (3)),
            properties (property_set),
            init_dir ({ NaN, NaN, NaN }),
            min_num_points (0),
            max_num_points (0),
            max_angle (NaN),
            max_angle_rk4 (NaN),
            cos_max_angle (NaN),
            cos_max_angle_rk4 (NaN),
            step_size (NaN),
            threshold (NaN),
            unidirectional (false),
            rk4 (false),
            stop_on_all_include (false),
            implicit_max_num_seeds (properties.find ("max_num_seeds") == properties.end()),
            downsampler ()
#ifdef DEBUG_TERMINATIONS
          , debug_header (Header::open (properties.find ("act") == properties.end() ? diff_path : properties["act"])),
            transform (debug_header)
#endif
        {
          if (properties.find ("max_num_tracks") == properties.end())
            max_num_tracks = (properties.find ("max_num_seeds") == properties.end()) ? TCKGEN_DEFAULT_NUM_SELECTED_TRACKS : 0;
          properties.set (max_num_tracks, "max_num_tracks");

          properties.set (unidirectional, "unidirectional");
          properties.set (rk4, "rk4");
          properties.set (stop_on_all_include, "stop_on_all_include");

          properties["source"] = source.name();

          max_num_seeds = TCKGEN_DEFAULT_SEED_TO_SELECT_RATIO * max_num_tracks;
          properties.set (max_num_seeds, "max_num_seeds");

          assert (properties.seeds.num_seeds());
          max_seed_attempts = properties.seeds[0]->get_max_attempts();
          properties.set (max_seed_attempts, "max_seed_attempts");

          if (properties.find ("init_direction") != properties.end()) {
            auto V = parse_floats (properties["init_direction"]);
            if (V.size() != 3) throw Exception (std::string ("invalid initial direction \"") + properties["init_direction"] + "\"");
            init_dir[0] = V[0];
            init_dir[1] = V[1];
            init_dir[2] = V[2];
            init_dir.normalize();
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
          debug_header.ndim() = 3;
          debug_header.datatype() = DataType::UInt32;
          for (size_t i = 0; i != TERMINATION_REASON_COUNT; ++i) {
            std::string name;
            switch (i) {
              case CONTINUE:              name = "undefined";      break;
              case ENTER_CGM:             name = "enter_cgm";      break;
              case CALIBRATOR:            name = "calibrator";     break;
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
            debug_images[i] = new Image<uint32_t> (Image<uint32_t>::create ("terms_" + name + ".mif", debug_header));
          }
#endif

        }


        SharedBase::~SharedBase()
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
              case CALIBRATOR:           term_type = "Calibrator sub-threshold";      to_print = true;     break;
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
              INFO ("  " + term_type + ": " + str (100.0 * terminations[i] / (double)sum_terminations, 3) + "\%");
          }

          INFO ("Track rejection counts:");
          for (size_t i = 0; i != REJECTION_REASON_COUNT; ++i) {
            std::string reject_type;
            bool to_print = false;
            switch (i) {
              case INVALID_SEED:              reject_type = "Invalid seed point";              to_print = true;     break;
              case NO_PROPAGATION_FROM_SEED:  reject_type = "No propagation from seed";        to_print = true;     break;
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



        void SharedBase::set_step_size (float stepsize)
        {
          step_size = stepsize * vox();
          properties.set (step_size, "step_size");
          INFO ("step size = " + str (step_size) + " mm");

          if (downsampler.get_ratio() > 1)
            properties["output_step_size"] = str (step_size * downsampler.get_ratio());

          float max_dist = 100.0 * vox();
          properties.set (max_dist, "max_dist");
          max_num_points = std::round (max_dist/step_size) + 1;

          float min_dist = is_act() ? (2.0 * vox()) : (5.0 * vox());
          properties.set (min_dist, "min_dist");
          min_num_points = std::max (2, int(std::round (min_dist/step_size) + 1));

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



        void SharedBase::set_cutoff (float cutoff)
        {
          threshold = cutoff;
          properties.set (threshold, "threshold");
          init_threshold = threshold;
          properties.set (init_threshold, "init_threshold");
        }



#ifdef DEBUG_TERMINATIONS
        void SharedBase::add_termination (const term_t i, const Eigen::Vector3f& p) const
        {
          terminations[i].fetch_add (1, std::memory_order_relaxed);
          Image<uint32_t> image (*debug_images[i]);
          const auto pv = transform.scanner2voxel * p.cast<default_type>();
          image.index(0) = ssize_t (std::round (pv[0]));
          image.index(1) = ssize_t (std::round (pv[1]));
          image.index(2) = ssize_t (std::round (pv[2]));
          if (!is_out_of_bounds (image))
            image.value() += 1;
        }
#endif



      }
    }
  }
}

