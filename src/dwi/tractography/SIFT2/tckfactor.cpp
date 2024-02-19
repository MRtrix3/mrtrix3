/* Copyright (c) 2008-2024 the MRtrix3 contributors.
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

#include "file/matrix.h"
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

void TckFactor::set_reg_lambdas(const double lambda_tikhonov, const double lambda_tv) {
  assert(num_tracks());
  double A = 0.0;
  for (size_t i = 1; i != fixels.size(); ++i)
    A += fixels[i].get_weight() * Math::pow2(fixels[i].get_FOD());

  A /= double(num_tracks());
  INFO("Constant A scaling regularisation terms to match data term is " + str(A));
  reg_multiplier_tikhonov = lambda_tikhonov * A;
  reg_multiplier_tv = lambda_tv * A;
}

void TckFactor::store_orig_TDs() {
  for (std::vector<Fixel>::iterator i = fixels.begin(); i != fixels.end(); ++i)
    i->store_orig_TD();
}

void TckFactor::remove_excluded_fixels(const float min_td_frac) {
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
      zero_TD_cf_sum += i->get_cost(fixed_mu);
    } else if ((fixed_mu * i->get_orig_TD() < min_td_frac * i->get_FOD()) || (i->get_count() == 1)) {
      i->exclude();
      ++excluded_count;
      excluded_cf_sum += i->get_cost(fixed_mu);
    }
  }
  INFO(str(zero_TD_count) + " fixels have no attributed streamlines; these account for " +
       str(100.0 * zero_TD_cf_sum / cf) + "\% of the initial cost function");
  if (excluded_count) {
    INFO(str(excluded_count) + " of " + str(fixels.size()) +
         " fixels were tracked, but have been excluded from optimisation due to inadequate reconstruction;");
    INFO("these contribute " + str(100.0 * excluded_cf_sum / cf) + "\% of the initial cost function");
  } else if (min_td_frac) {
    INFO("No fixels were excluded from optimisation due to poor reconstruction");
  }
}

void TckFactor::test_streamline_length_scaling() {
  VAR(calc_cost_function());

  for (std::vector<Fixel>::iterator i = fixels.begin(); i != fixels.end(); ++i)
    i->clear_TD();

  coefficients.resize(num_tracks(), 0.0);
  TD_sum = 0.0;

  for (SIFT::track_t track_index = 0; track_index != num_tracks(); ++track_index) {
    const SIFT::TrackContribution &tck_cont(*contributions[track_index]);
    const double weight = 1.0 / tck_cont.get_total_length();
    coefficients[track_index] = std::log(weight);
    for (size_t i = 0; i != tck_cont.dim(); ++i)
      fixels[tck_cont[i].get_fixel_index()] += weight * tck_cont[i].get_length();
    TD_sum += weight * tck_cont.get_total_contribution();
  }

  VAR(calc_cost_function());

  // Also test varying mu; produce a scatter plot
  const double actual_TD_sum = TD_sum;
  std::ofstream out("mu.csv", std::ios_base::trunc);
  for (int i = -1000; i != 1000; ++i) {
    const double factor = std::pow(10.0, double(i) / 1000.0);
    TD_sum = factor * actual_TD_sum;
    out << str(factor) << "," << str(calc_cost_function()) << "\n";
  }
  out << "\n";
  out.close();

  TD_sum = actual_TD_sum;
}

void TckFactor::calc_afcsa() {

  CONSOLE("Cost function before linear optimisation is " + str(calc_cost_function()) + ")");

  try {
    coefficients = decltype(coefficients)::Zero(num_tracks());
  } catch (...) {
    throw Exception("Error assigning memory for streamline weights vector");
  }

  class Functor {
  public:
    Functor(TckFactor &master) : master(master), fixed_mu(master.mu()) {}
    Functor(const Functor &) = default;
    bool operator()(const SIFT::TrackIndexRange &range) const {
      for (SIFT::track_t track_index = range.first; track_index != range.second; ++track_index) {
        const SIFT::TrackContribution &tckcont = *master.contributions[track_index];
        double sum_afd = 0.0;
        for (size_t f = 0; f != tckcont.dim(); ++f) {
          const size_t fixel_index = tckcont[f].get_fixel_index();
          const Fixel &fixel = master.fixels[fixel_index];
          const float length = tckcont[f].get_length();
          sum_afd += fixel.get_weight() * fixel.get_FOD() * (length / fixel.get_orig_TD());
        }
        if (sum_afd && tckcont.get_total_contribution()) {
          const double afcsa = sum_afd / tckcont.get_total_contribution();
          master.coefficients[track_index] = std::max(master.min_coeff, std::log(afcsa / fixed_mu));
        } else {
          master.coefficients[track_index] = master.min_coeff;
        }
      }
      return true;
    }

  private:
    TckFactor &master;
    const double fixed_mu;
  };
  {
    SIFT::TrackIndexRangeWriter writer(SIFT_TRACK_INDEX_BUFFER_SIZE, num_tracks());
    Functor functor(*this);
    Thread::run_queue(writer, SIFT::TrackIndexRange(), Thread::multi(functor));
  }

  for (std::vector<Fixel>::iterator i = fixels.begin(); i != fixels.end(); ++i) {
    i->clear_TD();
    i->clear_mean_coeff();
  }
  {
    SIFT::TrackIndexRangeWriter writer(SIFT_TRACK_INDEX_BUFFER_SIZE, num_tracks());
    FixelUpdater worker(*this);
    Thread::run_queue(writer, SIFT::TrackIndexRange(), Thread::multi(worker));
  }

  CONSOLE("Cost function after linear optimisation is " + str(calc_cost_function()) + ")");
}

void TckFactor::estimate_factors() {

  try {
    coefficients = decltype(coefficients)::Zero(num_tracks());
  } catch (...) {
    throw Exception("Error assigning memory for streamline weights vector");
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

  auto display_func = [&]() {
    return printf("    %5u        %3.3f%%         %2.3f%%        %u",
                  iter,
                  100.0 * cf_data / init_cf,
                  100.0 * cf_reg / init_cf,
                  nonzero_streamlines);
  };
  CONSOLE("         Iteration     CF (data)       CF (reg)    Streamlines");
  ProgressBar progress("");

  // Keep track of total exclusions, not just how many are removed in each iteration
  size_t total_excluded = 0;
  for (size_t i = 1; i != fixels.size(); ++i) {
    if (fixels[i].is_excluded())
      ++total_excluded;
  }

  std::unique_ptr<std::ofstream> csv_out;
  if (!csv_path.empty()) {
    csv_out.reset(new std::ofstream());
    csv_out->open(csv_path.c_str(), std::ios_base::trunc);
    (*csv_out)
        << "Iteration,Cost_data,Cost_reg_tik,Cost_reg_tv,Cost_reg,Cost_total,Streamlines,Fixels_excluded,Step_min,Step_"
           "mean,Step_mean_abs,Step_var,Step_max,Coeff_min,Coeff_mean,Coeff_mean_abs,Coeff_var,Coeff_max,Coeff_norm,\n";
    (*csv_out) << "0," << init_cf << ",0,0,0," << init_cf << "," << nonzero_streamlines << "," << total_excluded
               << ",0,0,0,0,0,0,0,0,0,0,0,\n";
    csv_out->flush();
  }

  // Initial estimates of how each weighting coefficient is going to change
  // The ProjectionCalculator classes overwrite these in place, so do an initial allocation but
  //   don't bother wiping it at every iteration
  // std::vector<float> projected_steps (num_tracks(), 0.0);

  // Logging which fixels need to be excluded from optimisation in subsequent iterations,
  //   due to driving streamlines to unwanted high weights
  BitSet fixels_to_exclude(fixels.size());

  do {

    ++iter;
    prev_cf = new_cf;

    // Line search to optimise each coefficient
    StreamlineStats step_stats, coefficient_stats;
    nonzero_streamlines = 0;
    fixels_to_exclude.clear();
    double sum_costs = 0.0;
    {
      SIFT::TrackIndexRangeWriter writer(SIFT_TRACK_INDEX_BUFFER_SIZE, num_tracks());
      // CoefficientOptimiserGSS worker (*this, /*projected_steps,*/ step_stats, coefficient_stats, nonzero_streamlines,
      // fixels_to_exclude, sum_costs); CoefficientOptimiserQLS worker (*this, /*projected_steps,*/ step_stats,
      // coefficient_stats, nonzero_streamlines, fixels_to_exclude, sum_costs);
      CoefficientOptimiserIterative worker(
          *this, /*projected_steps,*/ step_stats, coefficient_stats, nonzero_streamlines, fixels_to_exclude, sum_costs);
      Thread::run_queue(writer, SIFT::TrackIndexRange(), Thread::multi(worker));
    }
    step_stats.normalise();
    coefficient_stats.normalise();
    indicate_progress();

    // Perform fixel exclusion
    const size_t excluded_count = fixels_to_exclude.count();
    if (excluded_count) {
      DEBUG(str(excluded_count) + " fixels excluded this iteration");
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
      SIFT::TrackIndexRangeWriter writer(SIFT_TRACK_INDEX_BUFFER_SIZE, num_tracks());
      FixelUpdater worker(*this);
      Thread::run_queue(writer, SIFT::TrackIndexRange(), Thread::multi(worker));
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
      SIFT::TrackIndexRangeWriter writer(SIFT_TRACK_INDEX_BUFFER_SIZE, num_tracks());
      RegularisationCalculator worker(*this, cf_reg_tik, cf_reg_tv);
      Thread::run_queue(writer, SIFT::TrackIndexRange(), Thread::multi(worker));
    }
    cf_reg_tik *= reg_multiplier_tikhonov;
    cf_reg_tv *= reg_multiplier_tv;

    cf_reg = cf_reg_tik + cf_reg_tv;

    new_cf = cf_data + cf_reg;

    if (!csv_path.empty()) {
      (*csv_out) << str(iter) << "," << str(cf_data) << "," << str(cf_reg_tik) << "," << str(cf_reg_tv) << ","
                 << str(cf_reg) << "," << str(new_cf) << "," << str(nonzero_streamlines) << "," << str(total_excluded)
                 << "," << str(step_stats.get_min()) << "," << str(step_stats.get_mean()) << ","
                 << str(step_stats.get_mean_abs()) << "," << str(step_stats.get_var()) << ","
                 << str(step_stats.get_max()) << "," << str(coefficient_stats.get_min()) << ","
                 << str(coefficient_stats.get_mean()) << "," << str(coefficient_stats.get_mean_abs()) << ","
                 << str(coefficient_stats.get_var()) << "," << str(coefficient_stats.get_max()) << ","
                 << str(coefficient_stats.get_var() * (num_tracks() - 1)) << ",\n";
      csv_out->flush();
    }

    progress.update(display_func);

    // Leaving out testing the fixel exclusion mask criterion; doesn't converge, and results in CF increase
  } while (((new_cf - prev_cf < required_cf_change) || (iter < min_iters) /* || !fixels_to_exclude.empty() */) &&
           (iter < max_iters));
}

