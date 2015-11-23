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


#include "dwi/tractography/SIFT2/line_search.h"
#include "dwi/tractography/SIFT2/regularisation.h"
#include "dwi/tractography/SIFT2/tckfactor.h"


namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace SIFT2 {





      LineSearchFunctor::LineSearchFunctor (const SIFT::track_t index, const TckFactor& tckfactor) :
        track_index (index),
        mu (tckfactor.mu()),
        Fs (tckfactor.coefficients[track_index]),
        reg_tik (tckfactor.reg_multiplier_tikhonov),
        // Pre-scale reg_tv by total streamline contribution; each fixel then contributes (PM * length),
        //   and the whole thing is appropriately normalised
        reg_tv  (tckfactor.reg_multiplier_tv / tckfactor.contributions[track_index]->get_total_contribution())
      {
        const SIFT::TrackContribution& track_contribution = *tckfactor.contributions[track_index];
        for (size_t i = 0; i != track_contribution.dim(); ++i) {
          const SIFT2::Fixel& fixel (tckfactor.fixels[track_contribution[i].get_fixel_index()]);
          if (!fixel.is_excluded())
            fixels.push_back (Fixel (track_contribution[i], tckfactor, Fs, fixel.get_mean_coeff()));
        }
      }





      LineSearchFunctor::Result
      LineSearchFunctor::get (const double dFs) const
      {

        const double coefficient = Fs + dFs;
        const double factor = std::exp (coefficient);

        Result data_result, tik_result, tv_result;

        for (std::vector<Fixel>::const_iterator i = fixels.begin(); i != fixels.end(); ++i) {

          const double contribution = i->length * factor;
          const double scaled_contribution = mu * contribution;
          const double roc_contribution = mu * (contribution + i->dTD_dFs);
          const double diff = (mu * (i->TD + contribution + (i->dTD_dFs * dFs))) - i->FOD;

          data_result.cost += i->cost_frac * i->PM * Math::pow2 (diff);
          data_result.first_deriv += 2.0 * i->PM * i->cost_frac * (roc_contribution * diff);
          data_result.second_deriv += 2.0 * i->PM * i->cost_frac * (Math::pow2 (roc_contribution) + (scaled_contribution * diff));
          data_result.third_deriv += 2.0 * i->PM * i->cost_frac * scaled_contribution * ((3.0*roc_contribution) + diff);

          SIFT2::dxtvreg_dcoeffx (tv_result, coefficient, factor, i->SL_eff, i->meanFs, i->expmeanFs);

        }

        tik_result.cost         = reg_tik * Math::pow2 (coefficient);
        tik_result.first_deriv  = reg_tik * 2.0 * coefficient;
        tik_result.second_deriv = reg_tik * 2.0;

        tv_result *= reg_tv;

        data_result += tik_result;
        data_result += tv_result;
        return data_result;
      }



      double LineSearchFunctor::operator() (const double dFs) const
      {
        double cf_data = 0.0;
        double cf_reg_tv = 0.0;
        for (std::vector<Fixel>::const_iterator i = fixels.begin(); i != fixels.end(); ++i) {
          cf_data   += i->cost_frac * i->PM * Math::pow2 ((mu * (i->TD + (i->length * std::exp (Fs+dFs)) + (i->dTD_dFs*dFs))) - i->FOD);
          cf_reg_tv += i->SL_eff * SIFT2::tvreg (Fs+dFs, i->meanFs);
        }
        const double cf_reg_tik = Math::pow2 (Fs+dFs);
        return (cf_data + (reg_tik * cf_reg_tik) + (reg_tv * cf_reg_tv));
      }






      LineSearchFunctor::Fixel::Fixel (const SIFT::Track_fixel_contribution& in, const TckFactor& tckfactor, const double Fs, const double fixel_coeff_mean) :
          index (in.get_fixel_index()),
          length (in.get_length()),
          PM (tckfactor.fixels[index].get_weight()),
          TD (tckfactor.fixels[index].get_TD() - (length * std::exp (Fs))),
          cost_frac (length / tckfactor.fixels[index].get_orig_TD()),
          SL_eff (PM * length),
          dTD_dFs ((tckfactor.fixels[index].get_orig_TD() - length) * std::exp (Fs)),
          meanFs (fixel_coeff_mean),
          expmeanFs (std::exp (fixel_coeff_mean)),
          FOD (tckfactor.fixels[index].get_FOD()) { }




}
}
}
}

