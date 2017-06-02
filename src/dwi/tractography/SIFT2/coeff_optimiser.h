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


#ifndef __dwi_tractography_sift2_coeff_optimiser_h__
#define __dwi_tractography_sift2_coeff_optimiser_h__


#include <vector>

#include "bitset.h"

#include "math/golden_section_search.h"
#include "math/quadratic_line_search.h"

#include "dwi/tractography/SIFT/track_index_range.h"
#include "dwi/tractography/SIFT/types.h"

#include "dwi/tractography/SIFT2/streamline_stats.h"


//#define SIFT2_COEFF_OPTIMISER_DEBUG


namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace SIFT2 {



      class TckFactor;



      class CoefficientOptimiserBase
      { MEMALIGN(CoefficientOptimiserBase)
        public:
          CoefficientOptimiserBase (TckFactor&, StreamlineStats&, StreamlineStats&, unsigned int&, BitSet&, double&);
          CoefficientOptimiserBase (const CoefficientOptimiserBase&);
          virtual ~CoefficientOptimiserBase();

          bool operator() (const SIFT::TrackIndexRange&);


        protected:
          TckFactor& master;
          const double mu;

          virtual double get_coeff_change (const SIFT::track_t) const = 0;


#ifdef SIFT2_COEFF_OPTIMISER_DEBUG
          size_t total, failed, wrong_dir, step_truncated, coeff_truncated;
#endif


        private:
          StreamlineStats& step_stats;
          StreamlineStats& coefficient_stats;
          unsigned int& nonzero_streamlines;
          BitSet& fixels_to_exclude;
          double& sum_costs;

          StreamlineStats local_stats_steps, local_stats_coefficients;
          size_t local_nonzero_count;
          BitSet local_to_exclude;

        protected:
          mutable double local_sum_costs;

        private:
          double do_fixel_exclusion (const SIFT::track_t);

      };








      // Golden Section Search within the permitted range
      class CoefficientOptimiserGSS : public CoefficientOptimiserBase
      { MEMALIGN(CoefficientOptimiserGSS)

        public:
          CoefficientOptimiserGSS (TckFactor&, StreamlineStats&, StreamlineStats&, unsigned int&, BitSet&, double&);
          CoefficientOptimiserGSS (const CoefficientOptimiserGSS&);
          ~CoefficientOptimiserGSS() { }

        private:
          double get_coeff_change (const SIFT::track_t) const;

      };





      // Performs a Quadratic Line Search within the permitted domain
      // Does not requre derivatives; only needs 3 seed points (two extremities and 0.0)
      // Note however if that these extremities are large, the initial CF evaluation may be NAN!
      class CoefficientOptimiserQLS : public CoefficientOptimiserBase
      { MEMALIGN(CoefficientOptimiserQLS)

        public:
          CoefficientOptimiserQLS (TckFactor&, StreamlineStats&, StreamlineStats&, unsigned int&, BitSet&, double&);
          CoefficientOptimiserQLS (const CoefficientOptimiserQLS&);
          ~CoefficientOptimiserQLS() { }

        private:
          Math::QuadraticLineSearch<double> qls;

          double get_coeff_change (const SIFT::track_t) const;

      };




      // Coefficient optimiser based on iterative root-finding Newton / Halley
      // Early exit if outside the permitted coefficient step range and moving further away
      class CoefficientOptimiserIterative : public CoefficientOptimiserBase
      { MEMALIGN(CoefficientOptimiserIterative)

        public:
          CoefficientOptimiserIterative (TckFactor&, StreamlineStats&, StreamlineStats&, unsigned int&, BitSet&, double&);
          CoefficientOptimiserIterative (const CoefficientOptimiserIterative&);
          ~CoefficientOptimiserIterative();

        private:
          double get_coeff_change (const SIFT::track_t) const;

#ifdef SIFT2_COEFF_OPTIMISER_DEBUG
          mutable uint64_t iter_count;
#endif

      };





      }
    }
  }
}


#endif