void TckFactor::report_entropy() const {
  // H = - <sum_i> P(x_i) log_2 (P(x_i))
  //
  // Before SIFT2:
  // All streamlines have P(x_i) = 1.0 / num_streamlines
  const default_type P_before = 1.0 / coefficients.size();
  const default_type logP_before = std::log2(P_before);
  const default_type H_before = -coefficients.size() * (P_before * logP_before);
  // After SIFT2:
  // - First, need normalising factor, which is the reciprocal sum of all streamline weights
  //   (as opposed to the reciprocal number of streamlines)
  default_type sum_weights = 0.0;
  for (ssize_t i = 0; i != coefficients.size(); ++i)
    sum_weights += std::exp(coefficients[i]);
  const default_type inv_sum_weights = 1.0 / sum_weights;
  default_type H_after = 0.0;
  for (ssize_t i = 0; i != coefficients.size(); ++i) {
    const default_type P_after = std::exp(coefficients[i]) * inv_sum_weights;
    const default_type logP_after = std::log2(P_after);
    H_after += P_after * logP_after;
  }
  H_after *= -1.0;
  const size_t equiv_N = std::round(std::pow(2.0, H_after));
  INFO("Entropy decreased from " + str(H_before, 6) + " to " + str(H_after, 6) + "; " + "this is equivalent to " +
       str(equiv_N) + " equally-weighted streamlines");
}

