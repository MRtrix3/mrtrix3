/* Copyright (c) 2008-2017 the MRtrix3 contributors
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
      { MEMALIGN(Fixel)

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


