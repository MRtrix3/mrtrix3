/* Copyright (c) 2008-2017 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
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


#define TCKGEN_HIGHLY_VERBOSE



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
              track_included (S.properties.include.size(), false) { }


            bool operator() (GeneratedTrack& item) {
              rng = &thread_local_RNG;
              if (!seed_track (item))
                return false;
              if (item.get_status() == GeneratedTrack::status_t::SEED_REJECTED) {
                S.add_rejection (INVALID_SEED);
#ifdef TCKGEN_HIGHLY_VERBOSE
                std::cerr << "Seed rejected\n\n\n";
#endif
                return true;
              }
              gen_track (item);
              if (verify_track (item)) {
                S.downsampler (item);
                item.set_status (GeneratedTrack::status_t::ACCEPTED);
#ifdef TCKGEN_HIGHLY_VERBOSE
                std::cerr << "Track accepted\n\n\n";
              } else {
                std::cerr << "Track rejected\n\n\n";
#endif
              }
              return true;
            }


          private:

            const typename Method::Shared& S;
            Math::RNG thread_local_RNG;
            Method method;
            vector<bool> track_included;


            term_t iterate ()
            {

              const term_t method_term = (S.rk4 ? next_rk4() : method.next());

              if (method_term) {
                if (S.is_act() && method.act().sgm_depth) {
#ifdef TCKGEN_HIGHLY_VERBOSE
                  std::cerr << "iterate() terminated by method() in SGM due to: " << termination_descriptions[method_term] << "\n";
#endif
                  return TERM_IN_SGM;
                } else {
#ifdef TCKGEN_HIGHLY_VERBOSE
                  std::cerr << "iterate() terminated by method() due to: " << termination_descriptions[method_term] << "\n";
                  if (method_term == EXIT_IMAGE)
                    std::cerr << "Break here\n";
#endif
                  return method_term;
                }
              }

              if (S.is_act()) {
                const term_t structural_term = method.act().check_structural (method.pos);
                if (structural_term) {
#ifdef TCKGEN_HIGHLY_VERBOSE
                  std::cerr << "iterate() terminated by ACT due to: " << termination_descriptions[structural_term] << "\n";
#endif
                  return structural_term;
                }
              }

              if (S.properties.mask.size() && !S.properties.mask.contains (method.pos)) {
#ifdef TCKGEN_HIGHLY_VERBOSE
                std::cerr << "iterate() terminated due to exiting mask\n";
#endif
                return EXIT_MASK;
              }

              if (S.properties.exclude.contains (method.pos)) {
#ifdef TCKGEN_HIGHLY_VERBOSE
                std::cerr << "iterate() terminated due to entering an exclude region\n";
#endif
                return ENTER_EXCLUDE;
              }

              // If backtracking is not enabled, add streamline to include regions as it is generated
              // If it is enabled, this check can only be performed after the streamline is completed
              if (!(S.is_act() && S.act().backtrack()))
                S.properties.include.contains (method.pos, track_included);

              if (S.stop_on_all_include && traversed_all_include_regions()) {
#ifdef TCKGEN_HIGHLY_VERBOSE
                std::cerr << "iterate() terminated due to having traversed all include regions\n";
#endif
                return TRAVERSE_ALL_INCLUDE;
              }

              return CONTINUE;

            }



            bool seed_track (GeneratedTrack& tck)
            {
              tck.clear();
              track_included.assign (track_included.size(), false);
              method.dir = { NaN, NaN, NaN };

              if (S.properties.seeds.is_finite()) {

                if (!S.properties.seeds.get_seed (method.pos, method.dir))
                  return false;
                if (!method.check_seed() || !method.init())
                  tck.set_status (GeneratedTrack::status_t::SEED_REJECTED);
                return true;

              } else {

                for (size_t num_attempts = 0; num_attempts != MAX_NUM_SEED_ATTEMPTS; ++num_attempts) {
                  if (S.properties.seeds.get_seed (method.pos, method.dir)) {
                    if (!(method.check_seed() && method.init()))
                      tck.set_status (GeneratedTrack::status_t::SEED_REJECTED);
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

              S.properties.include.contains (method.pos, track_included);

              const Eigen::Vector3f seed_dir (method.dir);
              tck.push_back (method.pos);

              gen_track_unidir (tck);

              if (tck.get_status() != GeneratedTrack::status_t::TRACK_REJECTED && !unidirectional) {
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

                // This is the information that the recursive back-tracking functor will return
                struct TrackAndTermination
                {
                  GeneratedTrack tck;
                  term_t termination;
                };

                std::function<TrackAndTermination(GeneratedTrack&, size_t, size_t)>
                backtrack_functor = [&] (GeneratedTrack& tck, size_t target_length, size_t recursion_depth) -> TrackAndTermination
                {

#ifdef TCKGEN_HIGHLY_VERBOSE
                    std::cerr << "(" << recursion_depth << ", " << tck.size() << "): Commencing at this recursion depth\n";
#endif


                  if (recursion_depth > (tck.size() - tck.get_seed_index())) {
#ifdef TCKGEN_HIGHLY_VERBOSE
                    std::cerr << "(" << recursion_depth << ", " << tck.size() << "): Excessive recursion depth\n";
#endif
                    return TrackAndTermination { tck, CONTINUE };
                  }

                  const size_t initial_tck_length = tck.size();
                  term_t functor_termination = CONTINUE;

                  do {
                    functor_termination = iterate();
                    if (term_add_to_tck[functor_termination])
                      tck.push_back (method.pos);
                    if (functor_termination) {
                      apply_priors (tck, functor_termination);
                      if (tck.get_status() == GeneratedTrack::status_t::TRACK_REJECTED && functor_termination != ENTER_EXCLUDE) {

                        if (tck.get_seed_index() == tck.size() - 1) {
#ifdef TCKGEN_HIGHLY_VERBOSE
                          std::cerr << "(" << recursion_depth << ", " << tck.size() << "): Truncated all the way to seed point; aborting\n";
#endif
                          return TrackAndTermination { tck, functor_termination };
                        }

#ifdef TCKGEN_HIGHLY_VERBOSE
                        std::cerr << "(" << recursion_depth << ", " << tck.size() << "): Exclusionary termination detected\n";
#endif

                        // Here is where we want to engage back-tracking
                        // Both loops here:
                        //   - Some number of back-tracking attempts per truncation length
                        //   - Increasing truncation length
                        size_t revert_step = 1;
                        do {
                          for (size_t attempt_counter = 0; attempt_counter < ACT_BACKTRACK_ATTEMPTS; ++attempt_counter) {
                            // While this truncation could be performed outside of the attempt_counter loop,
                            //   truncate_track() may be responsible for restoring different state variables
                            //   that are specific to the method
                            GeneratedTrack truncated_tck (tck);

#ifdef TCKGEN_HIGHLY_VERBOSE
                            std::cerr << "(" << recursion_depth << ", " << tck.size() << "): With revert_step " << revert_step << ", attempt " << attempt_counter << ", track truncated from " << tck.size() << " to ";
#endif

                            method.truncate_track (truncated_tck, tck.size(), revert_step); // TODO Remove maximum truncation length
                            truncated_tck.set_status (GeneratedTrack::status_t::UNDEFINED);

#ifdef TCKGEN_HIGHLY_VERBOSE
                            std::cerr << truncated_tck.size() << "\n";
#endif

                            // Only restrict length of truncation if we have failed to exceed the length of the streamline that invoked us
                            if ((tck.size() <= target_length && truncated_tck.size() <= initial_tck_length) || !method.pos.allFinite()) {

#ifdef TCKGEN_HIGHLY_VERBOSE
                              std::cerr << "(" << recursion_depth << ", " << tck.size() << "): Cannot truncate any further due to original track length (" << initial_tck_length << ") at this depth\n";
#endif

                              return TrackAndTermination { tck, functor_termination };
                            }

#ifdef TCKGEN_HIGHLY_VERBOSE
                            std::cerr << "(" << recursion_depth << ", " << tck.size() << "): Running attempt " << attempt_counter << " at revert_step " << revert_step << "\n";
#endif

                            TrackAndTermination result = backtrack_functor (truncated_tck, tck.size(), recursion_depth+1);
                            // Return value will never have termination == CONTINUE;
                            //   therefore can just check the relative lengths
                            if (result.tck.size() > tck.size()) {
#ifdef TCKGEN_HIGHLY_VERBOSE
                              std::cerr << "(" << recursion_depth << ", " << tck.size() << "): Attempt " << attempt_counter << " at revert_step " << revert_step << " yielded track length " << result.tck.size() << ", which is longer than original (" << tck.size() << "); returning\n";
#endif
                              return result;
                            }
                          }
                          ++revert_step;
                          // If truncation can no longer be performed successfully, that's our flag that there's
                          //   no point in further increasing the truncation length
                        } while (method.pos.allFinite());

#ifdef TCKGEN_HIGHLY_VERBOSE
                        std::cerr << "(" << recursion_depth << ", " << tck.size() << "): Unable to successfully backtrack; exiting with track length " << tck.size() << ", termination: " << termination_descriptions[functor_termination] << "\n";
#endif

                        // We have been unable to successfully generate a longer streamline at any truncation length
                        // This is therefore our final result of tracking
                        return TrackAndTermination { tck, functor_termination };

#ifdef TCKGEN_HIGHLY_VERBOSE
                      } else {
                        std::cerr << "(" << recursion_depth << ", " << tck.size() << "): Streamline terminated due to: " << termination_descriptions[functor_termination] << "\n";
#endif
                      }
                    } else if (tck.size() >= S.max_num_points) {
#ifdef TCKGEN_HIGHLY_VERBOSE
                      std::cerr << "(" << recursion_depth << ", " << tck.size() << "): Track terminated due to reaching maximum length\n";
#endif
                      return TrackAndTermination { tck, LENGTH_EXCEED };
                    }
#ifdef TCKGEN_HIGHLY_VERBOSE
                  if (functor_termination)
                    std::cerr << "(" << recursion_depth << ", " << tck.size() << "): NOT iterating again; termination = " << termination_descriptions[functor_termination] << "\n";
#endif
                  } while (!functor_termination);

#ifdef TCKGEN_HIGHLY_VERBOSE
                  std::cerr << "(" << recursion_depth << ", " << tck.size() << "): Returning at end of functor\n";
#endif

                  return TrackAndTermination { tck, functor_termination };

                };

                auto result = backtrack_functor (tck, 0, 0);
                //std::swap (tck, result.tck);
                tck = result.tck;
                termination = result.termination;

/*
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
#ifdef TCKGEN_HIGHLY_VERBOSE
                        std::cerr << "Have not yet back-tracked from track length " << tck.size() << "; re-initializing\n";
#endif
                        max_size_at_backtrack = tck.size();
                        revert_step = 1;
                        revert_count = 1;
                      } else {
                        if (revert_count++ == ACT_BACKTRACK_ATTEMPTS) {
                          revert_count = 1;
                          ++revert_step;
#ifdef TCKGEN_HIGHLY_VERBOSE
                          std::cerr << "Hit max backtrack attempts; increasing backtrack length to " << revert_step << "\n";
                        } else {
                          std::cerr << "Continuing to attempt backtrack at length " << revert_step << "\n";
#endif
                        }
                      }
#ifdef TCKGEN_HIGHLY_VERBOSE
                      std::cerr << "Back-tracking from length " << tck.size() << ", max length " << max_size_at_backtrack << ", revert step " << revert_step << "\n";
#endif
                      method.truncate_track (tck, max_size_at_backtrack, revert_step);
                      if (method.pos.allFinite()) {
                        track_excluded = false;
                        termination = CONTINUE;
#ifdef TCKGEN_HIGHLY_VERBOSE
                        std::cerr << "Re-attempting tracking, track length " << tck.size() << "\n";
                      } else {
                        std::cerr << "method.pos is invalid; track to be abandoned\n";
#endif
                      }
                    }
                  } else if (tck.size() >= S.max_num_points) {
#ifdef TCKGEN_HIGHLY_VERBOSE
                    std::cerr << "Track terminated due to reaching maximum length\n";
#endif
                    termination = LENGTH_EXCEED;
                  }
                } while (!termination);
*/
              } else {

                do {
                  termination = iterate();
                  if (term_add_to_tck[termination])
                    tck.push_back (method.pos);
                  if (!termination && tck.size() >= S.max_num_points) {
#ifdef TCKGEN_HIGHLY_VERBOSE
                    std::cerr << "Track terminated due to reaching maximum length\n";
#endif
                    termination = LENGTH_EXCEED;
                  }
                } while (!termination);

              }

              apply_priors (tck, termination);

              if (termination == EXIT_SGM) {
#ifdef TCKGEN_HIGHLY_VERBOSE
                std::cerr << "Truncating track due to exiting SGM; length from " << tck.size() << " to ";
#endif
                truncate_exit_sgm (tck);
#ifdef TCKGEN_HIGHLY_VERBOSE
                std::cerr << tck.size() << "\n";
#endif
                method.pos = tck.back();
              }

              if (tck.get_status() == GeneratedTrack::status_t::TRACK_REJECTED) {
                switch (termination) {
                  case CALIBRATOR: case ENTER_CSF: case BAD_SIGNAL: case HIGH_CURVATURE:
                    S.add_rejection (ACT_POOR_TERMINATION);
                    break;
                  case LENGTH_EXCEED:
                    S.add_rejection (TRACK_TOO_LONG);
                    break;
                  case ENTER_EXCLUDE:
                    S.add_rejection (ENTER_EXCLUDE_REGION);
                    break;
                  default:
                    VAR (termination);
                    throw Exception ("\nFIXME: Unidirectional track rejected but termination is good!\n");
                }
              }

              if (S.is_act() && (termination == ENTER_CGM) && S.act().crop_at_gmwmi()) {
#ifdef TCKGEN_HIGHLY_VERBOSE
                std::cerr << "Cropping track at GM-WM Interface\n";
#endif
                S.act().crop_at_gmwmi (tck);
              }

