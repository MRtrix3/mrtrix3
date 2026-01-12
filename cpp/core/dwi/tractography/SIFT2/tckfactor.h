/* Copyright (c) 2008-2026 the MRtrix3 contributors.
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

#pragma once

#include <fstream>
#include <limits>
#include <mutex>

#include "image.h"
#include "types.h"

#include "dwi/directions/set.h"

#include "dwi/tractography/SIFT/model.h"
#include "dwi/tractography/SIFT/output.h"

#include "dwi/tractography/SIFT2/fixel.h"

namespace MR::DWI::Tractography::SIFT2 {

constexpr default_type default_regularisation_tikhonov = 0.0;
constexpr default_type default_regularisation_tv = 0.1;
constexpr default_type default_minimum_td_fraction = 0.1;
constexpr ssize_t default_minimum_iterations = 10;
constexpr ssize_t default_maximum_iterations = 1000;
constexpr default_type default_minimum_coefficient = -std::numeric_limits<default_type>::infinity();
constexpr default_type default_maximum_coefficient = std::numeric_limits<default_type>::infinity();
constexpr default_type default_maximum_coeffstep = 1.0;
constexpr default_type default_minimum_cf_fractional_decrease = 2.5e-5;

class TckFactor : public SIFT::Model<Fixel> {

public:
  TckFactor(Image<float> &fod_image, const DWI::Directions::FastLookupSet &dirs)
      : SIFT::Model<Fixel>(fod_image, dirs),
        reg_multiplier_tikhonov(default_regularisation_tikhonov),
        reg_multiplier_tv(default_regularisation_tv),
        min_iters(default_minimum_iterations),
        max_iters(default_maximum_iterations),
        min_coeff(default_minimum_coefficient),
        max_coeff(default_maximum_coefficient),
        max_coeff_step(default_maximum_coeffstep),
        min_cf_decrease_percentage(default_minimum_cf_fractional_decrease),
        data_scale_term(0.0) {}

  void set_reg_lambdas(const double, const double);
  void set_min_iters(const int i) { min_iters = i; }
  void set_max_iters(const int i) { max_iters = i; }
  void set_min_factor(const double i) { min_coeff = i ? std::log(i) : -std::numeric_limits<double>::infinity(); }
  void set_min_coeff(const double i) { min_coeff = i; }
  void set_max_factor(const double i) { max_coeff = std::log(i); }
  void set_max_coeff(const double i) { max_coeff = i; }
  void set_max_coeff_step(const double i) { max_coeff_step = i; }
  void set_min_cf_decrease(const double i) { min_cf_decrease_percentage = i; }

  void set_csv_path(std::string_view i) { csv_path = i; }

  void store_orig_TDs();

  void remove_excluded_fixels(const float);

  // Function that prints the cost function, then sets the streamline weights according to
  //   the inverse of length, and re-calculates and prints the cost function
  void test_streamline_length_scaling();

  // AFCSA method: Sum the attributable fibre volumes along the streamline length;
  //   divide by streamline length, convert to a weighting coefficient,
  //   see how the cost function fares
  void calc_afcsa();

  void estimate_factors();

  void report_entropy() const;

  void output_factors(std::string_view) const;
  void output_coefficients(std::string_view) const;

  void output_TD_images(std::string_view, std::string_view, std::string_view) const;
  void output_all_debug_images(std::string_view, std::string_view) const;

private:
  Eigen::Array<default_type, Eigen::Dynamic, 1> coefficients;

  double reg_multiplier_tikhonov, reg_multiplier_tv;
  size_t min_iters, max_iters;
  double min_coeff, max_coeff, max_coeff_step, min_cf_decrease_percentage;
  std::string csv_path;

  double data_scale_term;

  friend class LineSearchFunctor;
  friend class CoefficientOptimiserBase;
  friend class CoefficientOptimiserGSS;
  friend class CoefficientOptimiserQLS;
  friend class CoefficientOptimiserIterative;
  friend class FixelUpdater;
  friend class RegularisationCalculator;

  // For when multiple threads are trying to write their final information back
  std::mutex mutex;

  void indicate_progress() {
    if (App::log_level)
      fprintf(stderr, ".");
  }
};

} // namespace MR::DWI::Tractography::SIFT2
