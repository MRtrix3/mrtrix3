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

#include "math/golden_section_search.h"
#include "math/quadratic_line_search.h"

#include "dwi/tractography/SIFT/track_index_range.h"
#include "dwi/tractography/SIFT/types.h"

#include "dwi/tractography/SIFT2/streamline_stats.h"

// #define SIFT2_COEFF_OPTIMISER_DEBUG

namespace MR::DWI::Tractography::SIFT2 {

class TckFactor;
using SIFT::fixel_mask_type;

class CoefficientOptimiserBase {
public:
  CoefficientOptimiserBase(
      TckFactor &, StreamlineStats &, StreamlineStats &, unsigned int &, fixel_mask_type &, double &);
  CoefficientOptimiserBase(const CoefficientOptimiserBase &);
  virtual ~CoefficientOptimiserBase();

  bool operator()(const SIFT::TrackIndexRange &);

protected:
  TckFactor &master;
  const double mu;

  virtual double get_coeff_change(const SIFT::track_t) const = 0;

#ifdef SIFT2_COEFF_OPTIMISER_DEBUG
  size_t total, failed, wrong_dir, step_truncated, coeff_truncated;
#endif

private:
  StreamlineStats &step_stats;
  StreamlineStats &coefficient_stats;
  unsigned int &nonzero_streamlines;
  fixel_mask_type &fixels_to_exclude;
  double &sum_costs;

  StreamlineStats local_stats_steps, local_stats_coefficients;
  size_t local_nonzero_count;
  fixel_mask_type local_to_exclude;

protected:
  mutable double local_sum_costs;

private:
  double do_fixel_exclusion(const SIFT::track_t);
};

// Golden Section Search within the permitted range
class CoefficientOptimiserGSS : public CoefficientOptimiserBase {

public:
  CoefficientOptimiserGSS(
      TckFactor &, StreamlineStats &, StreamlineStats &, unsigned int &, fixel_mask_type &, double &);
  CoefficientOptimiserGSS(const CoefficientOptimiserGSS &);
  ~CoefficientOptimiserGSS() {}

private:
  double get_coeff_change(const SIFT::track_t) const;
};

// Performs a Quadratic Line Search within the permitted domain
// Does not requre derivatives; only needs 3 seed points (two extremities and 0.0)
// Note however if that these extremities are large, the initial CF evaluation may be NAN!
class CoefficientOptimiserQLS : public CoefficientOptimiserBase {

public:
  CoefficientOptimiserQLS(
      TckFactor &, StreamlineStats &, StreamlineStats &, unsigned int &, fixel_mask_type &, double &);
  CoefficientOptimiserQLS(const CoefficientOptimiserQLS &);
  ~CoefficientOptimiserQLS() {}

private:
  Math::QuadraticLineSearch<double> qls;

  double get_coeff_change(const SIFT::track_t) const;
};

// Coefficient optimiser based on iterative root-finding Newton / Halley
// Early exit if outside the permitted coefficient step range and moving further away
class CoefficientOptimiserIterative : public CoefficientOptimiserBase {

public:
  CoefficientOptimiserIterative(
      TckFactor &, StreamlineStats &, StreamlineStats &, unsigned int &, fixel_mask_type &, double &);
  CoefficientOptimiserIterative(const CoefficientOptimiserIterative &);
  ~CoefficientOptimiserIterative();

private:
  double get_coeff_change(const SIFT::track_t) const;

#ifdef SIFT2_COEFF_OPTIMISER_DEBUG
  mutable uint64_t iter_count;
#endif
};

} // namespace MR::DWI::Tractography::SIFT2
