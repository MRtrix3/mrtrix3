/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 27/06/08.

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

/** \file
  This file describes a number of functions related to performing
  Monte Carlo Markhov Chain simulations of regularised spherical deconvolution.
 */

#ifndef __MCMC_spherical_deconvolution_h__
#define __MCMC_spherical_deconvolution_h__

#include "spherical_deconvolution.h"
#include <gsl/gsl_cdf.h>
#include <gsl/gsl_rng.h>
#include <gsl/gsl_multimin.h>
#include "matrix_ops.h"

namespace MR {

  class MCMCSphericalDeconv
  {
    protected:
      Matrix              fconv;
      Matrix              rconv;
      Matrix              HR_trans;
      Matrix              iHR_trans;
      Matrix              HR_enc;
      Matrix              iHR_trans_final;
      Vector              p_sigs;
      Vector              FOD;
      Vector              tmp;
      double             sigma;
      NumberSequence      p_bzeros, p_dwis, index_pos, min_index_pos;
      NumberSequence      B_index, N_index;
      Vector*             M_col;
      double*            M_col_norm2;
      gsl_rng*            rng;

      double             eval_f(Vector& residue);
      void                eval_df(Vector& df, const Vector& residue);
      void                subsolve(NumberSequence& pos_val);
      double             min_fval;

      Matrix              B, Binv, N;
      Vector              rcost, ones;
      PseudoInverter      pinverter;


    public:

      MCMCSphericalDeconv();
      ~MCMCSphericalDeconv();

      int  init(SHcoefs& response, Vector& init_filter,
                Matrix& DW_encoding, Matrix& HR_encoding, double noise_level, uint lmax = 8);

      void   set(Vector& sigs);
      float iterate_MAP();
      int   iterate_MAP2();
      int   iterate_MAP3(double& fval);
      void   iterate_MCMC();
      void   FOD2SH(const Vector& fod, Vector& SH, uint lmax = 8);
      void   get_state(Vector& state) const;
      void   get_best_state(Vector& state);
      void   get_sigs(Vector& sigs) const;
      void   FOD2sigs(const Vector& fod, Vector& SH) const;

  };





  inline double rand_truncated_Gaussian(const gsl_rng* r, double mu, double sigma)
  {
    double zero = gsl_cdf_gaussian_P(-mu, sigma);
    double rn = gsl_rng_uniform(r);
    double val = rn * (1.0 - zero) + zero;
    val = mu + gsl_cdf_ugaussian_Pinv(val)*sigma;
    if (isinf(val) || val < 0.0) val = 0.0;
    return (val);
  }

/****************************************************************
  *       MCMCRegularisedSphericalDeconv inline functions:
 ***************************************************************/


  inline void MCMCSphericalDeconv::get_state(Vector& state) const { state.copy(FOD); }

  inline void MCMCSphericalDeconv::get_sigs(Vector& sigs) const { sigs.copy(p_sigs); }
  inline void MCMCSphericalDeconv::FOD2sigs(const Vector& fod, Vector& SH) const
  {
    SH.allocate(fconv.rows());
    SH.multiply(fconv, fod);
  }

  inline double MCMCSphericalDeconv::eval_f(Vector& residue)
  {
    residue.multiply(fconv, FOD);
    residue.sub(p_sigs);
    return(residue.norm2());
  }




  inline void MCMCSphericalDeconv::eval_df(Vector& df, const Vector& residue)
  {
    df.multiply_trans(fconv, residue);
  }



}



#endif
