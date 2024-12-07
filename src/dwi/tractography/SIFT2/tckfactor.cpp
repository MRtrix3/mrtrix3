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
#include "dwi/tractography/SIFT2/tckfactor.h"

#include "dwi/tractography/SIFT/track_index_range.h"






namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace SIFT2 {




      // TODO Reorder .h file to reflect necessary reordering of templated functions
      template <operation_mode_t Mode>
      void TckFactor::update_fixels()
      {
        switch (Mode) {
          case operation_mode_t::ABSOLUTE:
            // Multi-threaded calculation of updated streamline density, and mean weighting coefficient, in each fixel
            fixels.col(td_column).setZero();
            fixels.col(mean_coeff_column).setZero();
            break;
          case operation_mode_t::DIFFERENTIAL:
            fixels.col(differential_td_column).setZero();
            fixels.col(mean_deltacoeff_column).setZero();
            break;
          default:
            assert (false);
        }
        {
          SIFT::TrackIndexRangeWriter writer (SIFT_TRACK_INDEX_BUFFER_SIZE, num_tracks());
          typename FixelUpdaterSelector<Mode>::type worker (*this);
          Thread::run_queue (writer, SIFT::TrackIndexRange(), Thread::multi (worker));
        }
        // // Scale the fixel mean coefficient terms (each streamline in the fixel is weighted by its length)
        // for (MR::Fixel::index_type i = 0; i != nfixels(); ++i) {
        //   Fixel fixel (*this, i);
        //   switch (Mode) {
        //     case operation_mode_t::ABSOLUTE: fixel.normalise_mean_coeff(); break;
        //     case operation_mode_t::DIFFERENTIAL: fixel.normalise_mean_deltacoeff(); break;
        //   }
        // }
        // No normalisation of a mean parameter required for differential mode
        if (Mode == operation_mode_t::ABSOLUTE) {
          for (MR::Fixel::index_type i = 0; i != nfixels(); ++i) {
            Fixel fixel (*this, i);
            fixel.normalise_mean_coeff();
          }
        } else if (Mode == operation_mode_t::DIFFERENTIAL) {
          for (MR::Fixel::index_type i = 0; i != nfixels(); ++i) {
            Fixel fixel (*this, i);
            fixel.normalise_mean_deltacoeff();
          }
        } else {
          assert (false);
        }
      }
      template void TckFactor::update_fixels<operation_mode_t::ABSOLUTE>();
      template void TckFactor::update_fixels<operation_mode_t::DIFFERENTIAL>();






      namespace
      {
        template <reg_basis_t RegBasis, reg_fn_abs_t RegFn>
        value_type calc_reg_absolute (TckFactor& master)
        {
          value_type result = value_type(0.0);
          {
            SIFT::TrackIndexRangeWriter writer (SIFT_TRACK_INDEX_BUFFER_SIZE, master.num_tracks());
            RegularisationCalculatorAbsolute<RegBasis, RegFn> worker (master, result);
            Thread::run_queue (writer, SIFT::TrackIndexRange(), Thread::multi (worker));
          }
          return result;
        }
        template <reg_fn_diff_t RegFn>
        value_type calc_reg_differential (TckFactor& master)
        {
          value_type result = value_type(0.0);
          {
            SIFT::TrackIndexRangeWriter writer (SIFT_TRACK_INDEX_BUFFER_SIZE, master.num_tracks());
            RegularisationCalculatorDifferential<RegFn> worker (master, result);
            Thread::run_queue (writer, SIFT::TrackIndexRange(), Thread::multi (worker));
          }
          return result;
        }
      }
      template <>
      value_type TckFactor::calculate_regularisation<operation_mode_t::ABSOLUTE>()
      {
        value_type unscaled = value_type(0.0);
        switch (reg_basis_abs) {
          case reg_basis_t::STREAMLINE: {
            switch (reg_fn_abs) {
              case reg_fn_abs_t::COEFF:  unscaled = calc_reg_absolute<reg_basis_t::STREAMLINE, reg_fn_abs_t::COEFF>  (*this); break;
              case reg_fn_abs_t::FACTOR: unscaled = calc_reg_absolute<reg_basis_t::STREAMLINE, reg_fn_abs_t::FACTOR> (*this); break;
              case reg_fn_abs_t::GAMMA:  unscaled = calc_reg_absolute<reg_basis_t::STREAMLINE, reg_fn_abs_t::GAMMA>  (*this); break;
            }
            break;
          }
          case reg_basis_t::FIXEL: {
            switch (reg_fn_abs) {
              case reg_fn_abs_t::COEFF:  unscaled = calc_reg_absolute<reg_basis_t::FIXEL, reg_fn_abs_t::COEFF>  (*this); break;
              case reg_fn_abs_t::FACTOR: unscaled = calc_reg_absolute<reg_basis_t::FIXEL, reg_fn_abs_t::FACTOR> (*this); break;
              case reg_fn_abs_t::GAMMA:  unscaled = calc_reg_absolute<reg_basis_t::FIXEL, reg_fn_abs_t::GAMMA>  (*this); break;
            }
            break;
          }
        }
        assert (std::isfinite(unscaled));
        assert (std::isfinite(reg_multiplier_abs));
        return unscaled * reg_multiplier_abs;
      }
      template <>
      value_type TckFactor::calculate_regularisation<operation_mode_t::DIFFERENTIAL>()
      {
        value_type unscaled = value_type(0.0);
        switch (reg_fn_diff) {
          case reg_fn_diff_t::ASYMPTOTIC: unscaled = calc_reg_differential<reg_fn_diff_t::ASYMPTOTIC> (*this); break;
          case reg_fn_diff_t::DELTACOEFF: unscaled = calc_reg_differential<reg_fn_diff_t::DELTACOEFF> (*this); break;
        }
        assert (std::isfinite(unscaled));
        assert (std::isfinite(reg_multiplier_diff));
        return unscaled * reg_multiplier_diff;
      }



      void TckFactor::set_csv_path (const std::string& path)
      {
        csv_path = path;
        File::OFStream csv_out (path);
        csv_out << "Iteration,Cost_data,Cost_reg,Cost_total,Streamlines,Fixels_excluded,Step_min,Step_mean,Step_mean_abs,Step_var,Step_max,Coeff_min,Coeff_mean,Coeff_mean_abs,Coeff_var,Coeff_max,Coeff_norm,\n";
      }



      void TckFactor::set_coefficients (const std::string& path)
      {
        coefficients = load_vector<value_type> (path);
        if (coefficients.size() != contributions.size())
          throw Exception ("Number of entries in input weighting coefficients file \"" + path + "\" (" + str(coefficients.size()) + ")"
                           + "does not match number of streamlines read (" + str(contributions.size()) + ")");
        if (mask_absolute.size() == 0)
          mask_absolute = Eigen::Array<bool, Eigen::Dynamic, 1>::Ones(num_tracks());
        SIFT::track_t inclusion_count = 0;
        SIFT::track_t exclusion_count = 0;
        for (SIFT::track_t index = 0; index != num_tracks(); ++index) {
          if (coefficients[index] == -std::numeric_limits<value_type>::infinity()) {
            if (mask_absolute[index])
              ++exclusion_count;
            mask_absolute[index] = false;
          } else if (coefficients[index] == std::numeric_limits<value_type>::infinity()) {
            throw Exception("Input coefficients file \"" + path + "\" contains infinity; check derivation");
          } else if (std::isnan(coefficients[index])) {
            coefficients[index] = -std::numeric_limits<value_type>::infinity();
            if (mask_absolute[index])
              ++exclusion_count;
            mask_absolute[index] = false;
          } else if (mask_absolute[index]) {
            ++inclusion_count;
          }
        }
        if (exclusion_count > 0) {
          INFO(str(exclusion_count) + " streamlines excluded from absolute mode optimisation due to non-finite coefficients imported from \"" + path + "\"");
          INFO(str(inclusion_count) + " remaining streamlines will participate in optimisation");
        }
        update_fixels<operation_mode_t::ABSOLUTE>();
      }



      void TckFactor::set_factors (const std::string& path)
      {
        const Eigen::Array<value_type, Eigen::Dynamic, 1> factors = load_vector<value_type> (path);
        if (factors.size() != contributions.size())
          throw Exception ("Number of entries in input weighting factors file \"" + path + "\" (" + str(coefficients.size()) + ")"
                           + "does not match number of streamlines read (" + str(contributions.size()) + ")");
        coefficients.resize (factors.size());
        if (mask_absolute.size() == 0)
          mask_absolute = Eigen::Array<bool, Eigen::Dynamic, 1>::Ones(factors.size());
        SIFT::track_t inclusion_count = 0;
        SIFT::track_t exclusion_count = 0;
        for (SIFT::track_t i = 0; i != factors.size(); ++i) {
          if (factors[i] == -std::numeric_limits<value_type>::infinity() || factors[i] == std::numeric_limits<value_type>::infinity())
            throw Exception("Input weighting factors file \"" + path + "\" contains infinity; check derivation");
          if (factors[i] < 0.0)
            throw Exception("Input weighting factors file \"" + path + "\" contains negative value; check derivation");
          if (std::isnan(factors[i]) || factors[i] == value_type(0)) {
            coefficients[i] = -std::numeric_limits<value_type>::infinity();
            if (mask_absolute[i])
              ++exclusion_count;
            mask_absolute[i] = false;
          } else {
            coefficients[i] = std::log (factors[i]);
            ++inclusion_count;
          }
        }
        if (exclusion_count > 0) {
          INFO(str(exclusion_count) + " streamlines excluded from absolute mode optimisation due to zero / NaN weighting factors imported from \"" + path + "\"");
          INFO(str(inclusion_count) + " remaining streamlines will participate in optimisation");
        }
        update_fixels<operation_mode_t::ABSOLUTE>();
      }




      void TckFactor::set_deltacoeffs (const std::string &path) {
        deltacoeffs = load_vector<value_type> (path);
        if (deltacoeffs.size() != contributions.size())
          throw Exception (std::string("Number of entries in input delta coefficients file")
                           + " \"" + path + "\" (" + str(coefficients.size()) + ")"
                           + " does not match number of streamlines read"
                           + " (" + str(contributions.size()) + ")");
        if (mask_differential.size() == 0)
          mask_differential = Eigen::Array<bool, Eigen::Dynamic, 1>::Ones(deltacoeffs.size());
        SIFT::track_t inclusion_count = 0;
        SIFT::track_t exclusion_count = 0;
        for (SIFT::track_t i = 0; i != deltacoeffs.size(); ++i) {
          if (deltacoeffs[i] == -std::numeric_limits<value_type>::infinity()
              || deltacoeffs[i] == std::numeric_limits<value_type>::infinity())
            throw Exception("Input delta coefficients file \"" + path + "\" contains infinity; "
                            "check derivation");
          if (deltacoeffs[i] > value_type(1))
            throw Exception("Input delta coefficients file \"" + path + "\" contains values greater than 1.0; "
                            "check derivation");
          if (deltacoeffs[i] < value_type(-1))
            throw Exception("Input delta coefficients file \"" + path + "\" contains values less than -1.0; "
                            "check derivation");
          if (std::isnan(deltacoeffs[i])) {
            if (mask_differential[i])
              ++exclusion_count;
            mask_differential[i] = false;
            // Interpret NaN -> Removal of streamline
            deltacoeffs[i] = -1.0;
          } else {
            ++inclusion_count;
          }
        }
        if (exclusion_count > 0) {
          INFO(str(exclusion_count) + " streamlines excluded from differential mode optimisation "
               "due to NaN delta coefficients imported from \"" + path + "\"");
          INFO(str(inclusion_count) + " remaining streamlines will participate in optimisation");
        }
        update_fixels<operation_mode_t::DIFFERENTIAL>();
      }

      void TckFactor::set_deltafactors (const std::string &path) {
        deltacoeffs = load_vector<value_type> (path);
        if (deltacoeffs.size() != contributions.size())
          throw Exception (std::string("Number of entries in input delta factors file")
                           + " \"" + path + "\" (" + str(coefficients.size()) + ")"
                           + " does not match number of streamlines read"
                           + " (" + str(contributions.size()) + ")");
        if (mask_differential.size() == 0)
          mask_differential = Eigen::Array<bool, Eigen::Dynamic, 1>::Ones(deltacoeffs.size());
        SIFT::track_t inclusion_count = 0;
        SIFT::track_t exclusion_count = 0;
        for (SIFT::track_t i = 0; i != deltacoeffs.size(); ++i) {
          if (deltacoeffs[i] == -std::numeric_limits<value_type>::infinity()
              || deltacoeffs[i] == std::numeric_limits<value_type>::infinity())
            throw Exception("Input delta factors file \"" + path + "\" contains infinity; "
                            "check derivation");
          auto WCF (WeightingCoeffAndFactor::from_coeff(coefficients[i]));
          if (std::abs(deltacoeffs[i]) > WCF.factor())
            throw Exception("Input delta coefficients file \"" + path + "\" "
                            "contains values greater in magnitude that absolute mode factor; "
                            "check derivation");
          if (deltacoeffs[i] < value_type(-1))
            throw Exception("Input delta coefficients file \"" + path + "\" contains values less than -1.0; "
                            "check derivation");
          if (std::isnan(deltacoeffs[i])) {
            if (mask_differential[i])
              ++exclusion_count;
            mask_differential[i] = false;
            // Interpret NaN -> Removal of streamline
            deltacoeffs[i] = -1.0;
          } else {
            ++inclusion_count;
            // Transform from "delta factor" to "delta coefficient"
            deltacoeffs[i] /= WCF.factor();
          }
        }
        if (exclusion_count > 0) {
          INFO(str(exclusion_count) + " streamlines excluded from differential mode optimisation "
               "due to NaN delta factors imported from \"" + path + "\"");
          INFO(str(inclusion_count) + " remaining streamlines will participate in optimisation");
        }

        update_fixels<operation_mode_t::DIFFERENTIAL>();
      }



      template <>
      void TckFactor::enforce_limits<operation_mode_t::ABSOLUTE>()
      {
        if (min_coeff == -std::numeric_limits<value_type>::infinity() && max_coeff == std::numeric_limits<value_type>::infinity())
          return;
        SIFT::track_t removed_count = 0, clamped_count = 0;
        for (SIFT::track_t i = 0; i != num_tracks(); ++i) {
          if (coefficients[i] < min_coeff) {
            coefficients[i] = -std::numeric_limits<value_type>::infinity();
            ++removed_count;
          } else if (coefficients[i] > max_coeff) {
            coefficients[i] = max_coeff;
            ++clamped_count;
          }
        }
        if (removed_count) {
          INFO (str(removed_count) + " streamlines were removed due to initial weights being lower than minimum permissible");
        }
        if (clamped_count) {
          INFO (str(clamped_count) + " streamlines had their initial weight reduced due to exceeding the maximum permissible");
        }
      }



      template <>
      void TckFactor::enforce_limits<operation_mode_t::DIFFERENTIAL>()
      {
        if (min_deltacoeff == -std::numeric_limits<value_type>::infinity() && max_deltacoeff == std::numeric_limits<value_type>::infinity())
          return;
        SIFT::track_t clamped_count = 0;
        for (SIFT::track_t i = 0; i != num_tracks(); ++i) {
          if (coefficients[i] < -min_deltacoeff) {
            coefficients[i] = -min_deltacoeff;
            ++clamped_count;
          } else if (coefficients[i] > max_deltacoeff) {
            coefficients[i] = max_deltacoeff;
            ++clamped_count;
          }
        }
        if (clamped_count) {
          INFO (str(clamped_count) + " streamlines had their initial delta coefficients reduced in magnitude "
                "due to exceeding the maximum permissible");
        }
      }



      template <>
      void TckFactor::set_mask<operation_mode_t::ABSOLUTE> (const std::string &path) {
        mask_absolute = load_vector<bool>(path);
        if (mask_absolute.size() != contributions.size())
          throw Exception ("Number of entries in streamline mask for absolute mode \"" + path + "\" (" + str(mask_absolute.size()) + ")"
                           + "does not match number of streamlines read (" + str(contributions.size()) + ")");
        INFO(str(mask_absolute.count()) + " of " + str(contributions.size()) + " streamlines present in mask for absolute mode");
      }

      template <>
      void TckFactor::set_mask<operation_mode_t::DIFFERENTIAL> (const std::string &path) {
        mask_differential = load_vector<bool>(path);
        if (mask_differential.size() != contributions.size())
          throw Exception ("Number of entries in streamline mask for differential mode \"" + path + "\" (" + str(mask_differential.size()) + ")"
                           + "does not match number of streamlines read (" + str(contributions.size()) + ")");
        INFO(str(mask_differential.count()) + " of " + str(contributions.size()) + " streamlines present in mask for differential mode");
      }



      void TckFactor::import_differential_data (const std::string& path)
      {
        fixels.conservativeResizeLike (data_matrix_type::Zero (nfixels(), 10));
        Image<float> diff_fd_image (Image<float>::open (path));
        MR::Fixel::check_data_file (diff_fd_image, nfixels());
        fixels.col(differential_fd_column) = diff_fd_image.row(0);
        deltacoeffs = Eigen::Array<value_type, Eigen::Dynamic, 1>::Zero(num_tracks());
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
        SIFT::track_t low_TD_count = 0, zero_TD_count = 0, nonfinite_FD_count = 0;
        value_type zero_TD_cf_sum = 0.0, excluded_cf_sum = 0.0;
        for (MR::Fixel::index_type i = 0; i != nfixels(); ++i) {
          Fixel fixel (*this, i);
          if (fixel.weight()) {
            if (!std::isfinite(fixel.fd())) {
              fixel.exclude();
              ++nonfinite_FD_count;
            } else if (!fixel.orig_TD()) {
              ++zero_TD_count;
              zero_TD_cf_sum += fixel.get_cost (fixed_mu);
            } else if ((fixed_mu * fixel.orig_TD() < min_td_frac * fixel.fd()) || (fixel.count() < 2)) {
              fixel.exclude();
              ++low_TD_count;
              excluded_cf_sum += fixel.get_cost (fixed_mu);
            }
          }
        }
        if (nonfinite_FD_count) {
          WARN(str(nonfinite_FD_count) + " fixels have non-finite FD values; "
               "model weights of these fixels set to zero");
        }
        if (zero_TD_count) {
          INFO (str(zero_TD_count) + " fixels have no attributed streamlines; "
                "these account for " + str(100.0 * zero_TD_cf_sum / cf) + "\% of the initial cost function");
        }
        if (low_TD_count) {
          INFO (str(low_TD_count) + " of " + str(fixels.size()) + " fixels were tracked, "
                "but have been excluded from optimisation due to inadequate reconstruction; "
                "these contribute " + str (100.0 * excluded_cf_sum / cf) + "\% of the initial cost function");
        } else {
          INFO ("No fixels needed to be excluded from optimisation due to poor reconstruction");
        }
      }



      void TckFactor::calibrate_regularisation()
      {
        assert (num_tracks());
        value_type A = 0.0;
        for (MR::Fixel::index_type i = 0; i != nfixels(); ++i) {
          Fixel fixel (*this, i);
          if (fixel.weight())
            A += fixel.weight() * Math::pow2 (fixel.fd());
        }
        reg_scaling = A / value_type(num_tracks());
        // Cannot use one-liner as some fixels may contain NaN values
        // TODO Consider substituting NaNs with 0.0 at time of import
        //reg_scaling = fixels.col(model_weight_column).matrix().dot(fixels.col(fd_column).square().matrix()) / value_type(num_tracks());
        assert (std::isfinite(reg_scaling));
        INFO ("Constant A scaling regularisation term to match data term is " + str(reg_scaling));
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
        update_dynamic_mu();

        VAR (calc_cost_function());

        // Also test varying mu; produce a scatter plot
        const value_type actual_TD_sum = TD_sum;
        {
          std::ofstream out ("mu.csv", std::ios_base::trunc);
          for (int i = -1000; i != 1000; ++i) {
            const value_type factor = std::pow (10.0, value_type(i) / 1000.0);
            TD_sum = factor * actual_TD_sum;
            update_dynamic_mu();
            out << str(factor) << "," << str(calc_cost_function()) << "\n";
          }
          out << "\n";
        }

        TD_sum = actual_TD_sum;
        update_dynamic_mu();
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
          FixelUpdaterAbsolute worker (*this);
          Thread::run_queue (writer, SIFT::TrackIndexRange(), Thread::multi (worker));
        }

        CONSOLE ("Cost function after linear optimisation is " + str(calc_cost_function()) + ")");

      }




      value_type TckFactor::calc_cost_function_differential()
      {
        const value_type current_mu = mu();
        value_type cost = value_type(0);
        for (size_t i = 0; i != nfixels(); ++i)
          cost += Fixel (*this, i).differential_cost (current_mu);
        return cost;
      }




      void TckFactor::save_naive_cf()
      {
        naive_cf = calc_cost_function();
      }





      // TODO Facilitate calculation of data cost function based on Mode template parameter
      template <operation_mode_t Mode>
      void TckFactor::estimate_weights()
      {
        auto allocate_vector = [] (Eigen::Array<value_type, Eigen::Dynamic, 1>& vector, const ssize_t size)
        {
          if (vector.size() == 0) {
            try {
              vector = Eigen::Array<value_type, Eigen::Dynamic, 1>::Zero (size);
            } catch (...) {
              throw Exception ("Error assigning memory for streamline weights vector");
            }
          }
        };
        auto allocate_mask = [] (Eigen::Array<bool, Eigen::Dynamic, 1>& mask, const ssize_t size)
        {
          if (mask.size() == 0) {
            try {
              mask = Eigen::Array<bool, Eigen::Dynamic, 1>::Ones (size);
            } catch (...) {
              throw Exception ("Error assigning memory for streamline mask");
            }
          }
        };

        value_type cf_data;
        switch (Mode) {
          case operation_mode_t::ABSOLUTE:
            allocate_vector (coefficients, num_tracks());
            allocate_mask (mask_absolute, num_tracks());
            cf_data = calc_cost_function();
            break;
          case operation_mode_t::DIFFERENTIAL:
            allocate_vector (deltacoeffs, num_tracks());
            allocate_mask (mask_differential, num_tracks());
            cf_data = calc_cost_function_differential();
            break;
        }
        assert(std::isfinite(cf_data));

        // Need to include regularisation in this calculation
        //   in case the weighting coefficients have been initialised to something other than zero
        value_type cf_reg = calculate_regularisation<Mode>();
        assert(std::isfinite(cf_reg));
        const value_type init_cf = cf_data + cf_reg;
        value_type new_cf = init_cf;
        value_type prev_cf = init_cf;
        assert (std::isfinite(naive_cf));
        const value_type required_cf_change = -min_cf_decrease_percentage * naive_cf;

        Eigen::Array<bool, Eigen::Dynamic, 1> &mask (Mode == operation_mode_t::ABSOLUTE ? mask_absolute : mask_differential);
        SIFT::track_t participating_streamlines = 0;
        for (SIFT::track_t i = 0; i != num_tracks(); ++i) {
          if (contributions[i] && contributions[i]->dim() && std::isfinite (coefficients[i]) && mask[i])
            ++participating_streamlines;
        }

        unsigned int iter = 0;

        auto display_func = [&](){ return printf("    %5u        %3.3f%%         %2.3f%%        %u", iter, 100.0 * cf_data / init_cf, 100.0 * cf_reg / init_cf, participating_streamlines); };
        CONSOLE ("         Iteration     CF (data)       CF (reg)    Streamlines");
        ProgressBar progress ("");

        // Keep track of total fixel exclusions, not just how many are removed in each iteration
        MR::Fixel::index_type total_excluded = fixels.col(exclude_column).sum();

        std::unique_ptr<std::ofstream> csv_out;
        if (!csv_path.empty()) {
          csv_out.reset (new std::ofstream());
          // TODO If running in combined absolute & differential mode, will need to handle this file differently
          // TODO Consider adding weight vector entropy to CSV output in absolute mode
          csv_out->open (csv_path.c_str(), std::ios_base::app);
          (*csv_out) << "0," << cf_data << "," << cf_reg << "," << init_cf << "," << participating_streamlines << "," << total_excluded << ",0,0,0,0,0,0,0,0,0,0,0,\n";
          csv_out->flush();
        }

        // Initial estimates of how each weighting coefficient is going to change
        // The ProjectionCalculator classes overwrite these in place, so do an initial allocation but
        //   don't bother wiping it at every iteration
        //vector<float> projected_steps (num_tracks(), 0.0);
        // TODO Rather than this approach, quantify fixel-wise statistics on fractional correction,
        //   and reduce a global multiplier on the projection terms if possible

        // Logging which fixels need to be excluded from optimisation in subsequent iterations,
        //   due to driving streamlines to unwanted high weights
        // TODO Should fixel exclusion be omitted completely when processing in differential mode?
        // TODO Alternatively, could have different criteria by which to exclude fixels from differential optimisation
        BitSet fixels_to_exclude (nfixels());




        // TODO Implement BBGD (manually)
#ifdef SIFT2_USE_BBGD
        // TODO Additional tracking of prior coefficients and cost function gradients
        //   as these will be necessary to perform BBGD
        Eigen::Array<value_type, Eigen::Dynamic, 1> prev_coefficients (Eigen::Array<value_type, Eigen::Dynamic, 1>::Zero (num_tracks()));
        Eigen::Array<value_type, Eigen::Dynamic, 1> prev_gradients (Eigen::Array<value_type, Eigen::Dynamic, 1>::Constant (num_tracks(), std::numeric_limits<value_type>::signaling_NaN()));
        Eigen::Array<value_type, Eigen::Dynamic, 1> gradients (Eigen::Array<value_type, Eigen::Dynamic, 1>::Zero (num_tracks()));
        // TODO Better way of determining size of first step?
        value_type step_size = NaN;
        value_type next_step_size = 1.0;
#endif


        // TODO Have I tried a scheme where the magnitude of the dTD/dFs term is decreased over time?
        do {

          ++iter;
          //VAR (iter);
          prev_cf = new_cf;




          // TODO BBGD steps for start of iteration
#ifdef SIFT2_USE_BBGD
          // No attempt yet at implementing for differential mode
          assert (Mode == operation_mode_t::ABSOLUTE);
          step_size = next_step_size;
          VAR (step_size);

          // TODO Compute the step size for the iteration after this one, so that we don't have to store coefficients & gradients over 3 iterations
          if (iter > 2) {
            auto dx = coefficients - prev_coefficients;
            auto dg = gradients - prev_gradients;
            const value_type next_step_size_long = dx.matrix().squaredNorm() / dx.matrix().dot(dg.matrix());
            const value_type next_step_size_short = dx.matrix().dot(dg.matrix()) / dg.matrix().squaredNorm();
            const value_type next_step_size_geommean = std::sqrt(next_step_size_long * next_step_size_short);
            // TODO How can "next_step_size" possibly come out as negative here?
            //next_step_size = std::abs(next_step_size_geommean);
            next_step_size = std::min (2 * step_size, std::abs(next_step_size_geommean));
            //next_step_size = next_step_size_short;
            VAR (next_step_size_long);
            VAR (next_step_size_short);
            VAR (next_step_size_geommean);
            VAR (next_step_size);
          }

          prev_coefficients = coefficients;
          prev_gradients = gradients;
          gradients.fill(std::numeric_limits<value_type>::signaling_NaN());
#endif



          // Line search to optimise each coefficient
          StreamlineStats step_stats, weight_stats;
          fixels_to_exclude.clear();
          value_type sum_costs = 0.0;

#ifdef SIFT2_USE_BBGD
          participating_streamlines = 0;
          switch (reg_basis_abs) {
            case reg_basis_t::STREAMLINE: {
              switch (reg_fn_abs)
              {
                case reg_fn_abs_t::COEFF:  BBGD_update<reg_basis_t::STREAMLINE, reg_fn_abs_t::COEFF>  (gradients, step_size, step_stats, weight_stats, participating_streamlines, fixels_to_exclude, sum_costs); break;
                case reg_fn_abs_t::FACTOR: BBGD_update<reg_basis_t::STREAMLINE, reg_fn_abs_t::FACTOR> (gradients, step_size, step_stats, weight_stats, participating_streamlines, fixels_to_exclude, sum_costs); break;
                case reg_fn_abs_t::GAMMA:  BBGD_update<reg_basis_t::STREAMLINE, reg_fn_abs_t::GAMMA>  (gradients, step_size, step_stats, weight_stats, participating_streamlines, fixels_to_exclude, sum_costs); break;
              }
            }
            break;
            case reg_basis_t::FIXEL: {
              switch (reg_fn_abs)
              {
                case reg_fn_abs_t::COEFF:  BBGD_update<reg_basis_t::FIXEL, reg_fn_abs_t::COEFF>  (gradients, step_size, step_stats, weight_stats, participating_streamlines, fixels_to_exclude, sum_costs); break;
                case reg_fn_abs_t::FACTOR: BBGD_update<reg_basis_t::FIXEL, reg_fn_abs_t::FACTOR> (gradients, step_size, step_stats, weight_stats, participating_streamlines, fixels_to_exclude, sum_costs); break;
                case reg_fn_abs_t::GAMMA:  BBGD_update<reg_basis_t::FIXEL, reg_fn_abs_t::GAMMA>  (gradients, step_size, step_stats, weight_stats, participating_streamlines, fixels_to_exclude, sum_costs); break;
              }
            }
            break;
          }
          assert (gradients.allFinite());
#else
          // TODO Could reduce duplication
          switch (Mode)
          {
            case operation_mode_t::ABSOLUTE:
            {
              participating_streamlines = 0;
              SIFT::TrackIndexRangeWriter writer (SIFT_TRACK_INDEX_BUFFER_SIZE, num_tracks());
              //CoefficientOptimiserGSS worker (*this, /*projected_steps,*/ step_stats, weight_stats, participating_streamlines, fixels_to_exclude, sum_costs);
              //CoefficientOptimiserQLS worker (*this, /*projected_steps,*/ step_stats, weight_stats, participating_streamlines, fixels_to_exclude, sum_costs);
              CoefficientOptimiserIterative worker (*this, /*projected_steps,*/ step_stats, weight_stats, participating_streamlines, fixels_to_exclude, sum_costs);
              Thread::run_queue (writer, SIFT::TrackIndexRange(), Thread::multi (worker));
            }
            break;
            case operation_mode_t::DIFFERENTIAL:
            {
              SIFT::TrackIndexRangeWriter writer (SIFT_TRACK_INDEX_BUFFER_SIZE, num_tracks());
              DeltaOptimiserIterative worker (*this, step_stats, weight_stats, sum_costs);
              Thread::run_queue (writer, SIFT::TrackIndexRange(), Thread::multi (worker));
            }
            break;
          }
#endif

          step_stats.normalise();
          weight_stats.normalise();
          //std::cerr << "Step stats: " << step_stats << "\n";
          //std::cerr << "Weight stats: " << weight_stats << "\n";
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

          // Re-calculate per-fixel TD and mean coefficient
          update_fixels<Mode>();
          indicate_progress();

          // TODO Template this out
          // (SIFT1 code would need to become aware of "absolute" vs "differential" modes)
          switch (Mode) {
            case operation_mode_t::ABSOLUTE:     cf_data = calc_cost_function(); break;
            case operation_mode_t::DIFFERENTIAL: cf_data = calc_cost_function_differential(); break;
          }
          assert (std::isfinite(cf_data));

          // Calculate the cost of regularisation, given the updates to both the
          //   streamline weighting coefficients and the new fixel mean coefficients
          cf_reg = calculate_regularisation<Mode>();
          assert (std::isfinite(cf_reg));
          new_cf = cf_data + cf_reg;

          if (!csv_path.empty()) {
            (*csv_out) << str (iter) << "," << str (cf_data) << "," << str (cf_reg) << "," << str (new_cf) << "," << str (participating_streamlines) << "," << str (total_excluded) << ","
                << str (step_stats  .get_min()) << "," << str (step_stats  .get_mean()) << "," << str (step_stats  .get_mean_abs()) << "," << str (step_stats  .get_var()) << "," << str (step_stats  .get_max()) << ","
                << str (weight_stats.get_min()) << "," << str (weight_stats.get_mean()) << "," << str (weight_stats.get_mean_abs()) << "," << str (weight_stats.get_var()) << "," << str (weight_stats.get_max()) << ","
                << str (weight_stats.get_var() * (num_tracks() - 1))
                << ",\n";
            csv_out->flush();
          }

          std::cerr << "\n";
          progress.update (display_func);
          std::cerr << "\n";

          // Leaving out testing the fixel exclusion mask criterion; doesn't converge, and results in CF increase
        } while (((new_cf - prev_cf < required_cf_change) || (iter < min_iters) /* || !fixels_to_exclude.empty() */ ) && (iter < max_iters));
      }
      template void TckFactor::estimate_weights<operation_mode_t::ABSOLUTE>();
      template void TckFactor::estimate_weights<operation_mode_t::DIFFERENTIAL>();




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
        const value_type H_after = calculate_entropy();
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



      void TckFactor::output_deltacoeffs (const std::string& path) const
      {
        save_vector(deltacoeffs, path);
      }

      void TckFactor::output_deltaweights (const std::string& path) const
      {
        if (size_t(deltacoeffs.size()) != contributions.size())
          throw Exception ("Cannot output differential weighting factors if they have not first been estimated!");
        try {
          decltype(deltacoeffs) weights (deltacoeffs.size());
          for (SIFT::track_t i = 0; i != num_tracks(); ++i)
            weights[i] = (coefficients[i] == min_coeff || !std::isfinite(coefficients[i])) ?
                         0.0 :
                         std::exp (coefficients[i]) * deltacoeffs[i];
          save_vector (weights, path);
        } catch (...) {
          WARN ("Unable to assign memory for output differential factor file: \"" + Path::basename(path) + "\" not created");
        }
      }



      void TckFactor::output_differential_debug_images (const std::string& dirpath, const std::string& prefix)
      {
        vector<value_type> mins   (nfixels(), std::numeric_limits<value_type>::infinity());
        vector<value_type> stdevs (nfixels(), 0.0);
        vector<value_type> maxs   (nfixels(), -std::numeric_limits<value_type>::infinity());
        {
          ProgressBar progress ("Generating delta coefficient statistic images", num_tracks());
          for (SIFT::track_t i = 0; i != num_tracks(); ++i) {
            const value_type weighting_factor = WeightingCoeffAndFactor::from_coeff (coefficients[i]).factor();
            const value_type deltacoeff = deltacoeffs.size() ? deltacoeffs[i] : value_type(0);
            const SIFT::TrackContribution& this_contribution (*contributions[i]);
            for (size_t j = 0; j != this_contribution.dim(); ++j) {
              const MR::Fixel::index_type fixel_index = this_contribution[j].get_fixel_index();
              const Fixel fixel (*this, fixel_index);
              const value_type mean_deltacoeff = fixel.mean_deltacoeff();
              mins  [fixel_index] = std::min (mins[fixel_index], deltacoeff);
              stdevs[fixel_index] += weighting_factor * Math::pow2 (deltacoeff - mean_deltacoeff);
              maxs  [fixel_index] = std::max (maxs[fixel_index], deltacoeff);
            }
            ++progress;
          }
        }
        for (MR::Fixel::index_type i = 0; i != nfixels(); ++i) {
          Fixel fixel (*this, i);
          if (!std::isfinite (mins[i]))
            mins[i] = std::numeric_limits<value_type>::quiet_NaN();
          stdevs[i] = (fixel.td() > 1) ? (std::sqrt (stdevs[i] / value_type(fixel.td() - 1))) : 0.0;
          if (!std::isfinite (maxs[i]))
            maxs[i] = std::numeric_limits<value_type>::quiet_NaN();
        }
        Header H (MR::Fixel::data_header_from_nfixels (nfixels()));

        Image<float> min_fimage   (Image<float>::create (Path::join (dirpath, prefix + "_delta_min.mif"), H));
        Image<float> mean_fimage  (Image<float>::create (Path::join (dirpath, prefix + "_delta_mean.mif"), H));
        Image<float> stdev_fimage (Image<float>::create (Path::join (dirpath, prefix + "_delta_stdev.mif"), H));
        Image<float> max_fimage   (Image<float>::create (Path::join (dirpath, prefix + "_delta_max.mif"), H));
        Image<float> delta_fimage (Image<float>::create (Path::join (dirpath, prefix + "_delta_fixel.mif"), H));
        Image<float> diff_fimage  (Image<float>::create (Path::join (dirpath, prefix + "_delta_diff_fixel.mif"), H));
        Image<float> cost_fimage  (Image<float>::create (Path::join (dirpath, prefix + "_delta_cost_fixel.mif"), H));
        const value_type current_mu = mu();
        for (auto l = Loop(0) (min_fimage, mean_fimage, stdev_fimage, max_fimage, delta_fimage, diff_fimage, cost_fimage); l; ++l) {
          const MR::Fixel::index_type index = min_fimage.index(0);
          const Fixel fixel (*this, index);
          min_fimage.value() = mins[index];
          mean_fimage.value() = fixel.mean_deltacoeff();
          stdev_fimage.value() = stdevs[index];
          max_fimage.value() = maxs[index];
          delta_fimage.value() = fixel.differential_TD();
          diff_fimage.value() = fixel.differential_diff(current_mu);
          cost_fimage.value() = fixel.differential_cost(current_mu);
        }

        IndexImage index (*this);
        Image<float> sum_vimage        (Image<float>::create (Path::join (dirpath, prefix + "_delta_voxel.mif"), *this));
        Image<float> maxabsdiff_vimage (Image<float>::create (Path::join (dirpath, prefix + "_delta_maxabsdiff.mif"), *this));
        Image<float> diff_vimage       (Image<float>::create (Path::join (dirpath, prefix + "_delta_diff_voxel.mif"), *this));
        Image<float> cost_vimage       (Image<float>::create (Path::join (dirpath, prefix + "_delta_cost_voxel.mif"), *this));
        value_type sum, maxabsdiff, diff, cost;
        for (auto lv = Loop(index) (index, sum_vimage, maxabsdiff_vimage, diff_vimage, cost_vimage); lv; ++lv) {
          sum = maxabsdiff = diff = cost = value_type(0);
          for (auto lf : index.value()) {
            Fixel fixel (*this, lf);
            sum += fixel.differential_TD();
            maxabsdiff = std::max (maxabsdiff, std::abs(fixel.differential_diff(current_mu)));
            diff += fixel.differential_diff(current_mu);
            cost += fixel.differential_cost(current_mu);
          }
          sum_vimage.value() = sum * current_mu;
          maxabsdiff_vimage.value() = maxabsdiff;
          diff_vimage.value() = diff;
          cost_vimage.value() = cost;
        }

        File::OFStream scatter (Path::join (dirpath, prefix + "_delta_scatter.csv"));
        scatter << "# " << App::command_history_string << "\n";
        scatter << "#Delta fibre density,Delta track density (unscaled),Delta track density (scaled),Weight,\n";
        for (size_t i = 0; i != nfixels(); ++i) {
          const Fixel fixel (*this, i);
          scatter << str (fixel.differential_FD()) << "," << str (fixel.differential_TD()) << "," << str (fixel.differential_TD() * current_mu) << "," << str (fixel.weight()) << ",\n";
        }

      }



      value_type TckFactor::calculate_entropy() const
      {
        // After SIFT2:
        // - First, need normalising factor, which is the reciprocal sum of all streamline weights
        //   (as opposed to the reciprocal number of streamlines)
        value_type sum_weights = 0.0;
        for (SIFT::track_t i = 0; i != coefficients.size(); ++i)
          sum_weights += WeightingCoeffAndFactor::from_coeff (coefficients[i]).factor();
        const value_type inv_sum_weights = 1.0 / sum_weights;
        value_type H = 0.0;
        for (SIFT::track_t i = 0; i != coefficients.size(); ++i) {
          const value_type P = WeightingCoeffAndFactor::from_coeff (coefficients[i]).factor() * inv_sum_weights;
          const value_type logP = std::log2 (P);
          H += P * logP;
        }
        return -1.0 * H;
      }




      template <reg_basis_t RegBasis, reg_fn_abs_t RegFn>
      void TckFactor::BBGD_update (Eigen::Array<value_type, Eigen::Dynamic, 1>& gradients,
                                   const value_type step_size,
                                   StreamlineStats& step_stats,
                                   StreamlineStats& coefficient_stats,
                                   unsigned int& participating_streamlines,
                                   BitSet& fixels_to_exclude,
                                   value_type& sum_costs)
      {
        SIFT::TrackIndexRangeWriter writer (SIFT_TRACK_INDEX_BUFFER_SIZE, num_tracks());
        CoefficientOptimiserBBGD<RegBasis, RegFn> worker (*this, gradients, step_size, step_stats, coefficient_stats, participating_streamlines, fixels_to_exclude, sum_costs);
        Thread::run_queue (writer, SIFT::TrackIndexRange(), Thread::multi (worker));
      }
      template void TckFactor::BBGD_update<reg_basis_t::STREAMLINE, reg_fn_abs_t::COEFF> (Eigen::Array<value_type, Eigen::Dynamic, 1>&, const value_type, StreamlineStats&, StreamlineStats&, unsigned int&, BitSet&, value_type&);
      template void TckFactor::BBGD_update<reg_basis_t::STREAMLINE, reg_fn_abs_t::FACTOR> (Eigen::Array<value_type, Eigen::Dynamic, 1>&, const value_type, StreamlineStats&, StreamlineStats&, unsigned int&, BitSet&, value_type&);
      template void TckFactor::BBGD_update<reg_basis_t::STREAMLINE, reg_fn_abs_t::GAMMA> (Eigen::Array<value_type, Eigen::Dynamic, 1>&, const value_type, StreamlineStats&, StreamlineStats&, unsigned int&, BitSet&, value_type&);
      template void TckFactor::BBGD_update<reg_basis_t::FIXEL, reg_fn_abs_t::COEFF> (Eigen::Array<value_type, Eigen::Dynamic, 1>&, const value_type, StreamlineStats&, StreamlineStats&, unsigned int&, BitSet&, value_type&);
      template void TckFactor::BBGD_update<reg_basis_t::FIXEL, reg_fn_abs_t::FACTOR> (Eigen::Array<value_type, Eigen::Dynamic, 1>&, const value_type, StreamlineStats&, StreamlineStats&, unsigned int&, BitSet&, value_type&);
      template void TckFactor::BBGD_update<reg_basis_t::FIXEL, reg_fn_abs_t::GAMMA> (Eigen::Array<value_type, Eigen::Dynamic, 1>&, const value_type, StreamlineStats&, StreamlineStats&, unsigned int&, BitSet&, value_type&);





      }
    }
  }
}



