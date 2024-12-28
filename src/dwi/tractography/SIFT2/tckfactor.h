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

#ifndef __dwi_tractography_sift2_tckfactor_h__
#define __dwi_tractography_sift2_tckfactor_h__


#include <fstream>
#include <limits>
#include <mutex>

#include "image.h"
#include "types.h"

#include "math/sphere/set/adjacency.h"

#include "dwi/tractography/SIFT/model.h"
#include "dwi/tractography/SIFT2/regularisation.h"
#include "dwi/tractography/SIFT2/streamline_stats.h"
#include "dwi/tractography/SIFT2/types.h"



#define SIFT2_MIN_TD_FRAC_DEFAULT 0.10

#define SIFT2_MIN_ITERS_DEFAULT 10
#define SIFT2_MAX_ITERS_DEFAULT 1000
#define SIFT2_MIN_COEFF_DEFAULT (-std::numeric_limits<SIFT::value_type>::infinity())
#define SIFT2_MAX_COEFF_DEFAULT (std::numeric_limits<SIFT::value_type>::infinity())
#define SIFT2_MAX_COEFF_STEP_DEFAULT 1.0
#define SIFT2_MIN_DELTACOEFF_DEFAULT -1.0
#define SIFT2_MAX_DELTACOEFF_DEFAULT 1.0
#define SIFT2_MAX_DELTACOEFF_STEP_DEFAULT 0.1
#define SIFT2_MIN_CF_DECREASE_DEFAULT 2.5e-5



// TODO For now, branch to BBGD using an envvar,
//   since aspects of the code other than just the coefficient optimiser need to change
//#define SIFT2_USE_BBGD



// TODO Should tcksift (ie. SIFT1) be given a differential mode?
// It may need an additional global multiplier on top of mu that modulates the total differential signal
// Also would need a bool per streamline to represent whether it's a positive or negative effect
// Would need multiple prospective changes per remaining streamline:
// - Change to negative effect (if not already so)
// - Change to positive effect (if not already so)
// - Remove from differential effect (irreversible)
// Maybe first complete some number of iterations where streamlines are just classified as positive or negative;
//   once that is completed, start a second set of iterations that prunes streamlines
//   (potentially not permitting streamlines to switch from positive to negative)





namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace SIFT2 {




      // TODO Once first version is working, put more effort into explicitly handling
      //   streamlines for which weighting factor is zero during differential processing

      class TckFactor : public SIFT::Model
      {

        public:

          static const ssize_t orig_td_column = 4;
          static const ssize_t mean_coeff_column = 5;
          // Replicating prior behaviour in that each fixel can have a boolean exclude flag set
          // This differs from a model weight of zero in that it only controls
          //   whether or not a fixel influences the optimisation, not the calculation of mu / cost function
          static const ssize_t exclude_column = 6;

          // Additional columns for differential version
          static const ssize_t differential_fd_column = 7;
          static const ssize_t differential_td_column = 8;
          static const ssize_t mean_deltacoeff_column = 9;

          class Fixel : public SIFT::Model::FixelBase
          {
            using BaseType = SIFT::Model::FixelBase;
            using RowType = data_matrix_type::RowXpr;
            using track_t = SIFT::track_t;
            using value_type = SIFT::Model::value_type;

            public:
              using BaseType::BaseType;

              // Functions for altering the state of this more advanced fixel class
              void store_orig_TD() { (*this)[orig_td_column] = td(); }
              void add_to_mean_coeff (const value_type i) { (*this)[mean_coeff_column] += i; }
              void clear_mean_coeff() { (*this)[mean_coeff_column] = 0.0; }
              void normalise_mean_coeff() { if (orig_TD()) (*this)[mean_coeff_column] /= orig_TD(); if (count() < 2) clear_mean_coeff(); }
              void exclude() { (*this)[exclude_column] = 1.0; }

              value_type orig_TD()    const { return (*this)[orig_td_column]; }
              value_type mean_coeff() const { return (*this)[mean_coeff_column]; }
              bool       excluded()   const { return bool((*this)[exclude_column]); }




              // TODO New functions for experimental BBGD solver
              value_type get_dcost_dTD_unweighted (const value_type mu) const { return 2.0 * mu * get_diff (mu); }
              value_type get_dcost_dTD (const value_type mu) const { return weight() * get_dcost_dTD_unweighted (mu); }



              // TODO New functions for differential fit
              value_type differential_TD() const { return (*this)[differential_td_column]; }
              void add_to_differential_TD (const value_type differential_td, const value_type length, const value_type weight) { (*this)[differential_td_column] += differential_td * length * weight; }
              void add_to_differential_TD (const value_type i) { (*this)[differential_td_column] += i; }
              void clear_mean_deltacoeff() { (*this)[mean_deltacoeff_column] = 0.0; }
              void add_to_mean_deltacoeff (const value_type i) { (*this)[mean_deltacoeff_column] += i; }
              void normalise_mean_deltacoeff() { if (td()) (*this)[mean_deltacoeff_column] /= td(); if (count() < 2) clear_mean_deltacoeff(); }
              value_type mean_deltacoeff() const { return (*this)[mean_deltacoeff_column]; }
              // TODO Re-attempt using the below function as a substitute for mean deltacoeff
              //value_type mean_deltacoeff() const { return differential_TD() / td(); }
              value_type differential_FD() const { return (*this)[differential_fd_column]; }

              value_type differential_diff (const value_type mu) const { return (mu * differential_TD()) - differential_FD(); }
              value_type differential_cost_unweighted (const value_type mu) const { return Math::pow2 (differential_diff(mu)); }
              value_type differential_cost (const value_type mu) const { return weight() > value_type(0) ? weight() * differential_cost_unweighted(mu) : value_type(0); }

          };






          TckFactor (const std::string& fd_path) :
              Model (fd_path),
              reg_basis_abs (reg_basis_t::FIXEL),
              reg_basis_diff (reg_basis_t::STREAMLINE),
              reg_fn_abs (reg_fn_abs_t::GAMMA),
              reg_fn_diff (reg_fn_diff_t::DUALINVBARR),
              reg_multiplier_abs (0.0),
              reg_multiplier_diff (0.0),
              reg_scaling (std::numeric_limits<value_type>::signaling_NaN()),
              min_iters (SIFT2_MIN_ITERS_DEFAULT),
              max_iters (SIFT2_MAX_ITERS_DEFAULT),
              min_coeff (SIFT2_MIN_COEFF_DEFAULT),
              max_coeff (SIFT2_MAX_COEFF_DEFAULT),
              max_coeff_step (SIFT2_MAX_COEFF_STEP_DEFAULT),
              min_deltacoeff (SIFT2_MIN_DELTACOEFF_DEFAULT),
              max_deltacoeff (SIFT2_MAX_DELTACOEFF_DEFAULT),
              max_deltacoeff_step (SIFT2_MAX_DELTACOEFF_STEP_DEFAULT),
              min_cf_decrease_percentage (SIFT2_MIN_CF_DECREASE_DEFAULT),
              data_scale_term (0.0),
              naive_cf (std::numeric_limits<value_type>::signaling_NaN())
          {
            // Expand data matrix to include additional columns necessary for new fixel class
            // TODO More robust way to define new number of columns
#ifndef NDEBUG
            fixels.conservativeResize(nfixels(), 7);
#else
            fixels.conservativeResizeLike(data_matrix_type::Constant(nfixels(), 7, std::numeric_limits<value_type>::signaling_NaN()));
#endif
            fixels.col(exclude_column).setZero();
          }

          TckFactor (const TckFactor& that) = delete;

          ~TckFactor() { }



          void set_reg_basis_abs   (const reg_basis_t i) { reg_basis_abs = i; }
          void set_reg_basis_diff  (const reg_basis_t i) { reg_basis_diff = i; }
          void set_reg_fn_abs      (const reg_fn_abs_t i) { reg_fn_abs = i; }
          void set_reg_fn_diff     (const reg_fn_diff_t i) { reg_fn_diff = i; }
          void set_reg_lambda_abs  (const value_type i)  { assert (std::isfinite (reg_scaling)); reg_multiplier_abs = i * reg_scaling; }
          void set_reg_lambda_diff (const value_type i)  { assert (std::isfinite (reg_scaling)); reg_multiplier_diff = i * reg_scaling; }
          void set_min_iters       (const int i) { min_iters = i; }
          void set_max_iters       (const int i) { max_iters = i; }
          void set_min_factor      (const value_type i) { min_coeff = i ? std::log(i) : -std::numeric_limits<value_type>::infinity(); }
          void set_min_coeff       (const value_type i) { min_coeff = i; }
          void set_max_factor      (const value_type i) { max_coeff = std::log(i); }
          void set_max_coeff       (const value_type i) { max_coeff = i; }
          void set_max_coeff_step  (const value_type i) { max_coeff_step = i; }
          void set_min_deltacoeff      (const value_type i) { assert (i >= -1.0); min_deltacoeff = i; }
          void set_max_deltacoeff      (const value_type i) { assert (i <= 1.0); max_deltacoeff = i; }
          void set_max_deltacoeff_step (const value_type i) { assert (i > 0.0); assert (i <= 2.0); max_deltacoeff_step = i; }
          void set_min_cf_decrease     (const value_type i) { min_cf_decrease_percentage = i; }
          void set_fixed_mu            (const value_type i) { fixed_mu = i; }

          void set_csv_path (const std::string& path);

          void set_coefficients (const std::string& path);
          void set_factors (const std::string& path);
          void set_deltacoeffs (const std::string &path);
          void set_deltafactors (const std::string &path);
          // This should only be called if coefficients have been loaded from the command-line,
          //   and there is to be subsequent optimisation performed based on such;
          //   these limits should otherwise be being enforced by whatever code is responsible for the optimisation
          template <operation_mode_t Mode>
          void enforce_limits();

          template <operation_mode_t Mode>
          void set_mask (const std::string &path);

          void import_differential_data (const std::string &path);


          void store_orig_TDs();

          void exclude_fixels (const value_type);

          void calibrate_regularisation();

          // Function that prints the cost function, then sets the streamline weights according to
          //   the inverse of length, and re-calculates and prints the cost function
          void test_streamline_length_scaling();

          // AFCSA method: Sum the attributable fibre volumes along the streamline length;
          //   divide by streamline length, convert to a weighting coefficient,
          //   see how the cost function fares
          void calc_afcsa();

          value_type calc_cost_function_differential();

          void save_naive_cf();

          template <operation_mode_t Mode>
          void estimate_weights();

          void report_entropy() const;

          void output_factors (const std::string&) const;
          void output_coefficients (const std::string&) const;

          void output_TD_images (const std::string&, const std::string&, const std::string&);
          void output_all_debug_images (const std::string&, const std::string&);

          void output_deltacoeffs (const std::string&) const;
          void output_deltaweights (const std::string&) const;
          void output_differential_debug_images (const std::string&, const std::string&);

        private:
          Eigen::Array<value_type, Eigen::Dynamic, 1> coefficients;
          Eigen::Array<value_type, Eigen::Dynamic, 1> deltacoeffs;

          // TODO Preclude specific streamlines from having their contributions modified by the optimiser
          // TODO If there is any multi-threading process that attempts to update this,
          //   a race condition is going to occur
          Eigen::Array<bool, Eigen::Dynamic, 1> mask_absolute;
          Eigen::Array<bool, Eigen::Dynamic, 1> mask_differential;

          reg_basis_t reg_basis_abs;
          reg_basis_t reg_basis_diff;
          reg_fn_abs_t reg_fn_abs;
          reg_fn_diff_t reg_fn_diff;
          value_type reg_multiplier_abs, reg_multiplier_diff;
          value_type reg_scaling;
          size_t min_iters, max_iters;
          value_type min_coeff, max_coeff, max_coeff_step;
          value_type min_deltacoeff, max_deltacoeff, max_deltacoeff_step;
          value_type min_cf_decrease_percentage;
          std::string csv_path;

          value_type data_scale_term;
          value_type naive_cf;


          friend class LineSearchFunctorBase;
          friend class LineSearchFunctorAbsolute;
          friend class LineSearchFunctorDifferential;
          friend class CoefficientOptimiserBase;
          //friend class CoefficientOptimiserGSS;
          //friend class CoefficientOptimiserQLS;
          friend class CoefficientOptimiserIterative;
          friend class DeltaOptimiserIterative;
          template <reg_basis_t RegBasis, reg_fn_abs_t RegFn>
          friend class CoefficientOptimiserBBGD;
          friend class FixelUpdaterBase;
          friend class FixelUpdaterAbsolute;
          friend class FixelUpdaterDifferential;
          friend class RegularisationCalculatorBase;
          template <reg_basis_t RegBasis, reg_fn_abs_t RegFn>
          friend class RegularisationCalculatorAbsolute;
          template <reg_basis_t RegBasis, reg_fn_diff_t RegFn>
          friend class RegularisationCalculatorDifferential;

          // For when multiple threads are trying to write their final information back
          std::mutex mutex;

          void indicate_progress() { if (App::log_level) fprintf (stderr, "."); }


        public:
          template <operation_mode_t Mode>
          void update_fixels();

        private:
          template <operation_mode_t Mode>
          value_type calculate_regularisation();
          // TODO Implement entropy calculation for differential mode (based on absolute values)
          value_type calculate_entropy() const;



          template <reg_basis_t RegBasis, reg_fn_abs_t RegFn>
          void BBGD_update (Eigen::Array<value_type, Eigen::Dynamic, 1>& gradients,
                            const value_type step_size,
                            StreamlineStats& step_stats,
                            StreamlineStats& coefficient_stats,
                            unsigned int& nonzero_streamlines,
                            BitSet& fixels_to_exclude,
                            value_type& sum_costs);


      };





      }
    }
  }
}


#endif