void TckFactor::output_factors(const std::string &path) const {
  if (size_t(coefficients.size()) != contributions.size())
    throw Exception("Cannot output weighting factors if they have not first been estimated!");
  decltype(coefficients) weights;
  try {
    weights.resize(coefficients.size());
  } catch (...) {
    WARN("Unable to assign memory for output factor file: \"" + Path::basename(path) + "\" not created");
    return;
  }
  for (SIFT::track_t i = 0; i != num_tracks(); ++i)
    weights[i] = (coefficients[i] == min_coeff || !std::isfinite(coefficients[i])) ? 0.0 : std::exp(coefficients[i]);
  File::Matrix::save_vector(weights, path);
}

void TckFactor::output_coefficients(const std::string &path) const { File::Matrix::save_vector(coefficients, path); }

void TckFactor::output_TD_images(const std::string &dirpath,
                                 const std::string &origTD_path,
                                 const std::string &count_path) const {
  Header H(MR::Fixel::data_header_from_nfixels(fixels.size()));
  Header H_count;
  H_count.datatype() = DataType::native(DataType::UInt32);
  Image<float> origTD_image(Image<float>::create(Path::join(dirpath, origTD_path), H));
  Image<uint32_t> count_image(Image<uint32_t>::create(Path::join(dirpath, count_path), H));
  for (auto l = Loop(0)(origTD_image, count_image); l; ++l) {
    const size_t index = count_image.index(0);
    origTD_image.value() = fixels[index].get_orig_TD();
    count_image.value() = fixels[index].get_count();
  }
}

