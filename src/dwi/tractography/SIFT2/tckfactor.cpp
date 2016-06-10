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


#include "bitset.h"
#include "header.h"
#include "image.h"

#include "math/math.h"

#include "sparse/fixel_metric.h"
#include "sparse/image.h"
#include "sparse/keys.h"

#include "dwi/tractography/SIFT2/coeff_optimiser.h"
#include "dwi/tractography/SIFT2/fixel_updater.h"
#include "dwi/tractography/SIFT2/reg_calculator.h"
#include "dwi/tractography/SIFT2/streamline_stats.h"
#include "dwi/tractography/SIFT2/tckfactor.h"

#include "dwi/tractography/SIFT/track_index_range.h"






namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace SIFT2 {




      void TckFactor::set_reg_lambdas (const double lambda_tikhonov, const double lambda_tv)
      {
        assert (num_tracks());
        double A = 0.0, sum_PM = 0.0;
        for (size_t i = 1; i != fixels.size(); ++i) {
          A += fixels[i].get_weight() * Math::pow2 (fixels[i].get_FOD());
          sum_PM += fixels[i].get_weight();
        }
        A /= double(num_tracks());
        INFO ("Constant A scaling regularisation terms to match data term is " + str(A));
        reg_multiplier_tikhonov = lambda_tikhonov * A;
        reg_multiplier_tv       = lambda_tv * A;
      }



      void TckFactor::store_orig_TDs()
      {
        for (std::vector<Fixel>::iterator i = fixels.begin(); i != fixels.end(); ++i)
          i->store_orig_TD();
      }



      void TckFactor::remove_excluded_fixels (const float min_td_frac)
      {
        Model<Fixel>::remove_excluded_fixels();

        // In addition to the complete exclusion, want to identify poorly-tracked fixels and
        //   exclude them from the optimisation (though they will still remain in the model)

        // There's no particular pattern to it; just use a hard threshold
        // Would prefer not to actually modify the streamline visitations; just exclude fixels from optimisation
        const double fixed_mu = mu();
        const double cf = calc_cost_function();
        SIFT::track_t excluded_count = 0, zero_TD_count = 0;
        double zero_TD_cf_sum = 0.0, excluded_cf_sum = 0.0;
        for (std::vector<Fixel>::iterator i = fixels.begin(); i != fixels.end(); ++i) {
          if (!i->get_orig_TD()) {
            ++zero_TD_count;
            zero_TD_cf_sum += i->get_cost (fixed_mu);
          } else if ((fixed_mu * i->get_orig_TD() < min_td_frac * i->get_FOD()) || (i->get_count() == 1)) {
            i->exclude();
            ++excluded_count;
            excluded_cf_sum += i->get_cost (fixed_mu);
          }
        }
        INFO (str(zero_TD_count) + " fixels have no attributed streamlines; these account for " + str(100.0 * zero_TD_cf_sum / cf) + "\% of the initial cost function");
        if (excluded_count) {
          INFO (str(excluded_count) + " of " + str(fixels.size()) + " fixels were tracked, but have been excluded from optimisation due to inadequate reconstruction;");
          INFO ("these contribute " + str (100.0 * excluded_cf_sum / cf) + "\% of the initial cost function");
        } else if (min_td_frac) {
          INFO ("No fixels were excluded from optimisation due to poor reconstruction");
        }
      }




      void TckFactor::test_streamline_length_scaling()
      {
        VAR (calc_cost_function());

        for (std::vector<Fixel>::iterator i = fixels.begin(); i != fixels.end(); ++i)
          i->clear_TD();

        coefficients.resize (num_tracks(), 0.0);
        TD_sum = 0.0;

        for (SIFT::track_t track_index = 0; track_index != num_tracks(); ++track_index) {
          const SIFT::TrackContribution& tck_cont (*contributions[track_index]);
          const double weight = 1.0 / tck_cont.get_total_length();
          coefficients[track_index] = std::log (weight);
          for (size_t i = 0; i != tck_cont.dim(); ++i)
            fixels[tck_cont[i].get_fixel_index()] += weight * tck_cont[i].get_length();
          TD_sum += weight * tck_cont.get_total_contribution();
        }

        VAR (calc_cost_function());

        // Also test varying mu; produce a scatter plot
        const double actual_TD_sum = TD_sum;
        std::ofstream out ("mu.csv", std::ios_base::trunc);
        for (int i = -1000; i != 1000; ++i) {
          const double factor = std::pow (10.0, double(i) / 1000.0);
          TD_sum = factor * actual_TD_sum;
          out << str(factor) << "," << str(calc_cost_function()) << "\n";
        }
        out << "\n";
        out.close();

        TD_sum = actual_TD_sum;
      }



      void TckFactor::calc_afcsa()
      {

        VAR (calc_cost_function());

        coefficients.resize (num_tracks(), 0.0);

        const double fixed_mu = mu();

        // Just do single-threaded for now
        for (SIFT::track_t i = 0; i != num_tracks(); ++i) {
          const SIFT::TrackContribution& tckcont = *contributions[i];
          double sum_afd = 0.0;
          for (size_t f = 0; f != tckcont.dim(); ++f) {
            const size_t fixel_index = tckcont[f].get_fixel_index();
            const Fixel& fixel = fixels[fixel_index];
            const float length = tckcont[f].get_length();
            sum_afd += fixel.get_weight() * fixel.get_FOD() * (length / fixel.get_orig_TD());
          }
          const double afcsa = sum_afd / tckcont.get_total_contribution();
          coefficients[i] = std::log (afcsa / fixed_mu);
        }

        for (std::vector<Fixel>::iterator i = fixels.begin(); i != fixels.end(); ++i) {
          i->clear_TD();
          i->clear_mean_coeff();
        }
        {
          SIFT::TrackIndexRangeWriter writer (SIFT_TRACK_INDEX_BUFFER_SIZE, num_tracks());
          FixelUpdater worker (*this);
          Thread::run_queue (writer, SIFT::TrackIndexRange(), Thread::multi (worker));
        }

        VAR (calc_cost_function());

      }




      void TckFactor::estimate_factors()
      {

        try {
          coefficients = decltype(coefficients)::Zero (num_tracks());
        } catch (...) {
          throw Exception ("Error assigning memory for streamline weights vector");
        }

        const double init_cf = calc_cost_function();
        double cf_data = init_cf;
        double new_cf = init_cf;
        double prev_cf = init_cf;
        double cf_reg = 0.0;
        const double required_cf_change = -min_cf_decrease_percentage * init_cf;

        unsigned int nonzero_streamlines = 0;
        for (SIFT::track_t i = 0; i != num_tracks(); ++i) {
          if (contributions[i] && contributions[i]->dim())
            ++nonzero_streamlines;
        }

        unsigned int iter = 0;
        
        auto display_func = [&](){ return printf("    %5u        %3.3f%%         %2.3f%%        %u", iter, 100.0 * cf_data / init_cf, 100.0 * cf_reg / init_cf, nonzero_streamlines); };
        CONSOLE ("  Iteration     CF (data)      CF (reg)     Streamlines");
        ProgressBar progress ("");

        // Keep track of total exclusions, not just how many are removed in each iteration
        size_t total_excluded = 0;
        for (size_t i = 1; i != fixels.size(); ++i) {
          if (fixels[i].is_excluded())
            ++total_excluded;
        }

        std::unique_ptr<std::ofstream> csv_out;
        if (!csv_path.empty()) {
          csv_out.reset (new std::ofstream());
          csv_out->open (csv_path.c_str(), std::ios_base::trunc);
          (*csv_out) << "Iteration,Cost_data,Cost_reg_tik,Cost_reg_tv,Cost_reg,Cost_total,Streamlines,Fixels_excluded,Step_min,Step_mean,Step_mean_abs,Step_var,Step_max,Coeff_min,Coeff_mean,Coeff_mean_abs,Coeff_var,Coeff_max,Coeff_norm,\n";
          (*csv_out) << "0," << init_cf << ",0,0,0," << init_cf << "," << nonzero_streamlines << "," << total_excluded << ",0,0,0,0,0,0,0,0,0,0,0,\n";
          csv_out->flush();
        }

        // Initial estimates of how each weighting coefficient is going to change
        // The ProjectionCalculator classes overwrite these in place, so do an initial allocation but
        //   don't bother wiping it at every iteration
        //std::vector<float> projected_steps (num_tracks(), 0.0);

        // Logging which fixels need to be excluded from optimisation in subsequent iterations,
        //   due to driving streamlines to unwanted high weights
        BitSet fixels_to_exclude (fixels.size());

        do {

          ++iter;
          prev_cf = new_cf;

          // Line search to optimise each coefficient
          StreamlineStats step_stats, coefficient_stats;
          nonzero_streamlines = 0;
          fixels_to_exclude.clear();
          double sum_costs = 0.0;
          {
            SIFT::TrackIndexRangeWriter writer (SIFT_TRACK_INDEX_BUFFER_SIZE, num_tracks());
            //CoefficientOptimiserGSS worker (*this, /*projected_steps,*/ step_stats, coefficient_stats, nonzero_streamlines, fixels_to_exclude, sum_costs);
            //CoefficientOptimiserQLS worker (*this, /*projected_steps,*/ step_stats, coefficient_stats, nonzero_streamlines, fixels_to_exclude, sum_costs);
            CoefficientOptimiserIterative worker (*this, /*projected_steps,*/ step_stats, coefficient_stats, nonzero_streamlines, fixels_to_exclude, sum_costs);
            Thread::run_queue (writer, SIFT::TrackIndexRange(), Thread::multi (worker));
          }
          step_stats.normalise();
          coefficient_stats.normalise();
          indicate_progress();

          // Perform fixel exclusion
          const size_t excluded_count = fixels_to_exclude.count();
          if (excluded_count) {
            DEBUG (str(excluded_count) + " fixels excluded this iteration");
            for (size_t f = 0; f != fixels.size(); ++f) {
              if (fixels_to_exclude[f])
                fixels[f].exclude();
            }
            total_excluded += excluded_count;
          }

          // Multi-threaded calculation of updated streamline density, and mean weighting coefficient, in each fixel
          for (std::vector<Fixel>::iterator i = fixels.begin(); i != fixels.end(); ++i) {
            i->clear_TD();
            i->clear_mean_coeff();
          }
          {
            SIFT::TrackIndexRangeWriter writer (SIFT_TRACK_INDEX_BUFFER_SIZE, num_tracks());
            FixelUpdater worker (*this);
            Thread::run_queue (writer, SIFT::TrackIndexRange(), Thread::multi (worker));
          }
          // Scale the fixel mean coefficient terms (each streamline in the fixel is weighted by its length)
          for (std::vector<Fixel>::iterator i = fixels.begin(); i != fixels.end(); ++i)
            i->normalise_mean_coeff();
          indicate_progress();

          cf_data = calc_cost_function();

          // Calculate the cost of regularisation, given the updates to both the
          //   streamline weighting coefficients and the new fixel mean coefficients
          // Log different regularisation costs separately
          double cf_reg_tik = 0.0, cf_reg_tv = 0.0;
          {
            SIFT::TrackIndexRangeWriter writer (SIFT_TRACK_INDEX_BUFFER_SIZE, num_tracks());
            RegularisationCalculator worker (*this, cf_reg_tik, cf_reg_tv);
            Thread::run_queue (writer, SIFT::TrackIndexRange(), Thread::multi (worker));
          }
          cf_reg_tik *= reg_multiplier_tikhonov;
          cf_reg_tv  *= reg_multiplier_tv;

          cf_reg = cf_reg_tik + cf_reg_tv;

          new_cf = cf_data + cf_reg;

          if (!csv_path.empty()) {
            (*csv_out) << str (iter) << "," << str (cf_data) << "," << str (cf_reg_tik) << "," << str (cf_reg_tv) << "," << str (cf_reg) << "," << str (new_cf) << "," << str (nonzero_streamlines) << "," << str (total_excluded) << ","
                << str (step_stats       .get_min()) << "," << str (step_stats       .get_mean()) << "," << str (step_stats       .get_mean_abs()) << "," << str (step_stats       .get_var()) << "," << str (step_stats       .get_max()) << ","
                << str (coefficient_stats.get_min()) << "," << str (coefficient_stats.get_mean()) << "," << str (coefficient_stats.get_mean_abs()) << "," << str (coefficient_stats.get_var()) << "," << str (coefficient_stats.get_max()) << ","
                << str (coefficient_stats.get_var() * (num_tracks() - 1))
                << ",\n";
            csv_out->flush();
          }

          progress.update (display_func);
          
          // Leaving out testing the fixel exclusion mask criterion; doesn't converge, and results in CF increase
        } while (((new_cf - prev_cf < required_cf_change) || (iter < min_iters) /* || !fixels_to_exclude.empty() */ ) && (iter < max_iters));

        progress.done();
      }




      void TckFactor::output_factors (const std::string& path) const
      {
        if (size_t(coefficients.size()) != contributions.size())
          throw Exception ("Cannot output weighting factors if they have not first been estimated!");
        try {
          decltype(coefficients) weights (coefficients.size());
          for (SIFT::track_t i = 0; i != num_tracks(); ++i)
            weights[i] = std::exp (coefficients[i]);
          save_vector (weights, path);
        } catch (...) {
          WARN ("Unable to assign memory for output factor file: \"" + Path::basename(path) + "\" not created");
        }
      }


      void TckFactor::output_coefficients (const std::string& path) const
      {
        save_vector (coefficients, path);
      }




      void TckFactor::output_all_debug_images (const std::string& prefix) const
      {

        Model<Fixel>::output_all_debug_images (prefix);

        if (!coefficients.size())
          return;

        std::vector<double> mins   (fixels.size(), 100.0);
        std::vector<double> stdevs (fixels.size(), 0.0);
        std::vector<double> maxs   (fixels.size(), -100.0);
        std::vector<size_t> zeroed (fixels.size(), 0);

        {
          ProgressBar progress ("Generating streamline coefficient statistic images", num_tracks());
          for (SIFT::track_t i = 0; i != num_tracks(); ++i) {
            const double coeff = coefficients[i];
            const SIFT::TrackContribution& this_contribution (*contributions[i]);
            if (coeff > min_coeff) {
              for (size_t j = 0; j != this_contribution.dim(); ++j) {
                const size_t fixel_index = this_contribution[j].get_fixel_index();
                const double mean_coeff = fixels[fixel_index].get_mean_coeff();
                mins  [fixel_index] = std::min (mins[fixel_index], coeff);
                stdevs[fixel_index] += Math::pow2 (coeff - mean_coeff);
                maxs  [fixel_index] = std::max (maxs[fixel_index], coeff);
              }
            } else {
              for (size_t j = 0; j != this_contribution.dim(); ++j)
                ++zeroed[this_contribution[j].get_fixel_index()];
            }
            ++progress;
          }
        }

        for (size_t i = 1; i != fixels.size(); ++i) {
          if (mins[i] == 100.0)
            mins[i] = 0.0;
          stdevs[i] = (fixels[i].get_count() > 1) ? (std::sqrt (stdevs[i] / float(fixels[i].get_count() - 1))) : 0.0;
          if (maxs[i] == -100.0)
            maxs[i] = 0.0;
        }

        using Sparse::FixelMetric;
        Header H_fixel (Fixel_map<Fixel>::header());
        H_fixel.datatype() = DataType::UInt64;
        H_fixel.datatype().set_byte_order_native();
        H_fixel.keyval()[Sparse::name_key] = str(typeid(FixelMetric).name());
        H_fixel.keyval()[Sparse::size_key] = str(sizeof(FixelMetric));

        Sparse::Image<FixelMetric> count_image    (prefix + "_count.msf",        H_fixel);
        Sparse::Image<FixelMetric> min_image      (prefix + "_coeff_min.msf",    H_fixel);
        Sparse::Image<FixelMetric> mean_image     (prefix + "_coeff_mean.msf",   H_fixel);
        Sparse::Image<FixelMetric> stdev_image    (prefix + "_coeff_stdev.msf",  H_fixel);
        Sparse::Image<FixelMetric> max_image      (prefix + "_coeff_max.msf",    H_fixel);
        Sparse::Image<FixelMetric> zeroed_image   (prefix + "_coeff_zeroed.msf", H_fixel);
        Sparse::Image<FixelMetric> excluded_image (prefix + "_excluded.msf",     H_fixel);

        VoxelAccessor v (accessor());
        for (auto l = Loop(v) (v, count_image, min_image, mean_image, stdev_image, max_image, zeroed_image, excluded_image); l; ++l) {
          if (v.value()) {

            count_image   .value().set_size ((*v.value()).num_fixels());
            min_image     .value().set_size ((*v.value()).num_fixels());
            mean_image    .value().set_size ((*v.value()).num_fixels());
            stdev_image   .value().set_size ((*v.value()).num_fixels());
            max_image     .value().set_size ((*v.value()).num_fixels());
            zeroed_image  .value().set_size ((*v.value()).num_fixels());
            excluded_image.value().set_size ((*v.value()).num_fixels());

            size_t index = 0;
            for (typename Fixel_map<Fixel>::ConstIterator iter = begin (v); iter; ++iter, ++index) {
              const size_t fixel_index = size_t(iter);
              FixelMetric fixel_metric (iter().get_dir(), iter().get_FOD(), iter().get_count());
              count_image   .value()[index] = fixel_metric;
              fixel_metric.value = mins[fixel_index];
              min_image     .value()[index] = fixel_metric;
              fixel_metric.value = iter().get_mean_coeff();
              mean_image    .value()[index] = fixel_metric;
              fixel_metric.value = stdevs[fixel_index];
              stdev_image   .value()[index] = fixel_metric;
              fixel_metric.value = maxs[fixel_index];
              max_image     .value()[index] = fixel_metric;
              fixel_metric.value = zeroed[fixel_index];
              zeroed_image  .value()[index] = fixel_metric;
              fixel_metric.value = iter().is_excluded() ? 0.0 : 1.0;
              excluded_image.value()[index] = fixel_metric;
            }

          }
        }
      }






      }
    }
  }
}



