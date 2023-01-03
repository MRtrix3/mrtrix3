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
            min_num_points_preds (0),
            max_num_points_preds (0),
            min_num_points_postds (0),
            max_num_points_postds (0),
            min_dist (NaN),
            max_dist (NaN),
            max_angle_1o (NaN),
            max_angle_ho (NaN),
            cos_max_angle_1o (NaN),
            cos_max_angle_ho (NaN),
            step_size (NaN),
            min_radius (NaN),
            threshold (NaN),
            unidirectional (false),
            rk4 (false),
            stop_on_all_include (false),
            implicit_max_num_seeds (properties.find ("max_num_seeds") == properties.end()),
            downsampler (1)
#ifdef DEBUG_TERMINATIONS
          , debug_header (Header::open (properties.find ("act") == properties.end() ? diff_path : properties["act"])),
            transform (debug_header)
#endif
        {
          if (properties.find ("max_num_tracks") == properties.end())
            max_num_tracks = (properties.find ("max_num_seeds") == properties.end()) ? Defaults::num_selected_tracks : 0;
          properties.set (max_num_tracks, "max_num_tracks");

          properties.set (unidirectional, "unidirectional");
          properties.set (rk4, "rk4");
          properties.set (stop_on_all_include, "stop_on_all_include");

          properties["source"] = source.name();

          max_num_seeds = Defaults::seed_to_select_ratio * max_num_tracks;
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
              case MODEL:                 name = "model";          break;
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
              case MODEL:                term_type = "Diffusion model sub-threshold"; to_print = true;     break;
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



        void SharedBase::set_step_and_angle (const float voxel_frac, const float angle, const bool is_higher_order)
        {
          step_size = voxel_frac * vox();
          properties.set (step_size, "step_size");
          INFO ("step size = " + str (step_size) + " mm");

          max_dist = Defaults::maxlength_voxels * vox();
          properties.set (max_dist, "max_dist");

          min_dist = is_act() ?
                     (Defaults::minlength_voxels_withact * vox()) :
                     (Defaults::minlength_voxels_noact * vox());
          properties.set (min_dist, "min_dist");

          max_angle_1o = angle;
          properties.set (max_angle_1o, "max_angle");
          const std::string angle_msg = is_higher_order ?
                                        "maximum angular change in fibre orientation per step" :
                                        "maximum deviation angle per step";
          INFO (angle_msg + " = " + str (max_angle_1o) + " deg");
          max_angle_1o *= Math::pi / 180.0;
          cos_max_angle_1o = std::cos (max_angle_1o);
          min_radius = step_size / (2.0f * std::sin (0.5f * max_angle_1o));
          INFO ("Minimum radius of curvature = " + str(min_radius) + "mm");

          if (is_higher_order) {
            max_angle_ho = max_angle_1o;
            cos_max_angle_ho = cos_max_angle_1o;
            // Clear these variables so that the next() function of the underlying method
            //   does not enforce curvature constraints; rely on e.g. RK4 to do it
            max_angle_1o = float(Math::pi);
            cos_max_angle_1o = 0.0f;
          }

        }




        void SharedBase::set_num_points()
        {
          // Angle around the circle of minimum radius for the given step size
          const float angle_minradius_preds = 2.0f * std::asin (step_size / (2.0f * min_radius));
          // Maximum inter-vertex distance after streamline has been downsampled
          const float max_step_postds = downsampler.get_ratio() * step_size;

          set_num_points (angle_minradius_preds, max_step_postds);
        }



        void SharedBase::set_num_points (const float angle_minradius_preds,
                                         const float max_step_postds)
        {
          // Maximal angle around this minimum radius traversed after downsampling
          const float angle_minradius_postds = downsampler.get_ratio() * angle_minradius_preds;
          // Minimum chord length after streamline has been downsampled
          const float min_step_postds = (angle_minradius_postds > float(2.0 * Math::pi)) ?
                                        0.0f :
                                        (2.0f * min_radius * std::sin (0.5f * angle_minradius_postds));

          // What we need:
          //   - Before downsampling:
          //     - How many points must be generated in order for it to be feasible that the
          //         streamline may exceed the minimum length after downsampling?
          //       (If a streamline doesn't reach this number of vertices, there's no point in
          //         even attempting any further processing of it; it will always be rejected)
          min_num_points_preds = 1 + std::ceil (min_dist / step_size);
          //     - How many points before it is no longer feasible to become shorter than the
          //         maximum length, even after down-sampling?
          //       (There is no point in continuing streamlines propagation after this point;
          //         it will invariably be either truncated or rejected, no matter what
          //         happens during downsampling)
          max_num_points_preds = min_step_postds ?
                                 (3 + std::ceil (downsampler.get_ratio() * max_dist / min_step_postds)) :
                                 std::numeric_limits<size_t>::max();
          //   - After downsampling:
          //     - How many vertices must a streamline have (after downsampling) for it to be
          //         guaranteed to exceed the minimum length?
          //       (If a streamline has less than this number of vertices after downsampling, we
          //         need to quantify its length precisely and compare against the minimum)
          min_num_points_postds = 3 + std::ceil (min_dist / min_step_postds);
          //     - How many vertices can a streamline have (after downsampling) for it to be
          //         guaranteed to be shorter than the maximum length?
          //       (If a streamline has more than this number of vertices after downsampling, we
          //         need to quantify its length precisely and compare against the maximum)
          max_num_points_postds = 1 + std::floor (max_dist / max_step_postds);

          DEBUG ("For tracking step size " + str(step_size) + "mm, " +
                 (std::isfinite (max_angle_ho) ?
                     ("max change in fibre orientation angle per step " + str(max_angle_ho * 180.0 / Math::pi, 6) + " deg (using RK4)") :
                     ("max angle deviation per step " + str(max_angle_1o * 180.0 / Math::pi, 6) + "deg")) +
                 ", minimum radius of curvature " + str(min_radius, 6) + "mm, downsampling ratio " + str(downsampler.get_ratio()) + ": " +
                 "minimum length of " + str(min_dist) + "mm requires at least " + str(min_num_points_preds) + " vertices pre-DS, is tested explicitly for " + str(min_num_points_postds) + " vertices or less post-DS; " +
                 "maximum length of " + str(max_dist) + "mm will stop tracking after " + str(max_num_points_preds) + " vertices pre-DS, is tested explicitly for " + str(max_num_points_postds) + " or more vertices post-DS");
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

