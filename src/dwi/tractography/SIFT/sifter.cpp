/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 * 
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * For more details, see www.mrtrix.org
 * 
 */

#include "dwi/tractography/SIFT/sifter.h"

#include "progressbar.h"
#include "memory.h"
#include "timer.h"

#include "algo/loop.h"

#include "dwi/tractography/file.h"
#include "dwi/tractography/properties.h"

#include "dwi/tractography/ACT/tissues.h"

#include "dwi/tractography/mapping/loader.h"
#include "dwi/tractography/mapping/mapper.h"
#include "dwi/tractography/mapping/mapping.h"

#include "file/ofstream.h"

#include "math/rng.h"

#include "formats/fixel/legacy/image.h"



namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {
      namespace SIFT
      {



      void SIFTer::perform_filtering()
      {

        enum recalc_reason { UNDEFINED, NONLINEARITY, QUANTISATION, TERM_COUNT, TERM_RATIO, TERM_MU, POS_GRADIENT };

        // For streamlines that do not contribute to the map, remove an equivalent proportion of length to those that do contribute
        double sum_contributing_length = 0.0, sum_noncontributing_length = 0.0;
        std::vector<track_t> noncontributing_indices;
        for (track_t i = 0; i != contributions.size(); ++i) {
          if (contributions[i]) {
            if (contributions[i]->get_total_contribution()) {
              sum_contributing_length    += contributions[i]->get_total_length();
            } else {
              sum_noncontributing_length += contributions[i]->get_total_length();
              noncontributing_indices.push_back (i);
            }
          }
        }
        double contributing_length_removed = 0.0, noncontributing_length_removed = 0.0;
        // Randomise the order or removal here; faster than trying to select at random later
        std::random_shuffle (noncontributing_indices.begin(), noncontributing_indices.end());

        std::vector<Cost_fn_gradient_sort> gradient_vector;
        try {
          gradient_vector.assign (num_tracks(), Cost_fn_gradient_sort (num_tracks(), std::numeric_limits<double>::max(), std::numeric_limits<double>::max()));
        } catch (...) {
          throw Exception ("Error assigning memory for SIFT gradient vector");
        }

        unsigned int tracks_remaining = num_tracks();

        if (tracks_remaining < term_number)
          throw Exception ("Filtering failed; desired number of filtered streamlines is greater than or equal to the size of the input dataset");

        const double init_cf = calc_cost_function();
        unsigned int iteration = 0;
        double cf_end_iteration = init_cf;
        unsigned int removed_this_iteration = 0;

        if (!csv_path.empty()) {
          File::OFStream csv_out (csv_path, std::ios_base::out | std::ios_base::trunc);
          csv_out << "Iteration,Removed this iteration,Total removed,Remaining,Cost,TD,Mu,Recalculation,\n";
          csv_out << "0,0,0," << str (tracks_remaining) << "," << str (init_cf) << "," << str (TD_sum) << "," << str (mu()) << ",Start,\n";
        }

        auto display_func = [&](){ return printf(" %6u      %7u     %9u       %.2f%%", iteration, removed_this_iteration, tracks_remaining, 100.0 * cf_end_iteration / init_cf); };
        CONSOLE ("       Iteration     Removed     Remaining     Cost fn");
        ProgressBar progress ("");

        bool another_iteration = true;
        recalc_reason recalculate (UNDEFINED);

        do {

          ++iteration;

          const double current_mu     = mu();
          const double current_cf     = calc_cost_function();
          const double current_roc_cf = calc_roc_cost_function();


          TrackIndexRangeWriter range_writer (SIFT_TRACK_INDEX_BUFFER_SIZE, num_tracks());
          TrackGradientCalculator gradient_calculator (*this, gradient_vector, current_mu, current_roc_cf);
          Thread::run_queue (range_writer, TrackIndexRange(), Thread::multi (gradient_calculator));


          // Theoretically possible to optimise the sorting block size at execution time
          // * Estimate the smallest possible block size that will not overload the std::set
          // * Simulate a gradient vector using a Gaussian distribution of gradients (ignore length dependence)
          // * Simulate sorting this gradient vector; need to simulate both the sort, and some number of get() calls
          // * Perform a golden section search to find the optimal block size
          // This wasn't implemented as the optimal block size seems pretty stable regardless of gradient vector size

          // Ideally the sorting block size should change dynamically as streamlines are filtered
          // This is to reduce the load on the single-threaded section as the multi-threaded sorting complexity declines
          //   (as more streamlines are no longer present, hence have a null gradient and are excluded from the full sort)
          // Tried for an algebraic solution but the numbers didn't line up with my experiments
          // Trying a heuristic for now; go for a sort size of 1000 following initial sort, assuming half of all
          //   remaining streamlines have a negative gradient

          const track_t sort_size = std::min (num_tracks() / double(Thread::number_of_threads()), std::round (2000.0 * double(num_tracks()) / double(tracks_remaining)));
          MT_gradient_vector_sorter sorter (gradient_vector, sort_size);

          // Remove candidate streamlines one at a time, and correspondingly modify the fixels to which they were attributed
          removed_this_iteration = 0;
          recalculate = UNDEFINED;
          do {

            if (!output_at_counts.empty() && (tracks_remaining == output_at_counts.back())) {
              const std::string prefix = str (tracks_remaining);
              if (App::log_level)
                fprintf (stderr, "\n");
              output_filtered_tracks (tck_file_path, prefix + "_tracks.tck");
              if (output_debug)
                output_all_debug_images (prefix);
              INFO ("\nProportionality coefficient at " + str (tracks_remaining) + " streamlines is " + str (mu()));
              output_at_counts.pop_back();
            }

            if (tracks_remaining == term_number) {
              another_iteration = false;
              recalculate = TERM_COUNT;
              goto end_iteration;
            }

            if (term_mu && (mu() > term_mu)) {
              another_iteration = false;
              recalculate = TERM_MU;
              goto end_iteration;
            }

            // Determine whether or not it is appropriate to remove a non-contributing streamline at this point
            if (sum_noncontributing_length && ((contributing_length_removed / sum_contributing_length) > (noncontributing_length_removed / sum_noncontributing_length))) {

              // Select a non-contributing streamline at random
              const track_t to_remove = noncontributing_indices.back();
              noncontributing_indices.pop_back();

              // Remove this streamline, and adjust all of the relevant quantities
              noncontributing_length_removed += contributions[to_remove]->get_total_length();
              delete contributions[to_remove];
              contributions[to_remove] = nullptr;
              ++removed_this_iteration;
              --tracks_remaining;

            } else { // Proceed as normal

              const std::vector<Cost_fn_gradient_sort>::iterator candidate = sorter.get();

              const track_t candidate_index = candidate->get_tck_index();

              if (candidate->get_cost_gradient() >= 0.0) {
                recalculate = POS_GRADIENT;
                if (!removed_this_iteration)
                  another_iteration = false;
                goto end_iteration;
              }

              assert (candidate_index != num_tracks());
              assert (contributions[candidate_index]);

              const double streamline_density_ratio = candidate->get_cost_gradient() / (sum_contributing_length - contributing_length_removed);
              const double required_cf_change_ratio = - term_ratio * streamline_density_ratio * current_cf;

              const TrackContribution& candidate_contribution (*contributions[candidate_index]);

              const double old_mu = mu();
              const double new_mu = FOD_sum / (TD_sum - candidate_contribution.get_total_contribution());
              const double mu_change = new_mu - old_mu;

              // Initial estimate of cost change knowing only the change to the normalisation coefficient
              double this_actual_cf_change = current_roc_cf * mu_change;
              double quantisation = 0.0;

              for (size_t f = 0; f != candidate_contribution.dim(); ++f) {
                const Track_fixel_contribution& fixel_cont = candidate_contribution[f];
                const float length = fixel_cont.get_length();
                Fixel& this_fixel = fixels[fixel_cont.get_fixel_index()];
                quantisation += this_fixel.calc_quantisation (old_mu, length);
                const double undo_change_mu_only = this_fixel.get_d_cost_d_mu (old_mu) * mu_change;
                const double change_remove_tck = this_fixel.get_cost_wo_track (new_mu, length) - this_fixel.get_cost (old_mu);
                this_actual_cf_change = this_actual_cf_change - undo_change_mu_only + change_remove_tck;
              }

              const double required_cf_change_quantisation = enforce_quantisation ? (-0.5 * quantisation) : 0.0;
              const double this_nonlinearity = (candidate->get_cost_gradient() - this_actual_cf_change);

              if (this_actual_cf_change < std::min ( {required_cf_change_ratio, required_cf_change_quantisation, this_nonlinearity })) {

                // Candidate streamline removal meets all criteria; remove from reconstruction
                for (size_t f = 0; f != candidate_contribution.dim(); ++f) {
                  const Track_fixel_contribution& fixel_cont = candidate_contribution[f];
                  fixels[fixel_cont.get_fixel_index()] -= fixel_cont.get_length();
                }
                TD_sum -= candidate_contribution.get_total_contribution();
                contributing_length_removed += candidate_contribution.get_total_length();
                delete contributions[candidate_index];
                contributions[candidate_index] = nullptr;
                ++removed_this_iteration;
                --tracks_remaining;

              } else {

                // Removal doesn't meet all criteria

                if (this_actual_cf_change >= this_nonlinearity)
                  recalculate = NONLINEARITY;
                else if (term_ratio && this_actual_cf_change >= required_cf_change_ratio)
                  recalculate = TERM_RATIO;
                else
                  recalculate = QUANTISATION;
                if (!removed_this_iteration) {
                  // If filtering has been completed to convergence, but the user does not want to filter to convergence
                  //   (i.e. they have defined a desired termination criterion but it has not yet been met), disable
                  //   the quantisation check to give the algorithm a chance to meet the user's termination request
                  if (enforce_quantisation && (term_number || term_ratio || term_mu)) {
                    if (App::log_level)
                      fprintf (stderr, "\n");
                    WARN ("filtering has reached quantisation error but desired termination criterion has not been met;");
                    WARN ("  disabling cost function quantisation check");
                    enforce_quantisation = false;
                  } else {
                    // Filtering completed to convergence
                    another_iteration = false;
                  }
                }
                goto end_iteration;

              } // End checking validity of candidate streamline removal

            } // End switching between removal of zero-contribution or nonzero-contribution streamline

          } while (!recalculate); // End removing streamlines in this iteration

          end_iteration:

          cf_end_iteration = calc_cost_function();

          progress.update (display_func);

          if (!csv_path.empty()) {
            File::OFStream csv_out (csv_path, std::ios_base::out | std::ios_base::app | std::ios_base::ate);
            csv_out << str (iteration) << "," << str (removed_this_iteration) << "," << str (num_tracks() - tracks_remaining) << "," << str (tracks_remaining) << "," << str (cf_end_iteration) << "," << str (TD_sum) << "," << str (mu()) << ",";
            switch (recalculate) {
              case UNDEFINED:    throw Exception ("Encountered undefined recalculation at end of iteration!");
              case NONLINEARITY: csv_out << "Non-linearity"; break;
              case QUANTISATION: csv_out << "Quantisation"; break;
              case TERM_COUNT:   csv_out << "Target streamline count"; break;
              case TERM_RATIO:   csv_out << "Termination ratio"; break;
              case TERM_MU:      csv_out << "Target proportionality coefficient"; break;
              case POS_GRADIENT: csv_out << "Positive gradient"; break;
            }
            csv_out << ",\n";
          }

        } while (another_iteration);

        progress.done();

        switch (recalculate) {
          case UNDEFINED:    throw Exception ("Encountered undefined recalculation at end of iteration!");
          case NONLINEARITY: INFO ("Filtering terminated due to instability in cost function gradients"); break;
          case QUANTISATION: INFO ("Filtering terminated due to candidate streamline failing to exceed quantisation"); break;
          case TERM_COUNT:   INFO ("Filtering terminated due to reaching desired streamline count"); break;
          case TERM_RATIO:   INFO ("Filtering terminated due to cost function / streamline density decrease ratio"); break;
          case TERM_MU:      INFO ("Filtering terminated due to reaching desired proportionality coefficient"); break;
          case POS_GRADIENT: INFO ("Filtering terminated due to candidate streamline having positive gradient"); break;
        }

        if ((term_number || term_ratio || term_mu)
            && (recalculate == NONLINEARITY || recalculate == QUANTISATION || recalculate == POS_GRADIENT))
          WARN ("algorithm terminated before any user-specified termination criterion was met");

        INFO ("Proportionality coefficient at end of filtering is " + str (mu()));

      }





      void SIFTer::output_filtered_tracks (const std::string& input_path, const std::string& output_path) const
      {
        Tractography::Properties p;
        Tractography::Reader<float> reader (input_path, p);
        p["SIFT_mu"] = str (mu());
        Tractography::Writer<float> writer (output_path, p);
        track_t tck_counter = 0;
        Tractography::Streamline<> tck;
        ProgressBar progress ("Writing filtered tracks output file", contributions.size());
        Tractography::Streamline<> empty_tck;
        while (reader (tck) && tck_counter < contributions.size()) {
          if (contributions[tck_counter++])
            writer (tck);
          else
            writer (empty_tck);
          ++progress;
        }
        reader.close();
      }





      void SIFTer::output_selection (const std::string& path) const
      {
        File::OFStream out (path, std::ios_base::out | std::ios_base::trunc);
        for (track_t i = 0; i != contributions.size(); ++i) {
          if (contributions[i])
            out << "1\n";
          else
            out << "0\n";
        }
      }









      void SIFTer::set_regular_outputs (const std::vector<int>& in, const bool b)
      {
        for (std::vector<int>::const_iterator i = in.begin(); i != in.end(); ++i) {
          if (*i > 0 && *i <= (int)contributions.size())
            output_at_counts.push_back (*i);
        }
        sort (output_at_counts.begin(), output_at_counts.end());
        output_debug = b;
      }




      void SIFTer::test_sorting_block_size (const size_t num_tracks) const
      {

        Math::RNG::Normal<float> rng;

        std::vector<Cost_fn_gradient_sort> gradient_vector;
        gradient_vector.assign (num_tracks, Cost_fn_gradient_sort (num_tracks, 0.0, 0.0));
        // Fill the gradient vector with random Gaussian data
        for (track_t index = 0; index != num_tracks; ++index) {
          const float value = rng();
          gradient_vector[index].set (index, value, value);
        }

        std::vector<size_t> block_sizes;
        for (size_t i = 16; i < num_tracks; i *= 2)
          block_sizes.push_back (i);
        block_sizes.push_back (num_tracks);

        for (std::vector<size_t>::const_iterator i = block_sizes.begin(); i != block_sizes.end(); ++i) {
          const size_t block_size = *i;

          // Make a copy of the gradient vector, so the same data is sorted each time
          std::vector<Cost_fn_gradient_sort> temp_gv (gradient_vector);

          Timer timer;
          // Simulate sorting and filtering
          try {
            MT_gradient_vector_sorter sorter (temp_gv, block_size);
            for (size_t candidate_count = 0; candidate_count < num_tracks / 1000; ++candidate_count)
              sorter.get();
            std::cerr << "Time required for sorting " << num_tracks << " tracks, block size " << block_size << " = " << timer.elapsed() * 1000.0 << "ms\n";
          } catch (...) {
            std::cerr << "Could not sort " << num_tracks << "tracks with block size " << block_size << "\n";
          }



        }

      }






      // Convenience functions

      double SIFTer::calc_roc_cost_function() const
      {
        const double current_mu = mu();
        double roc_cost = 0.0;
        std::vector<Fixel>::const_iterator i = fixels.begin();
        for (++i; i != fixels.end(); ++i)
          roc_cost += i->get_d_cost_d_mu (current_mu);
        return roc_cost;
      }

      double SIFTer::calc_gradient (const track_t index, const double current_mu, const double current_roc_cost) const
      {
        if (!contributions[index])
          return std::numeric_limits<double>::max();
        const TrackContribution& tck_cont = *contributions[index];
        const double TD_sum_if_removed = TD_sum - tck_cont.get_total_contribution();
        const double mu_if_removed = FOD_sum / TD_sum_if_removed;
        const double mu_change_if_removed = mu_if_removed - current_mu;
        double gradient = current_roc_cost * mu_change_if_removed;
        for (size_t f = 0; f != tck_cont.dim(); ++f) {
          const Fixel& fixel = fixels[tck_cont[f].get_fixel_index()];
          const double undo_gradient_mu_only = fixel.get_d_cost_d_mu (current_mu) * mu_change_if_removed;
          const double gradient_remove_tck = fixel.get_cost_wo_track (mu_if_removed, tck_cont[f].get_length()) - fixel.get_cost (current_mu);
          gradient = gradient - undo_gradient_mu_only + gradient_remove_tck;
        }
        return gradient;
      }






      bool SIFTer::TrackGradientCalculator::operator() (const TrackIndexRange& in) const
      {
        for (track_t track_index = in.first; track_index != in.second; ++track_index) {
          if (master.contributions[track_index]) {
            const double gradient = master.calc_gradient (track_index, current_mu, current_roc_cost);
            const double grad_per_unit_length = master.contributions[track_index]->get_total_contribution() ? (gradient / master.contributions[track_index]->get_total_contribution()) : 0.0;
            gradient_vector[track_index].set (track_index, gradient, grad_per_unit_length);
          } else {
            gradient_vector[track_index].set (master.num_tracks(), 0.0, 0.0);
          }
        }
        return true;
      }





      }
    }
  }
}



