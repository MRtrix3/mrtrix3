/*
    Copyright 2011 Brain Research Institute, Melbourne, Australia

    Written by Robert Smith, 2013.

    This file is part of MRtrix.

    MRtrix is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    MRtrix is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with MRtrix.  If not, see <http://www.gnu.org/licenses/>.

 */



#ifndef __dwi_tractography_sift2_iterative_projection_h__
#define __dwi_tractography_sift2_iterative_projection_h__


#include <vector>

#include "dwi/tractography/SIFT/track_index_range.h"
#include "dwi/tractography/SIFT/types.h"

#include "dwi/tractography/SIFT2/line_search.h"
#include "dwi/tractography/SIFT2/projection.h"
#include "dwi/tractography/SIFT2/streamline_stats.h"




namespace MR {
namespace DWI {
namespace Tractography {
namespace SIFT2 {



class TckFactor;


// TODO New projection calculator:
// Use ProjectionCalculatorCombined as the basis method, but
//   make use of any (asymmetric) correlation terms present
//   Maybe change to 'ProjectionOptimiser'?
// Theoretically, if this converges, the projection vector itself
//   becomes the coefficient update step
//
// Pretty sure the main loop will need to be in tckfactor
// Therefore this class will just skip the initial projection
//   calculation step, and go straight into its own optimisation
// Use the same line search approach as in ProjectionCalculatorCombined,
//   but initialise the line search functor using the current
//   projected step such that the correlation terms are included
//
// Also: In this approach, the line search should use the
//   correlation term corresponding to the current estimated direction;
//   if it flips sign, this should be handled by the next iteration
// Make sure this is the case
// - Yep, seems to be
//
// Might make all this in its own header, since it won't follow
//   the normal projection -> coefficient or projection -> GLS chain
//
// TODO Role of Base class may not entirely apply; may be safest to
//   skip this and re-implement what is necessary
// Actually, may try to keep the inheritance
// Potential problems:
// * Need to clear the correlation terms from the fixels before they
//   are updated with the new values (handled by the base class)
//   - Try a static bool member, mutex in destructor tests,
//     get_projected_step() also tests to make sure they're not getting
//     wiped before all threads have been completed



class IterativeProjection : public ProjectionCalculatorBase
{

  public:
    IterativeProjection (TckFactor&, std::vector<float>&, StreamlineStats&, size_t&);
    IterativeProjection (const IterativeProjection&);
    ~IterativeProjection();


  protected:

    class Minimum {
      public:
        Minimum (const float projected_step, const float cost_function) : step (projected_step), cf (cost_function) { }
        float step, cf;
        bool operator== (const Minimum& that) const { return (step == that.step && cf == that.cf); }
        bool operator!= (const Minimum& that) const { return (step != that.step || cf != that.cf); }
    };
    static const Minimum failed_search;
    Minimum optimise (const LineSearchFunctor&, const float) const;


  private:
    float get_projected_step (const SIFT::track_t) const;

    size_t& sign_flip;
    mutable size_t local_sign_flip;

    static bool corr_zeroed;

};






}
}
}
}


#endif

