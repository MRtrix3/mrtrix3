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

#ifndef __dwi_tractography_sift2_coeff_optimiser_h__
#define __dwi_tractography_sift2_coeff_optimiser_h__


#include "math/golden_section_search.h"
#include "math/quadratic_line_search.h"
#include "misc/bitset.h"

#include "dwi/tractography/SIFT/track_index_range.h"
#include "dwi/tractography/SIFT/types.h"
#include "dwi/tractography/SIFT2/regularisation.h"
#include "dwi/tractography/SIFT2/streamline_stats.h"


//#define SIFT2_COEFF_OPTIMISER_DEBUG
//#define SIFT2_DELTA_OPTIMISER_DEBUG


namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace SIFT2 {



      class TckFactor;



      class CoefficientOptimiserBase
      {
        public:
          using value_type = SIFT::value_type;

          CoefficientOptimiserBase (TckFactor&, StreamlineStats&, StreamlineStats&, unsigned int&, BitSet&, value_type&);
          CoefficientOptimiserBase (const CoefficientOptimiserBase&);
          virtual ~CoefficientOptimiserBase();

          bool operator() (const SIFT::TrackIndexRange&);


        protected:
          TckFactor& master;
          const value_type mu;

          virtual value_type get_coeff_change (const SIFT::track_t) const = 0;


#ifdef SIFT2_COEFF_OPTIMISER_DEBUG
          size_t total, failed, wrong_dir, step_truncated, coeff_truncated;
#endif


        private:
          StreamlineStats& step_stats;
          StreamlineStats& coefficient_stats;
          unsigned int& nonzero_streamlines;
          BitSet& fixels_to_exclude;
          value_type& sum_costs;

          StreamlineStats local_stats_steps, local_stats_coefficients;
          size_t local_nonzero_count;
          BitSet local_to_exclude;

        protected:
          mutable value_type local_sum_costs;

        private:
          value_type do_fixel_exclusion (const SIFT::track_t);

      };






      // Completely commenting out the GSS optimiser for now,
      //   as it relies on having an unambiguous functor,
      //   whereas the current proposal involves a templated functor
      // TODO Should the template be moved from the line search functors to the class?
/*
      // Golden Section Search within the permitted range
      class CoefficientOptimiserGSS : public CoefficientOptimiserBase
      {

        public:
          CoefficientOptimiserGSS (TckFactor&, StreamlineStats&, StreamlineStats&, unsigned int&, BitSet&, value_type&);
          CoefficientOptimiserGSS (const CoefficientOptimiserGSS&);
          ~CoefficientOptimiserGSS() { }

        private:
          value_type get_coeff_change (const SIFT::track_t) const final;

      };
*/



      // As with GSS, QLS requires access to an untemplated functor
      // Theoretically this could be done with a wrapper class around LineSearchFunctor
      //   that disambiguates the template
      // But for now let's simplify as much as possible
/*
      // Performs a Quadratic Line Search within the permitted domain
      // Does not requre derivatives; only needs 3 seed points (two extremities and 0.0)
      // Note however if that these extremities are large, the initial CF evaluation may be NAN!
      class CoefficientOptimiserQLS : public CoefficientOptimiserBase
      {

        public:
          CoefficientOptimiserQLS (TckFactor&, StreamlineStats&, StreamlineStats&, unsigned int&, BitSet&, value_type&);
          CoefficientOptimiserQLS (const CoefficientOptimiserQLS&);
          ~CoefficientOptimiserQLS() { }

        private:
          Math::QuadraticLineSearch<value_type> qls;

          value_type get_coeff_change (const SIFT::track_t) const final;

      };
*/



      // Coefficient optimiser based on iterative root-finding Newton / Halley
      // Early exit if outside the permitted coefficient step range and moving further away
      // TODO Do I need to template this?
      // I think I've already done the line search functor; this is not much more than a wrapper...
      // Or we could just make a different class...
      class CoefficientOptimiserIterative : public CoefficientOptimiserBase
      {

        public:
          CoefficientOptimiserIterative (TckFactor&, StreamlineStats&, StreamlineStats&, unsigned int&, BitSet&, value_type&);
          CoefficientOptimiserIterative (const CoefficientOptimiserIterative&);
          ~CoefficientOptimiserIterative();

        private:
          value_type get_coeff_change (const SIFT::track_t) const final;

#ifdef SIFT2_COEFF_OPTIMISER_DEBUG
          mutable uint64_t iter_count;
#endif

      };




      // TODO New coefficient optimiser based on Barzilai-Borwein gradient descent
      template <reg_basis_t RegBasis, reg_fn_t RegFn>
      class CoefficientOptimiserBBGD : public CoefficientOptimiserBase
      {

        public:
          CoefficientOptimiserBBGD (TckFactor& tckfactor,
                                    Eigen::Array<value_type, Eigen::Dynamic, 1>& gradients,
                                    const value_type step_size,
                                    StreamlineStats& step_stats,
                                    StreamlineStats& coefficient_stats,
                                    unsigned int& nonzero_streamlines,
                                    BitSet& fixels_to_exclude,
                                    value_type& sum_costs) :
              CoefficientOptimiserBase (tckfactor, step_stats, coefficient_stats, nonzero_streamlines, fixels_to_exclude, sum_costs),
              gradients (gradients),
              step_size (step_size) { }

          CoefficientOptimiserBBGD (const CoefficientOptimiserBBGD& that) :
              CoefficientOptimiserBase (that),
              gradients (that.gradients),
              step_size (that.step_size) { }

          ~CoefficientOptimiserBBGD() { }

        private:
          Eigen::Array<value_type, Eigen::Dynamic, 1>& gradients;
          const value_type step_size;

          value_type get_coeff_change (const SIFT::track_t) const final;

      };




      // TODO Create a new class to deal with optimisation of the delta weights
      // Note that at least for now, this will NOT inherit from CoefficientOptimiserBase
      class DeltaOptimiserIterative
      {
        public:
          DeltaOptimiserIterative (TckFactor&, StreamlineStats&, StreamlineStats&, value_type&);
          ~DeltaOptimiserIterative();
          bool operator() (const SIFT::TrackIndexRange&);
        private:
          TckFactor& master;
          const value_type mu;
          StreamlineStats& step_stats;
          StreamlineStats& delta_stats;
          value_type& sum_costs;

          StreamlineStats local_stats_steps, local_stats_deltas;
          mutable value_type local_sum_costs;

          value_type operator() (const SIFT::track_t) const;

#ifdef SIFT2_COEFF_OPTIMISER_DEBUG
          size_t total, failed, wrong_dir, step_truncated, delta_truncated;
#endif
      };






      }
    }
  }
}


#endif
