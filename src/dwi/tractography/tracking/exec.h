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

#ifndef __dwi_tractography_tracking_exec_h__
#define __dwi_tractography_tracking_exec_h__


#include "thread.h"
#include "thread_queue.h"
#include "dwi/directions/set.h"
#include "dwi/tractography/streamline.h"
#include "dwi/tractography/rng.h"
#include "dwi/tractography/roi.h"
#include "dwi/tractography/tracking/generated_track.h"
#include "dwi/tractography/tracking/method.h"
#include "dwi/tractography/tracking/shared.h"
#include "dwi/tractography/tracking/write_kernel.h"

#include "dwi/tractography/mapping/mapper.h"
#include "dwi/tractography/mapping/mapping.h"
#include "dwi/tractography/mapping/voxel.h"

#include "dwi/tractography/seeding/dynamic.h"


#define MAX_NUM_SEED_ATTEMPTS 100000

#define TRACKING_BATCH_SIZE 10



namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {
      namespace Tracking
      {


        // TODO Try having ACT as a template boolean; allow compiler to optimise out branch statements

        template <class Method> class Exec { MEMALIGN(Exec<Method>)

          public:

            static void run (const std::string& diff_path, const std::string& destination, DWI::Tractography::Properties& properties)
            {

              if (properties.find ("seed_dynamic") == properties.end()) {

                typename Method::Shared shared (diff_path, properties);
                WriteKernel writer (shared, destination, properties);
                Exec<Method> tracker (shared);
                Thread::run_queue (Thread::multi (tracker), Thread::batch (GeneratedTrack(), TRACKING_BATCH_SIZE), writer);

              } else {

                const std::string& fod_path (properties["seed_dynamic"]);
                const std::string max_num_tracks = properties["max_num_tracks"];
                if (max_num_tracks.empty())
                  throw Exception ("Dynamic seeding requires setting the desired number of tracks using the -select option");
                const size_t num_tracks = to<size_t>(max_num_tracks);

                using SetDixel = Mapping::SetDixel;
                using TckMapper = Mapping::TrackMapperBase;
                using Writer = Seeding::WriteKernelDynamic;

                DWI::Directions::FastLookupSet dirs (1281);
                auto fod_data = Image<float>::open (fod_path);
                Math::SH::check (fod_data);
                Seeding::Dynamic* seeder = new Seeding::Dynamic (fod_path, fod_data, num_tracks, dirs);
                properties.seeds.add (seeder); // List is responsible for deleting this from memory

                typename Method::Shared shared (diff_path, properties);

                Writer       writer  (shared, destination, properties);
                Exec<Method> tracker (shared);

                TckMapper mapper (fod_data, dirs);
                mapper.set_upsample_ratio (Mapping::determine_upsample_ratio (fod_data, properties, 0.25));
                mapper.set_use_precise_mapping (true);

                Thread::run_queue (
                    Thread::multi (tracker),
                    Thread::batch (GeneratedTrack(), TRACKING_BATCH_SIZE),
                    writer,
                    Thread::batch (Streamline<>(), TRACKING_BATCH_SIZE),
                    Thread::multi (mapper),
                    Thread::batch (SetDixel(), TRACKING_BATCH_SIZE),
                    *seeder);

              }

            }



            Exec (const typename Method::Shared& shared) :
              S (shared),
              method (shared),
              track_excluded (false),
              include_visitation (S.properties.include, S.properties.ordered_include) { }

            Exec (const Exec& that) :
              S (that.S),
              method (that.method),
              track_excluded (false),
              include_visitation (S.properties.include, S.properties.ordered_include) { }


            bool operator() (GeneratedTrack& item) {
              if (!seed_track (item))
                return false;
              if (track_excluded) {
                item.set_status (GeneratedTrack::status_t::SEED_REJECTED);
                S.add_rejection (INVALID_SEED);
                return true;
              }
              gen_track (item);
              if (track_rejected (item)) {
                item.clear();
                item.set_status (GeneratedTrack::status_t::TRACK_REJECTED);
              } else if (S.downsampler.get_ratio() > 1 || (S.is_act() && S.act().crop_at_gmwmi())) {
                S.downsampler (item);
                check_downsampled_length (item);
              } else {
                item.set_status (GeneratedTrack::status_t::ACCEPTED);
              }
              return true;
            }


          private:

            const typename Method::Shared& S;
            Method method;
            bool track_excluded;
            IncludeROIVisitation include_visitation;


            term_t iterate ()
            {
              const term_t method_term = (S.rk4 ? next_rk4() : method.next());

              if (method_term)
                return (S.is_act() && method.act().sgm_depth) ? TERM_IN_SGM : method_term;

              if (S.is_act()) {
                const term_t structural_term = method.act().check_structural (method.pos);
                if (structural_term)
                  return structural_term;
              }

              if (S.properties.mask.size() && !S.properties.mask.contains (method.pos))
                return EXIT_MASK;

              if (S.properties.exclude.contains (method.pos))
                return ENTER_EXCLUDE;

              // If backtracking is not enabled, add streamline to include regions as it is generated
              // If it is enabled, this check can only be performed after the streamline is completed
              if (!(S.is_act() && S.act().backtrack()))
                include_visitation (method.pos);

              if (S.stop_on_all_include && bool(include_visitation))
                return TRAVERSE_ALL_INCLUDE;

              return CONTINUE;
            }



            bool seed_track (GeneratedTrack& tck)
            {
              tck.clear();
              track_excluded = false;
              include_visitation.reset();
              method.dir = { NaN, NaN, NaN };

              if (S.properties.seeds.is_finite()) {

                if (!S.properties.seeds.get_seed (method.pos, method.dir))
                  return false;
                if (!method.check_seed() || !method.init()) {
                  track_excluded = true;
                  tck.set_status (GeneratedTrack::status_t::SEED_REJECTED);
                }
                return true;

              } else {

                for (size_t num_attempts = 0; num_attempts != MAX_NUM_SEED_ATTEMPTS; ++num_attempts) {
                  if (S.properties.seeds.get_seed (method.pos, method.dir)) {
                    if (!(method.check_seed() && method.init())) {
                      track_excluded = true;
                      tck.set_status (GeneratedTrack::status_t::SEED_REJECTED);
                    }
                    return true;
                  }
                }
                FAIL ("Failed to find suitable seed point after " + str (MAX_NUM_SEED_ATTEMPTS) + " attempts - aborting");
                return false;

              }
            }



            bool gen_track (GeneratedTrack& tck)
            {
              bool unidirectional = S.unidirectional;
              if (S.is_act() && !unidirectional)
                unidirectional = method.act().seed_is_unidirectional (method.pos, method.dir);

              include_visitation (method.pos);

              const Eigen::Vector3f seed_dir (method.dir);
              tck.push_back (method.pos);

              gen_track_unidir (tck);

              if (!track_excluded && !unidirectional) {
                tck.reverse();
                method.pos = tck.back();
                method.dir = -seed_dir;
                method.reverse_track ();
                gen_track_unidir (tck);
              }

              return true;
            }




            void gen_track_unidir (GeneratedTrack& tck)
            {
              term_t termination = CONTINUE;

              if (S.is_act() && S.act().backtrack()) {

                size_t revert_step = 1;
                size_t max_size_at_backtrack = tck.size();
                unsigned int revert_count = 0;

                do {
                  termination = iterate();
                  if (term_add_to_tck[termination])
                    tck.push_back (method.pos);
                  if (termination) {
                    apply_priors (termination);
                    if (track_excluded && termination != ENTER_EXCLUDE) {
                      if (tck.size() > max_size_at_backtrack) {
                        max_size_at_backtrack = tck.size();
                        revert_step = 1;
                        revert_count = 1;
                      } else {
                        if (revert_count++ == ACT_BACKTRACK_ATTEMPTS) {
                          revert_count = 1;
                          ++revert_step;
                        }
                      }
                      method.truncate_track (tck, max_size_at_backtrack, revert_step);
                      if (method.pos.allFinite()) {
                        track_excluded = false;
                        termination = CONTINUE;
                      }
                    }
                  } else if (tck.size() >= S.max_num_points_preds) {
                    termination = LENGTH_EXCEED;
                  }
                } while (!termination);

              } else {

                do {
                  termination = iterate();
                  if (term_add_to_tck[termination])
                    tck.push_back (method.pos);
                  if (!termination && tck.size() >= S.max_num_points_preds)
                    termination = LENGTH_EXCEED;
                } while (!termination);

              }

              apply_priors (termination);

              if (termination == EXIT_SGM) {
                truncate_exit_sgm (tck);
                method.pos = tck.back();
              }

              if (track_excluded) {
                switch (termination) {
                  case CALIBRATOR: case ENTER_CSF: case MODEL: case HIGH_CURVATURE:
                    S.add_rejection (ACT_POOR_TERMINATION);
                    break;
                  case LENGTH_EXCEED:
                    S.add_rejection (TRACK_TOO_LONG);
                    break;
                  case ENTER_EXCLUDE:
                    S.add_rejection (ENTER_EXCLUDE_REGION);
                    break;
                  default:
                    throw Exception ("\nFIXME: Unidirectional track excluded but termination is good!\n");
                }
              }

              if (S.is_act() && (termination == ENTER_CGM) && S.act().crop_at_gmwmi())
                S.act().crop_at_gmwmi (tck);

#ifdef DEBUG_TERMINATIONS
              S.add_termination (termination, method.pos);
#else
              S.add_termination (termination);
#endif

            }



            void apply_priors (term_t& termination)
            {

              if (S.is_act()) {

                switch (termination) {

                  case CONTINUE:
                    throw Exception ("\nFIXME: undefined termination of track in apply_priors()\n");

                  case ENTER_CGM: case EXIT_IMAGE: case EXIT_MASK: case EXIT_SGM: case TERM_IN_SGM: case TRAVERSE_ALL_INCLUDE:
                    break;

                  case ENTER_CSF: case LENGTH_EXCEED: case ENTER_EXCLUDE:
                    track_excluded = true;
                    break;

                  case CALIBRATOR: case MODEL: case HIGH_CURVATURE:
                    if (method.act().sgm_depth)
                      termination = TERM_IN_SGM;
                    else if (!method.act().in_pathology())
                      track_excluded = true;
                    break;

                }

              } else {

                switch (termination) {

                  case CONTINUE:
                    throw Exception ("\nFIXME: undefined termination of track in apply_priors()\n");

                  case ENTER_CGM: case ENTER_CSF: case EXIT_SGM: case TERM_IN_SGM:
                    throw Exception ("\nFIXME: Have received ACT-based termination for non-ACT tracking in apply_priors()\n");

                  case EXIT_IMAGE: case EXIT_MASK: case LENGTH_EXCEED: case CALIBRATOR: case MODEL: case HIGH_CURVATURE: case TRAVERSE_ALL_INCLUDE:
                    break;

                  case ENTER_EXCLUDE:
                    track_excluded = true;
                    break;

                }

              }

            }



            bool track_rejected (const GeneratedTrack& tck)
            {

              if (track_excluded)
                return true;

              // seedtest algorithm uses min_num_points_preds = 1; should be 2 or more for all other algorithms
              if (tck.size() == 1 && S.min_num_points_preds > 1) {
                S.add_rejection (NO_PROPAGATION_FROM_SEED);
                return true;
              }

              if (tck.size() < S.min_num_points_preds) {
                S.add_rejection (TRACK_TOO_SHORT);
                return true;
              }

              if (S.is_act()) {

                if (!satisfy_wm_requirement (tck)) {
                  S.add_rejection (ACT_FAILED_WM_REQUIREMENT);
                  return true;
                }

                if (S.act().backtrack()) {
                  for (const auto& i : tck)
                    include_visitation (i);
                }

              }

              if (!bool(include_visitation)) {
                S.add_rejection (MISSED_INCLUDE_REGION);
                return true;
              }

              return false;

            }

            bool satisfy_wm_requirement (const vector<Eigen::Vector3f>& tck)
            {
              // If using the Seed_test algorithm (indicated by max_num_points == 2), don't want to execute this check
              if (S.max_num_points_preds == 2)
                return true;
              // If the seed was in SGM, need to confirm that one side of the track actually made it to WM
              if (method.act().seed_in_sgm && !method.act().sgm_seed_to_wm)
                return false;
              // Used these in the ACT paper, but wasn't entirely happy with the method; can change these #defines to re-enable
              // ACT instead now defaults to a 2-voxel minimum length
              if (!ACT_WM_INT_REQ && !ACT_WM_ABS_REQ)
                return true;
              float integral = 0.0, max_value = 0.0;
              for (const auto& i : tck) {
                if (method.act().fetch_tissue_data (i)) {
                  const float wm = method.act().tissues().get_wm();
                  max_value = std::max (max_value, wm);
                  if (((integral += (Math::pow2 (wm) * S.internal_step_size())) > ACT_WM_INT_REQ) && (max_value > ACT_WM_ABS_REQ))
                    return true;
                }
              }
              return false;
            }



            void truncate_exit_sgm (vector<Eigen::Vector3f>& tck)
            {
              const size_t sgm_start = tck.size() - method.act().sgm_depth;
              assert (sgm_start >= 0 && sgm_start < tck.size());
              size_t best_termination = tck.size() - 1;
              float min_value = std::numeric_limits<float>::infinity();
              for (size_t i = sgm_start; i != tck.size(); ++i) {
                const Eigen::Vector3f direction = (tck[i] - tck[i-1]).normalized();
                const float this_value = method.get_metric (tck[i], direction);
                if (this_value < min_value) {
                  min_value = this_value;
                  best_termination = i;
                }
              }
              tck.erase (tck.begin() + best_termination + 1, tck.end());
            }



            void check_downsampled_length (GeneratedTrack& tck)
            {
              // Don't quantify the precise streamline length if we don't have to
              //   (i.e. we know for sure that even in the presence of downsampling,
              //   we're not going to break either of these two criteria)
              if (tck.size() > S.min_num_points_postds && tck.size() < S.max_num_points_postds) {
                tck.set_status (GeneratedTrack::status_t::ACCEPTED);
                return;
              }
              const float length = Tractography::length (tck);
              if (length < S.min_dist) {
                tck.clear();
                tck.set_status (GeneratedTrack::status_t::TRACK_REJECTED);
                S.add_rejection (TRACK_TOO_SHORT);
              } else if (length > S.max_dist) {
                if (S.is_act()) {
                  tck.clear();
                  tck.set_status (GeneratedTrack::status_t::TRACK_REJECTED);
                  S.add_rejection (TRACK_TOO_LONG);
                } else {
                  truncate_maxlength (tck);
                  tck.set_status (GeneratedTrack::status_t::ACCEPTED);
                }
              } else {
                tck.set_status (GeneratedTrack::status_t::ACCEPTED);
              }
            }



            void truncate_maxlength (GeneratedTrack& tck)
            {
              // Chop the track short so that the length is precisely the maximum length
              // If the truncation would result in removing the seed point, truncate all
              //   the way back to the seed point, reverse the streamline, and then
              //   truncate off the end (which used to be the start)
              float length_sum = 0.0f;
              size_t index;
              for (index = 1; index != tck.size(); ++index) {
                const float seg_length = (tck[index] - tck[index-1]).norm();
                if (length_sum + seg_length > S.max_dist)
                  break;
                length_sum += seg_length;
              }

              // If we don't exceed the maximum length, this function should never have been called!
              assert (index != tck.size());
              // But nevertheless; if we happen to somehow get here, let's just allow processing to continue
              if (index == tck.size())
                return;

              // Would truncation at this vertex (plus including a new small segment at the end) result in
              //   discarding the seed point? If so:
              //   - Truncate so that the seed point is the last point on the streamline
              //   - Reverse the order of the vertices
              //   - Re-run this function recursively in order to truncate from the opposite end of the streamline
              if (tck.get_seed_index() >= index) {
                tck.resize (tck.get_seed_index()+1);
                tck.reverse();
                truncate_maxlength (tck);
                return;
              }

              // We want to determine a new vertex in between "index" and the prior vertex, which
              //   is of the appropriate distance away from "index" in order to make the streamline
              //   precisely the maximum length
              tck.resize (index+1);
              tck[index] = tck[index-1] + ((tck[index] - tck[index-1]).normalized() * (S.max_dist - length_sum));
            }



            term_t next_rk4()
            {
              term_t termination = CONTINUE;
              const Eigen::Vector3f init_pos (method.pos);
              const Eigen::Vector3f init_dir (method.dir);
              if ((termination = method.next()))
                return termination;
              const Eigen::Vector3f dir_rk1 (method.dir);
              method.pos = init_pos + (dir_rk1 * (0.5 * S.step_size));
              method.dir = init_dir;
              if ((termination = method.next()))
                return termination;
              const Eigen::Vector3f dir_rk2 (method.dir);
              method.pos = init_pos + (dir_rk2 * (0.5 * S.step_size));
              method.dir = init_dir;
              if ((termination = method.next()))
                return termination;
              const Eigen::Vector3f dir_rk3 (method.dir);
              method.pos = init_pos + (dir_rk3 * S.step_size);
              method.dir = (dir_rk2 + dir_rk3).normalized();
              if ((termination = method.next()))
                return termination;
              const Eigen::Vector3f dir_rk4 (method.dir);
              method.dir = (dir_rk1 + (dir_rk2 * 2.0) + (dir_rk3 * 2.0) + dir_rk4).normalized();
              method.pos = init_pos + (method.dir * S.step_size);
              const Eigen::Vector3f final_pos (method.pos);
              const Eigen::Vector3f final_dir (method.dir);
              if ((termination = method.next()))
                return termination;
              if (dir_rk1.dot (method.dir) < S.cos_max_angle_ho)
                return HIGH_CURVATURE;
              method.pos = final_pos;
              method.dir = final_dir;
              return CONTINUE;
            }



        };









      }
    }
  }
}

#endif


