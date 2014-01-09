/*
    Copyright 2011 Brain Research Institute, Melbourne, Australia

    Written by Robert Smith, 2011.

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



#ifndef __dwi_tractography_sift_fixel_h__
#define __dwi_tractography_sift_fixel_h__


#include "dwi/fmls.h"

#include "dwi/tractography/SIFT/model_base.h"


namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {
      namespace SIFT
      {


      class Fixel : public FixelBase
      {

        public:
          Fixel () :
            FixelBase () { }

          Fixel (const FMLS::FOD_lobe& lobe) :
            FixelBase (lobe) { }

          Fixel (const Fixel& that) :
            FixelBase (that) { }


          Fixel& operator-= (const double length) { TD = std::max (TD - length, 0.0); return *this; }

          double get_d_cost_d_mu    (const double mu)                         const { return get_d_cost_d_mu_unweighted    (mu) * weight; }
          double get_cost_wo_track  (const double mu, const double length)    const { return get_cost_wo_track_unweighted  (mu, length)    * weight; }
          double get_cost_manual_TD (const double mu, const double manual_TD) const { return get_cost_manual_TD_unweighted (mu, manual_TD) * weight; }
          double calc_quantisation  (const double mu, const double length)    const { return get_cost_manual_TD            (mu, (FOD/mu) + length); }


        private:
          double get_d_cost_d_mu_unweighted    (const double mu) const { return (2.0 * TD * get_diff (mu)); }
          double get_cost_wo_track_unweighted  (const double mu, const double length)    const { return (Math::pow2 ((std::max (TD-length, 0.0) * mu) - FOD)); }
          double get_cost_manual_TD_unweighted (const double mu, const double manual_TD) const { return  Math::pow2 ((      manual_TD         * mu) - FOD); }

      };





      }
    }
  }
}


#endif


