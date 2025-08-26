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

#include "header.h"
#include "image.h"

#include "math/math.h"
#include "misc/bitset.h"

#include "fixel/legacy/fixel_metric.h"
#include "fixel/legacy/image.h"
#include "fixel/legacy/keys.h"

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




      void TckFactor::set_reg_lambdas (const value_type lambda_tikhonov, const value_type lambda_tv)
      {
        assert (num_tracks());
        // value_type A = 0.0;
        // for (MR::Fixel::index_type i = 0; i != nfixels(); ++i) {
        //   Fixel fixel (*this, i);
        //   A += fixel.weight() * Math::pow2 (fixel.fd());
        // }
        // A /= value_type(num_tracks());
        const value_type A = fixels.col(model_weight_column).matrix().dot(fixels.col(fd_column).square().matrix()) / value_type(num_tracks());
        INFO ("Constant A scaling regularisation terms to match data term is " + str(A));
        reg_multiplier_tikhonov = lambda_tikhonov * A;
        reg_multiplier_tv       = lambda_tv * A;
      }



      void TckFactor::store_orig_TDs()
      {
        fixels.col(orig_td_column) = fixels.col(td_column);
      }



      void TckFactor::exclude_fixels (const value_type min_td_frac)
      {
        Model::exclude_fixels();

        // In addition to the complete exclusion, want to identify poorly-tracked fixels and
        //   exclude them from the optimisation (though they will still remain in the model)

        const value_type fixed_mu = mu();
        const value_type cf = calc_cost_function();
        SIFT::track_t excluded_count = 0, zero_TD_count = 0;
        value_type zero_TD_cf_sum = 0.0, excluded_cf_sum = 0.0;
        for (MR::Fixel::index_type i = 0; i != nfixels(); ++i) {
          Fixel fixel (*this, i);
          if (fixel.weight()) {
            if (!fixel.orig_TD()) {
              ++zero_TD_count;
              zero_TD_cf_sum += fixel.get_cost (fixed_mu);
            } else if ((fixed_mu * fixel.orig_TD() < min_td_frac * fixel.fd()) || (fixel.count() < 2)) {
              fixel.exclude();
              ++excluded_count;
              excluded_cf_sum += fixel.get_cost (fixed_mu);
            }
          }
        }
        if (zero_TD_count) {
          INFO (str(zero_TD_count) + " fixels have no attributed streamlines; "
                "these account for " + str(100.0 * zero_TD_cf_sum / cf) + "\% of the initial cost function");
        }
        if (excluded_count) {
          INFO (str(excluded_count) + " of " + str(fixels.size()) + " fixels were tracked, "
                "but have been excluded from optimisation due to inadequate reconstruction; "
                "these contribute " + str (100.0 * excluded_cf_sum / cf) + "\% of the initial cost function");
        } else {
          INFO ("No fixels were excluded from optimisation due to poor reconstruction");
        }
      }




      void TckFactor::test_streamline_length_scaling()
      {
        VAR (calc_cost_function());

        fixels.col(td_column).setZero();
        fixels.col(track_count_column).setZero();

        coefficients.resize (num_tracks(), 0.0);
        TD_sum = 0.0;

        for (SIFT::track_t track_index = 0; track_index != num_tracks(); ++track_index) {
          const SIFT::TrackContribution& tck_cont (*contributions[track_index]);
          const value_type streamline_weight = 1.0 / tck_cont.get_total_length();
          coefficients[track_index] = std::log (streamline_weight);
          for (size_t i = 0; i != tck_cont.dim(); ++i) {
            Fixel fixel (*this, tck_cont[i].get_fixel_index());
            fixel += streamline_weight * tck_cont[i].get_length();
          }
          TD_sum += streamline_weight * tck_cont.get_total_contribution();
        }

        VAR (calc_cost_function());

        // Also test varying mu; produce a scatter plot
        const value_type actual_TD_sum = TD_sum;
        {
          std::ofstream out ("mu.csv", std::ios_base::trunc);
          for (int i = -1000; i != 1000; ++i) {
            const value_type factor = std::pow (10.0, value_type(i) / 1000.0);
            TD_sum = factor * actual_TD_sum;
            out << str(factor) << "," << str(calc_cost_function()) << "\n";
          }
          out << "\n";
        }

        TD_sum = actual_TD_sum;
      }



      void TckFactor::calc_afcsa()
      {

        CONSOLE ("Cost function before linear optimisation is " + str(calc_cost_function()) + ")");

        try {
          coefficients = decltype(coefficients)::Zero (num_tracks());
        } catch (...) {
          throw Exception ("Error assigning memory for streamline weights vector");
        }

        class Functor
        {
          public:
            Functor (TckFactor& master) :
                master (master),
                fixed_mu (master.mu()) { }
            Functor (const Functor&) = default;
            bool operator() (const SIFT::TrackIndexRange& range) const {
              for (auto track_index : range) {
                const SIFT::TrackContribution& tckcont = *master.contributions[track_index];
                value_type sum_afd = 0.0;
                for (size_t f = 0; f != tckcont.dim(); ++f) {
                  const MR::Fixel::index_type fixel_index = tckcont[f].get_fixel_index();
                  const TckFactor::Fixel fixel (master, fixel_index);
                  const float length = tckcont[f].get_length();
                  sum_afd += fixel.weight() * fixel.fd() * (length / fixel.orig_TD());
                }
                if (sum_afd && tckcont.get_total_contribution()) {
                  const value_type afcsa = sum_afd / tckcont.get_total_contribution();
                  master.coefficients[track_index] = std::max (master.min_coeff, std::log (afcsa / fixed_mu));
                } else {
                  master.coefficients[track_index] = master.min_coeff;
                }
              }
              return true;
            }
          private:
            TckFactor& master;
            const value_type fixed_mu;
        };
        {
          SIFT::TrackIndexRangeWriter writer (SIFT_TRACK_INDEX_BUFFER_SIZE, num_tracks());
          Functor functor (*this);
          Thread::run_queue (writer, SIFT::TrackIndexRange(), Thread::multi (functor));
        }

        fixels.col(td_column).setZero();
        fixels.col(mean_coeff_column).setZero();
        {
          SIFT::TrackIndexRangeWriter writer (SIFT_TRACK_INDEX_BUFFER_SIZE, num_tracks());
          FixelUpdater worker (*this);
          Thread::run_queue (writer, SIFT::TrackIndexRange(), Thread::multi (worker));
        }

        CONSOLE ("Cost function after linear optimisation is " + str(calc_cost_function()) + ")");

      }




      void TckFactor::estimate_factors()
      {

        try {
          coefficients = decltype(coefficients)::Zero (num_tracks());
        } catch (...) {
          throw Exception ("Error assigning memory for streamline weights vector");
        }

        const value_type init_cf = calc_cost_function();
        value_type cf_data = init_cf;
        value_type new_cf = init_cf;
        value_type prev_cf = init_cf;
        value_type cf_reg = 0.0;
        const value_type required_cf_change = -min_cf_decrease_percentage * init_cf;

        SIFT::track_t nonzero_streamlines = 0;
        for (SIFT::track_t i = 0; i != num_tracks(); ++i) {
          if (contributions[i] && contributions[i]->dim())
            ++nonzero_streamlines;
        }

        unsigned int iter = 0;

        auto display_func = [&](){ return printf("    %5u        %3.3f%%         %2.3f%%        %u", iter, 100.0 * cf_data / init_cf, 100.0 * cf_reg / init_cf, nonzero_streamlines); };
        CONSOLE ("         Iteration     CF (data)       CF (reg)    Streamlines");
        ProgressBar progress ("");

        // Keep track of total exclusions, not just how many are removed in each iteration
        MR::Fixel::index_type total_excluded = fixels.col(exclude_column).sum();

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
        //vector<float> projected_steps (num_tracks(), 0.0);

        // Logging which fixels need to be excluded from optimisation in subsequent iterations,
        //   due to driving streamlines to unwanted high weights
        BitSet fixels_to_exclude (nfixels());

        do {

          ++iter;
          prev_cf = new_cf;

          // Line search to optimise each coefficient
          StreamlineStats step_stats, coefficient_stats;
          nonzero_streamlines = 0;
          fixels_to_exclude.clear();
          value_type sum_costs = 0.0;
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
          const MR::Fixel::index_type excluded_count = fixels_to_exclude.count();
          if (excluded_count) {
            DEBUG (str(excluded_count) + " fixels excluded this iteration");
            for (MR::Fixel::index_type f = 0; f != nfixels(); ++f) {
              if (fixels_to_exclude[f]) {
                Fixel fixel (*this, f);
                fixel.exclude();
              }
            }
            total_excluded += excluded_count;
          }

          // Multi-threaded calculation of updated streamline density, and mean weighting coefficient, in each fixel
          fixels.col(td_column).setZero();
          fixels.col(mean_coeff_column).setZero();
          {
            SIFT::TrackIndexRangeWriter writer (SIFT_TRACK_INDEX_BUFFER_SIZE, num_tracks());
            FixelUpdater worker (*this);
            Thread::run_queue (writer, SIFT::TrackIndexRange(), Thread::multi (worker));
          }
          // Scale the fixel mean coefficient terms (each streamline in the fixel is weighted by its length)
          for (MR::Fixel::index_type i = 0; i != nfixels(); ++i) {
            Fixel fixel (*this, i);
            fixel.normalise_mean_coeff();
          }
          indicate_progress();

          cf_data = calc_cost_function();

          // Calculate the cost of regularisation, given the updates to both the
          //   streamline weighting coefficients and the new fixel mean coefficients
          // Log different regularisation costs separately
          value_type cf_reg_tik = 0.0, cf_reg_tv = 0.0;
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
      }



      void TckFactor::report_entropy() const
      {
        // H = - <sum_i> P(x_i) log_2 (P(x_i))
        //
        // Before SIFT2:
        // All streamlines have P(x_i) = 1.0 / num_streamlines
        const value_type P_before = 1.0 / coefficients.size();
        const value_type logP_before = std::log2 (P_before);
        const value_type H_before = -coefficients.size() * (P_before * logP_before);
        // After SIFT2:
        // - First, need normalising factor, which is the reciprocal sum of all streamline weights
        //   (as opposed to the reciprocal number of streamlines)
        value_type sum_weights = 0.0;
        for (SIFT::track_t i = 0; i != coefficients.size(); ++i)
          sum_weights += std::exp (coefficients[i]);
        const value_type inv_sum_weights = 1.0 / sum_weights;
        value_type H_after = 0.0;
        for (SIFT::track_t i = 0; i != coefficients.size(); ++i) {
          const value_type P_after = std::exp (coefficients[i]) * inv_sum_weights;
          const value_type logP_after = std::log2 (P_after);
          H_after += P_after * logP_after;
        }
        H_after *= -1.0;
        const size_t equiv_N = std::round (std::pow (2.0, H_after));
        INFO ("Entropy decreased from " + str(H_before, 6) + " to " + str(H_after, 6) + "; "
              + "this is equivalent to " + str(equiv_N) + " equally-weighted streamlines");
       }




      void TckFactor::output_factors (const std::string& path) const
      {
        if (size_t(coefficients.size()) != contributions.size())
          throw Exception ("Cannot output weighting factors if they have not first been estimated!");
        try {
          decltype(coefficients) weights (coefficients.size());
          for (SIFT::track_t i = 0; i != num_tracks(); ++i)
            weights[i] = (coefficients[i] == min_coeff || !std::isfinite(coefficients[i])) ?
                         0.0 :
                         std::exp (coefficients[i]);
          save_vector (weights, path);
        } catch (...) {
          WARN ("Unable to assign memory for output factor file: \"" + Path::basename(path) + "\" not created");
        }
      }


      void TckFactor::output_coefficients (const std::string& path) const
      {
        save_vector (coefficients, path);
      }






      void TckFactor::output_TD_images (const std::string& dirpath, const std::string& origTD_path, const std::string& count_path)
      {
        Header H (MR::Fixel::data_header_from_nfixels (nfixels()));
        Header H_count;
        H_count.datatype() = DataType::native (DataType::UInt32);
        Image<float>    origTD_image (Image<float>   ::create (Path::join (dirpath, origTD_path), H));
        Image<uint32_t> count_image  (Image<uint32_t>::create (Path::join (dirpath, count_path), H));
        for (auto l = Loop(0) (origTD_image, count_image); l; ++l) {
          const size_t index = count_image.index(0);
          Fixel fixel (*this, index);
          origTD_image.value() = fixel.orig_TD();
          count_image.value() = fixel.count();
        }
      }




      void TckFactor::output_all_debug_images (const std::string& dirpath, const std::string& prefix)
      {

        Model::output_all_debug_images (dirpath, prefix);

        if (!coefficients.size())
          return;

        // TODO Change to Eigen::Array
        vector<value_type> mins   (nfixels(), std::numeric_limits<value_type>::infinity());
        vector<value_type> stdevs (nfixels(), 0.0);
        vector<value_type> maxs   (nfixels(), -std::numeric_limits<value_type>::infinity());
        vector<size_t>     zeroed (nfixels(), 0);

        {
          ProgressBar progress ("Generating streamline coefficient statistic images", num_tracks());
          for (SIFT::track_t i = 0; i != num_tracks(); ++i) {
            const value_type coeff = coefficients[i];
            const SIFT::TrackContribution& this_contribution (*contributions[i]);
            if (coeff > min_coeff) {
              for (size_t j = 0; j != this_contribution.dim(); ++j) {
                const MR::Fixel::index_type fixel_index = this_contribution[j].get_fixel_index();
                const Fixel fixel (*this, fixel_index);
                const value_type mean_coeff = fixel.mean_coeff();
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

        for (MR::Fixel::index_type i = 0; i != nfixels(); ++i) {
          Fixel fixel (*this, i);
          if (!std::isfinite (mins[i]))
            mins[i] = std::numeric_limits<value_type>::quiet_NaN();
          stdevs[i] = (fixel.count() > 1) ? (std::sqrt (stdevs[i] / value_type(fixel.count() - 1))) : 0.0;
          if (!std::isfinite (maxs[i]))
            maxs[i] = std::numeric_limits<value_type>::quiet_NaN();
        }

        Header H (MR::Fixel::data_header_from_nfixels (nfixels()));
        Header H_excluded (H);
        H_excluded.datatype() = DataType::Bit;
        Image<float> min_image      (Image<float>::create (Path::join (dirpath, prefix + "_coeff_min.mif"), H));
        Image<float> mean_image     (Image<float>::create (Path::join (dirpath, prefix + "_coeff_mean.mif"), H));
        Image<float> stdev_image    (Image<float>::create (Path::join (dirpath, prefix + "_coeff_stdev.mif"), H));
        Image<float> max_image      (Image<float>::create (Path::join (dirpath, prefix + "_coeff_max.mif"), H));
        Image<float> zeroed_image   (Image<float>::create (Path::join (dirpath, prefix + "_coeff_zeroed.mif"), H));
        Image<bool>  excluded_image (Image<bool> ::create (Path::join (dirpath, prefix + "_excludedfixels.mif"), H_excluded));

        // TODO If Eigen::Array is used, might be possible to use .row() here
        for (auto l = Loop(0) (min_image, mean_image, stdev_image, max_image, zeroed_image, excluded_image); l; ++l) {
          const MR::Fixel::index_type index = min_image.index(0);
          const Fixel fixel (*this, index);
          min_image.value() = mins[index];
          mean_image.value() = fixel.mean_coeff();
          stdev_image.value() = stdevs[index];
          max_image.value() = maxs[index];
          zeroed_image.value() = zeroed[index];
          excluded_image.value() = fixel.excluded();
        }

      }



      }
    }
  }
}



