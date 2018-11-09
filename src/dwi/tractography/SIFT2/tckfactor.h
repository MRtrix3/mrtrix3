/*
 * Copyright (c) 2008-2018 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/
 *
 * MRtrix3 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/
 */


#ifndef __dwi_tractography_sift2_tckfactor_h__
#define __dwi_tractography_sift2_tckfactor_h__


#include <fstream>
#include <limits>
#include <mutex>

#include "image.h"
#include "types.h"

#include "dwi/directions/set.h"

#include "dwi/tractography/SIFT/model.h"
#include "dwi/tractography/SIFT/output.h"

#include "dwi/tractography/SIFT2/fixel.h"



#define SIFT2_REGULARISATION_TIKHONOV_DEFAULT 0.0
#define SIFT2_REGULARISATION_TV_DEFAULT 0.1

#define SIFT2_MIN_TD_FRAC_DEFAULT 0.10

#define SIFT2_MIN_ITERS_DEFAULT 10
#define SIFT2_MAX_ITERS_DEFAULT 1000
#define SIFT2_MIN_COEFF_DEFAULT (-std::numeric_limits<default_type>::infinity())
#define SIFT2_MAX_COEFF_DEFAULT (std::numeric_limits<default_type>::infinity())
#define SIFT2_MAX_COEFF_STEP_DEFAULT 1.0
#define SIFT2_MIN_CF_DECREASE_DEFAULT 2.5e-5



namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace SIFT2 {




      class TckFactor : public SIFT::Model<Fixel>
      { MEMALIGN(TckFactor)

        public:

          TckFactor (Image<float>& fod_image, const DWI::Directions::FastLookupSet& dirs) :
              SIFT::Model<Fixel> (fod_image, dirs),
              reg_multiplier_tikhonov (0.0),
              reg_multiplier_tv (0.0),
              min_iters (SIFT2_MIN_ITERS_DEFAULT),
              max_iters (SIFT2_MAX_ITERS_DEFAULT),
              min_coeff (SIFT2_MIN_COEFF_DEFAULT),
              max_coeff (SIFT2_MAX_COEFF_DEFAULT),
              max_coeff_step (SIFT2_MAX_COEFF_STEP_DEFAULT),
              min_cf_decrease_percentage (SIFT2_MIN_CF_DECREASE_DEFAULT),
              data_scale_term (0.0) { }


          void set_reg_lambdas     (const double, const double);
          void set_min_iters       (const int    i) { min_iters = i; }
          void set_max_iters       (const int    i) { max_iters = i; }
          void set_min_factor      (const double i) { min_coeff = i ? std::log(i) : -std::numeric_limits<double>::infinity(); }
          void set_min_coeff       (const double i) { min_coeff = i; }
          void set_max_factor      (const double i) { max_coeff = std::log(i); }
          void set_max_coeff       (const double i) { max_coeff = i; }
          void set_max_coeff_step  (const double i) { max_coeff_step = i; }
          void set_min_cf_decrease (const double i) { min_cf_decrease_percentage = i; }

          void set_csv_path (const std::string& i) { csv_path = i; }


          void store_orig_TDs();

          void remove_excluded_fixels (const float);

          // Function that prints the cost function, then sets the streamline weights according to
          //   the inverse of length, and re-calculates and prints the cost function
          void test_streamline_length_scaling();

          // AFCSA method: Sum the attributable fibre volumes along the streamline length;
          //   divide by streamline length, convert to a weighting coefficient,
          //   see how the cost function fares
          void calc_afcsa();

          void estimate_factors();

          void output_factors (const std::string&) const;
          void output_coefficients (const std::string&) const;

          void output_all_debug_images (const std::string&) const;


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

          void indicate_progress() { if (App::log_level) fprintf (stderr, "."); }

      };





      }
    }
  }
}


#endif