void TckFactor::output_all_debug_images(const std::string &dirpath, const std::string &prefix) const {

  Model<Fixel>::output_all_debug_images(dirpath, prefix);

  if (!coefficients.size())
    return;

  std::vector<double> mins(fixels.size(), std::numeric_limits<double>::infinity());
  std::vector<double> stdevs(fixels.size(), 0.0);
  std::vector<double> maxs(fixels.size(), -std::numeric_limits<double>::infinity());
  std::vector<size_t> zeroed(fixels.size(), 0);

  {
    ProgressBar progress("Generating streamline coefficient statistic images", num_tracks());
    for (SIFT::track_t i = 0; i != num_tracks(); ++i) {
      const double coeff = coefficients[i];
      const SIFT::TrackContribution &this_contribution(*contributions[i]);
      if (coeff > min_coeff) {
        for (size_t j = 0; j != this_contribution.dim(); ++j) {
          const size_t fixel_index = this_contribution[j].get_fixel_index();
          const double mean_coeff = fixels[fixel_index].get_mean_coeff();
          mins[fixel_index] = std::min(mins[fixel_index], coeff);
          stdevs[fixel_index] += Math::pow2(coeff - mean_coeff);
          maxs[fixel_index] = std::max(maxs[fixel_index], coeff);
        }
      } else {
        for (size_t j = 0; j != this_contribution.dim(); ++j)
          ++zeroed[this_contribution[j].get_fixel_index()];
      }
      ++progress;
    }
  }

  for (size_t i = 1; i != fixels.size(); ++i) {
    if (!std::isfinite(mins[i]))
      mins[i] = std::numeric_limits<double>::quiet_NaN();
    stdevs[i] = (fixels[i].get_count() > 1) ? (std::sqrt(stdevs[i] / float(fixels[i].get_count() - 1))) : 0.0;
    if (!std::isfinite(maxs[i]))
      maxs[i] = std::numeric_limits<double>::quiet_NaN();
  }

  Header H(MR::Fixel::data_header_from_nfixels(fixels.size()));
  Header H_excluded(H);
  H_excluded.datatype() = DataType::Bit;
  Image<float> min_image(Image<float>::create(Path::join(dirpath, prefix + "_coeff_min.mif"), H));
  Image<float> mean_image(Image<float>::create(Path::join(dirpath, prefix + "_coeff_mean.mif"), H));
  Image<float> stdev_image(Image<float>::create(Path::join(dirpath, prefix + "_coeff_stdev.mif"), H));
  Image<float> max_image(Image<float>::create(Path::join(dirpath, prefix + "_coeff_max.mif"), H));
  Image<float> zeroed_image(Image<float>::create(Path::join(dirpath, prefix + "_coeff_zeroed.mif"), H));
  Image<bool> excluded_image(Image<bool>::create(Path::join(dirpath, prefix + "_excludedfixels.mif"), H_excluded));

  for (auto l = Loop(0)(min_image, mean_image, stdev_image, max_image, zeroed_image, excluded_image); l; ++l) {
    const size_t index = min_image.index(0);
    min_image.value() = mins[index];
    mean_image.value() = fixels[index].get_mean_coeff();
    stdev_image.value() = stdevs[index];
    max_image.value() = maxs[index];
    zeroed_image.value() = zeroed[index];
    excluded_image.value() = fixels[index].is_excluded();
  }
}

} // namespace SIFT2
} // namespace Tractography
} // namespace DWI
} // namespace MR
