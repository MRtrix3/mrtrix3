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



#ifndef __dwi_tractography_sift2_projection_h__
#define __dwi_tractography_sift2_projection_h__


#include <vector>

#include "dwi/tractography/SIFT/track_index_range.h"
#include "dwi/tractography/SIFT/types.h"

#include "dwi/tractography/SIFT2/line_search.h"
#include "dwi/tractography/SIFT2/streamline_stats.h"

#include "math/quadratic_line_search.h"




namespace MR {
namespace DWI {
namespace Tractography {
namespace SIFT2 {


/*
class TckFactor;


class ProjectionCalculatorBase
{

  public:
    ProjectionCalculatorBase (TckFactor&, std::vector<float>&, StreamlineStats&);
    ProjectionCalculatorBase (const ProjectionCalculatorBase&);

    virtual ~ProjectionCalculatorBase();


    bool operator() (const SIFT::TrackIndexRange&);


  protected:
    TckFactor& master;
    const double mu;

    virtual float get_projected_step (const SIFT::track_t) const = 0;

    float prev_projected_step (const SIFT::track_t i) const { return projected_steps[i]; }

  private:
    std::vector<float>& projected_steps;
    StreamlineStats& output_stats;

    // Each thread needs a local copy of this
    //std::vector<double> fixel_corr_term_TD;
    std::vector<double> fixel_corr_term_TD_pos, fixel_corr_term_TD_neg;
    StreamlineStats local_stats;


};




// Golden section search
// Simple and fairly robust
class ProjectionCalculatorGSS : public ProjectionCalculatorBase
{

  public:
    ProjectionCalculatorGSS (TckFactor&, std::vector<float>&, StreamlineStats&);
    ProjectionCalculatorGSS (const ProjectionCalculatorGSS&);
    ~ProjectionCalculatorGSS() { }

  private:
    float get_projected_step (const SIFT::track_t) const;

};




// Performs iterative root-finding searches from appropriate seed points
// From each extreme, it moves inwards at steps of 1.0 until the CF increases
// It then performs iterative Halley updates until it finds the minimum
// Finally, it selects the best of the two
class ProjectionCalculatorCombined : public ProjectionCalculatorBase
{

  public:

    class Minimum {
      public:
        Minimum (const float projected_step, const float cost_function) : step (projected_step), cf (cost_function) { }
        float step, cf;
        bool operator== (const Minimum& that) const { return (step == that.step && cf == that.cf); }
        bool operator!= (const Minimum& that) const { return (step != that.step || cf != that.cf); }
    };


    ProjectionCalculatorCombined (TckFactor&, std::vector<float>&, StreamlineStats&);
    ProjectionCalculatorCombined (const ProjectionCalculatorCombined&);
    ~ProjectionCalculatorCombined() { }

    static const Minimum failed_search;


  protected:
    Minimum optimise (const LineSearchFunctor&, const float, std::vector< std::pair<float, float> >&) const;


  private:
    float get_projected_step (const SIFT::track_t) const;

};
*/


/*

// New projection calculator based on linear streamline contribution
//   (produces a quadratic solution for the minimum location)
// FIXME More thorough testing, make sure it's doing what it should at later iterations
// FIXME Looks like it's getting some NANs...
class ProjectionCalculatorQuadratic : public ProjectionCalculatorBase
{

  public:
    ProjectionCalculatorQuadratic (TckFactor&, std::vector<float>&, StreamlineStats&);
    ProjectionCalculatorQuadratic (const ProjectionCalculatorQuadratic&);
    ~ProjectionCalculatorQuadratic() { }

  private:
    // Derived a new line search wrapper from LineSearchFunctor
    // Use it to gain access to the fixel information, which can then be used to
    //   get the two solutions of the quadratic equation; the functor is then used
    //   to select the appropriate solution
    class Worker : public SIFT2::LineSearchFunctor
    {
      public:
        Worker (const SIFT::track_t, const TckFactor&);
        float solve() const;
    };

    float get_projected_step (const SIFT::track_t) const;

};

*/






// Old attempts at projection calculation methods that are no good

/*

// Estimates the projection using a single Newton update
// Can produce erroneous values due to complex cost function shapes
class ProjectionCalculatorNewton : public ProjectionCalculatorBase
{

  public:
    ProjectionCalculatorNewton (TckFactor&, std::vector<float>&, StreamlineStats&);
    ProjectionCalculatorNewton (const ProjectionCalculatorNewton&);
    ~ProjectionCalculatorNewton() { }

  private:
    float get_projected_step (const SIFT::track_t) const;

};


// Repeat Newton-Raphson updates until convergence
// Often 0.0 is a poor seed point; hence why this often doesn't work
class ProjectionCalculatorNRIterative : public ProjectionCalculatorBase
{

  public:
    ProjectionCalculatorNRIterative (TckFactor&, std::vector<float>&, StreamlineStats&);
    ProjectionCalculatorNRIterative (const ProjectionCalculatorNRIterative&);
    ~ProjectionCalculatorNRIterative() { }

  private:
    float get_projected_step (const SIFT::track_t) const;

};


// Use Halley's method in an iterative fashion
// Suffers the same problem as NRIterative: Sometimes 0.0 is a poor start point for searching
class ProjectionCalculatorHalleyIterative : public ProjectionCalculatorBase
{

  public:
    ProjectionCalculatorHalleyIterative (TckFactor&, std::vector<float>&, StreamlineStats&);
    ProjectionCalculatorHalleyIterative (const ProjectionCalculatorHalleyIterative&);
    ~ProjectionCalculatorHalleyIterative() { }

  private:
    float get_projected_step (const SIFT::track_t) const;

};

*/








}
}
}
}


#endif

