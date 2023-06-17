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
#include "dwi/tractography/SIFT/types.h"



#define SIFT2_REGULARISATION_TIKHONOV_DEFAULT 0.0
#define SIFT2_REGULARISATION_TV_DEFAULT 0.1

#define SIFT2_MIN_TD_FRAC_DEFAULT 0.10

#define SIFT2_MIN_ITERS_DEFAULT 10
#define SIFT2_MAX_ITERS_DEFAULT 1000
#define SIFT2_MIN_COEFF_DEFAULT (-std::numeric_limits<SIFT::value_type>::infinity())
#define SIFT2_MAX_COEFF_DEFAULT (std::numeric_limits<SIFT::value_type>::infinity())
#define SIFT2_MAX_COEFF_STEP_DEFAULT 1.0
#define SIFT2_MIN_CF_DECREASE_DEFAULT 2.5e-5



namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace SIFT2 {




      class TckFactor : public SIFT::Model
      {

        public:

          static const ssize_t orig_td_column = 4;
          static const ssize_t mean_coeff_column = 5;
          // Replicating prior behaviour in that each fixel can have a boolean exclude flag set
          // This differs from a model weight of zero in that it only controls
          //   whether or not a fixel influences the optimisation, not the calculation of mu / cost function
          static const ssize_t exclude_column = 6;

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

          };






          TckFactor (const std::string& fd_path) :
              Model (fd_path),
              reg_multiplier_tikhonov (0.0),
              reg_multiplier_tv (0.0),
              min_iters (SIFT2_MIN_ITERS_DEFAULT),
              max_iters (SIFT2_MAX_ITERS_DEFAULT),
              min_coeff (SIFT2_MIN_COEFF_DEFAULT),
              max_coeff (SIFT2_MAX_COEFF_DEFAULT),
              max_coeff_step (SIFT2_MAX_COEFF_STEP_DEFAULT),
              min_cf_decrease_percentage (SIFT2_MIN_CF_DECREASE_DEFAULT),
              data_scale_term (0.0)
          {
            // Expand data matrix to include additional columns necessary for new fixel class
            // TODO More robust way to define new number of columns
            fixels.conservativeResizeLike(data_matrix_type::Zero(nfixels(), 7));
          }

          TckFactor (const TckFactor& that) = delete;

          ~TckFactor() { }




          void set_reg_lambdas     (const value_type, const value_type);
          void set_min_iters       (const int    i) { min_iters = i; }
          void set_max_iters       (const int    i) { max_iters = i; }
          void set_min_factor      (const value_type i) { min_coeff = i ? std::log(i) : -std::numeric_limits<value_type>::infinity(); }
          void set_min_coeff       (const value_type i) { min_coeff = i; }
          void set_max_factor      (const value_type i) { max_coeff = std::log(i); }
          void set_max_coeff       (const value_type i) { max_coeff = i; }
          void set_max_coeff_step  (const value_type i) { max_coeff_step = i; }
          void set_min_cf_decrease (const value_type i) { min_cf_decrease_percentage = i; }

          void set_csv_path (const std::string& i) { csv_path = i; }


          void store_orig_TDs();

          void exclude_fixels (const value_type);

          // Function that prints the cost function, then sets the streamline weights according to
          //   the inverse of length, and re-calculates and prints the cost function
          void test_streamline_length_scaling();

          // AFCSA method: Sum the attributable fibre volumes along the streamline length;
          //   divide by streamline length, convert to a weighting coefficient,
          //   see how the cost function fares
          void calc_afcsa();

          void estimate_factors();

          void report_entropy() const;

          void output_factors (const std::string&) const;
          void output_coefficients (const std::string&) const;

          void output_TD_images (const std::string&, const std::string&, const std::string&);
          void output_all_debug_images (const std::string&, const std::string&);


        private:
          Eigen::Array<value_type, Eigen::Dynamic, 1> coefficients;

          value_type reg_multiplier_tikhonov, reg_multiplier_tv;
          size_t min_iters, max_iters;
          value_type min_coeff, max_coeff, max_coeff_step, min_cf_decrease_percentage;
          std::string csv_path;

          value_type data_scale_term;


          friend class LineSearchFunctor;
          friend class CoefficientOptimiserBase;
          friend class CoefficientOptimiserGSS;
          friend class CoefficientOptimiserQLS;
          friend class CoefficientOptimiserIterative;
          friend class FixelUpdater;
          friend class RegularisationCalculator;


          // For when multiple threads are trying to write their final information back
          std::mutex mutex;

          void indicate_progress() { if (App::log_level) fprintf (stderr, "."); }

      };





      }
    }
  }
}


#endif

