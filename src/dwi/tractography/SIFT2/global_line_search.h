/*
    Copyright 2011 Brain Research Institute, Melbourne, Australia

    Written by Robert Smith, 2014.

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



#ifndef __dwi_tractography_sift2_global_line_search_h__
#define __dwi_tractography_sift2_global_line_search_h__



#include <vector>

#include "math/vector.h"


namespace MR {
namespace DWI {
namespace Tractography {
namespace SIFT2 {



class TckFactor;



// TODO New line search functor for minimising a global gradient descent step
// Although this was tried in the past, it may have failed due to the poor definition of the projected steps
// Therefore, try again with the newer, more robust calculation
// TODO Also, once this is finished, experiment with the three different projection types



class GlobalLineSearchFunctor
{

  public:

    GlobalLineSearchFunctor (TckFactor&, const std::vector<float>&);

    // Interface for line search
    float operator() (const float);

    float get_cf_data()         const { return cf_data; }
    float get_cf_reg_tik()      const { return cf_reg_tik; }
    float get_cf_reg_tv_fixel() const { return cf_reg_tv_fixel; }
    float get_cf_reg_tv_sl()    const { return cf_reg_tv_sl; }


  protected:
    TckFactor& master;
    const Math::Vector<float> orig_coeffs;
    const std::vector<float>& projected_steps;

    // Keep stats on most recent test
    float cf_data, cf_reg_tik, cf_reg_tv_fixel, cf_reg_tv_sl;

};






}
}
}
}


#endif