#ifdef DEBUG_TERMINATIONS
              S.add_termination (termination, method.pos);
#else
              S.add_termination (termination);
#endif

            }



            void apply_priors (GeneratedTrack& tck, term_t& termination)
            {

              if (S.is_act()) {

                switch (termination) {

                  case CONTINUE:
                    throw Exception ("\nFIXME: undefined termination of track in apply_priors()\n");

                  case ENTER_CGM: case EXIT_IMAGE: case EXIT_MASK: case EXIT_SGM: case TERM_IN_SGM: case TRAVERSE_ALL_INCLUDE:
                    break;

                  case ENTER_CSF: case LENGTH_EXCEED: case ENTER_EXCLUDE:
                    tck.set_status (GeneratedTrack::status_t::TRACK_REJECTED);
                    break;

                  case CALIBRATOR: case BAD_SIGNAL: case HIGH_CURVATURE:
                    if (method.act().sgm_depth) {
                      termination = TERM_IN_SGM;
                      tck.set_status (GeneratedTrack::status_t::UNDEFINED);
                    } else if (!method.act().in_pathology())
                      tck.set_status (GeneratedTrack::status_t::TRACK_REJECTED);
                    break;

                }

              } else {

                switch (termination) {

                  case CONTINUE:
                    throw Exception ("\nFIXME: undefined termination of track in apply_priors()\n");

                  case ENTER_CGM: case ENTER_CSF: case EXIT_SGM: case TERM_IN_SGM:
                    throw Exception ("\nFIXME: Have received ACT-based termination for non-ACT tracking in apply_priors()\n");

                  case EXIT_IMAGE: case EXIT_MASK: case LENGTH_EXCEED: case CALIBRATOR: case BAD_SIGNAL: case HIGH_CURVATURE: case TRAVERSE_ALL_INCLUDE:
                    break;

                  case ENTER_EXCLUDE:
                    tck.set_status (GeneratedTrack::status_t::TRACK_REJECTED);
                    break;

                }

              }

            }



            bool verify_track (GeneratedTrack& tck)
            {

              if (tck.get_status() == GeneratedTrack::status_t::TRACK_REJECTED)
                return false;

              // seedtest algorithm uses min_num_points = 1; should be 2 or more for all other algorithms
              if (tck.size() == 1 && S.min_num_points > 1) {
#ifdef TCKGEN_HIGHLY_VERBOSE
                std::cerr << "Track rejected due to failure to propagate from seed\n";
#endif
                tck.set_status (GeneratedTrack::status_t::TRACK_REJECTED);
                S.add_rejection (NO_PROPAGATION_FROM_SEED);
                return false;
              }

              if (tck.size() < S.min_num_points) {
#ifdef TCKGEN_HIGHLY_VERBOSE
                std::cerr << "Track rejected due to minimum length criterion\n";
#endif
                tck.set_status (GeneratedTrack::status_t::TRACK_REJECTED);
                S.add_rejection (TRACK_TOO_SHORT);
                return false;
              }

              if (S.is_act()) {

                if (!satisfy_wm_requirement (tck)) {
#ifdef TCKGEN_HIGHLY_VERBOSE
                  std::cerr << "Track rejected due to WM requirement\n";
#endif
                  tck.set_status (GeneratedTrack::status_t::TRACK_REJECTED);
                  S.add_rejection (ACT_FAILED_WM_REQUIREMENT);
                  return false;
                }

                if (S.act().backtrack()) {
                  for (const auto& i : tck)
                    S.properties.include.contains (i, track_included);
                }

              }

              if (!traversed_all_include_regions()) {
#ifdef TCKGEN_HIGHLY_VERBOSE
                std::cerr << "Track rejected due to failure to traverse all include regions\n";
#endif
                tck.set_status (GeneratedTrack::status_t::TRACK_REJECTED);
                S.add_rejection (MISSED_INCLUDE_REGION);
                return false;
              }

              return true;

            }



            bool traversed_all_include_regions ()
            {
              for (size_t n = 0; n < track_included.size(); ++n)
                if (!track_included[n])
                  return false;
              return true;
            }



            bool satisfy_wm_requirement (const vector<Eigen::Vector3f>& tck)
            {
              // If using the Seed_test algorithm (indicated by max_num_points == 2), don't want to execute this check
              if (S.max_num_points == 2)
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

              Interpolator<Image<float>>::type source (S.source);

              const size_t sgm_start = tck.size() - method.act().sgm_depth;
              assert (sgm_start >= 0 && sgm_start < tck.size());
              size_t best_termination = tck.size() - 1;
              float min_value = INFINITY;
              for (size_t i = sgm_start; i != tck.size(); ++i) {
                method.pos = tck[i];
                method.get_data (source);
                method.dir = (tck[i] - tck[i-1]).normalized();
                const float this_value = method.get_metric();
                if (this_value < min_value) {
                  min_value = this_value;
                  best_termination = i;
                }
              }
              tck.erase (tck.begin() + best_termination + 1, tck.end());

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
              if (dir_rk1.dot (method.dir) < S.cos_max_angle_rk4)
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


