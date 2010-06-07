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

#include "MCMC_spherical_deconvolution.h"
#include <time.h>
#include <gsl/gsl_randist.h>
#include <gsl/gsl_blas.h>
#include <gsl/gsl_linalg.h>
#define TOLERANCE 1e-6


namespace MR {



  MCMCSphericalDeconv::MCMCSphericalDeconv()
  {
    M_col = NULL;
    M_col_norm2 = NULL;
  }






  MCMCSphericalDeconv::~MCMCSphericalDeconv()
  {
    if (M_col) delete [] M_col;
    if (M_col_norm2) delete [] M_col_norm2;
  }





  void MCMCSphericalDeconv::subsolve(NumberSequence& pos_val)
  {
    uint i, j;
    Matrix M(fconv.rows(), pos_val.size());

    for (j = 0; j < pos_val.size(); j++)
      for (i = 0; i < fconv.rows(); i++)
        M(i,j) = fconv(i, pos_val[j]);

    Vector tau(MIN(fconv.rows(), pos_val.size()));
    Vector x(pos_val.size());
    Vector res(fconv.rows());

    gsl_linalg_QR_decomp(M.get_gsl_matrix(), tau.get_gsl_vector());
    gsl_linalg_QR_lssolve(M.get_gsl_matrix(), tau.get_gsl_vector(),
                          p_sigs.get_gsl_vector(), x.get_gsl_vector(), res.get_gsl_vector());

    for (i = 0; i < pos_val.size(); i++)
      FOD[pos_val[i]] = x[i];

    for (i = 0; i < FOD.size(); i++)
      if (FOD[i] < 0.0) FOD[i] = 0.0;
  }






  int MCMCSphericalDeconv::init(SHcoefs& response, Vector& init_filter, Matrix& DW_encoding, Matrix& HR_encoding, double noise_level, uint lmax)
  {
    sigma = noise_level;

    if (DW_encoding.rows() < 7 || DW_encoding.columns() != 4) {
      g_warning("unexpected diffusion encoding matrix dimensions");
      return(1);
    }

    if (guessDWdirections(p_dwis, p_bzeros, DW_encoding)) return(1);

    g_message("found %u diffusion-weighted studies", p_dwis.size());
    p_sigs.allocate(p_dwis.size());
    tmp.allocate(p_dwis.size());

    // generate directions from diffusion encoding:
    Matrix dirs;
    if (genDirectionMatrix(dirs, DW_encoding, p_dwis)) return(1);

    // check lmax:
    size_t n = LforNSH(p_dwis.size());
    if (n < (uint) lmax) {
      g_warning("warning: not enough data to estimate spherical harmonic components up to order %d", lmax);
      g_warning("falling back to lmax = %d", n);
      lmax = n;
    }
    g_message("calculating even spherical harmonic components up to order %d", lmax);

    if (response.size() < (lmax/2)+1) {
      g_warning("not enough response coefficients supplied for lmax = %d", lmax);
      return (1);
    }

    if (init_filter.size() < (lmax/2)+1) {
      g_warning("not enough initial filter coefficients supplied for lmax = %d", lmax);
      return (1);
    }

    // generate SH matrix:
    Matrix fconv_SH, rconv_SH;
    if (initSHTransform(fconv_SH, dirs, lmax)) return(1);

    pinverter.init(rconv_SH, fconv_SH);
    if (pinverter.invert(rconv_SH, fconv_SH)) {
      g_warning("error computing inverse");
      return (1);
    }

    Vector response_RH;
    if (SH2RH(response_RH, response)) {
      g_warning("error converting optimisation stage response function to rotational harmonics coefficients");
      return(1);
    }

    // include convolution with response function in fconv,
    // and filtered deconvolution in rconv
    uint row, col, l = 0U;
    for (row = 0; row < rconv_SH.rows(); row++) {
      if ((uint) NSHforL(2*l) <= row) l++;
      for (col = 0; col < rconv_SH.columns(); col++) {
//         rconv_SH(row, col) *= init_filter[l] / response_RH[l];
        rconv_SH(row, col) *= 1.0 / response_RH[l];
        fconv_SH(col,row) *= response_RH[l];
      }
    }

    // High-resolution scheme:
    HR_enc.copy(HR_encoding);
    if (initSHTransform(HR_trans, HR_encoding, lmax)) return(1);

    pinverter.init(iHR_trans, HR_trans);
    if (pinverter.invert(iHR_trans, HR_trans)) {
      g_warning("error computing inverse for high-resolution transform");
      return (1);
    }

    rconv.multiply(HR_trans, rconv_SH);
    fconv.multiply(fconv_SH, iHR_trans);

    FOD.allocate(fconv.columns());

    if (M_col) delete [] M_col;
    if (M_col_norm2) delete [] M_col_norm2;

    M_col = new Vector [FOD.size()];
    M_col_norm2 = new double [FOD.size()];


    for (col = 0; col < fconv.columns(); col++) {
      M_col[col].allocate(fconv.rows());
      M_col_norm2[col] = 0.0;
      for (row = 0; row < fconv.rows(); row++) {
        M_col[col][row] = fconv(row, col);
        M_col_norm2[col] += fconv(row, col)*fconv(row, col);
      }
    }


    rng = gsl_rng_alloc (gsl_rng_mt19937);
    gsl_rng_set(rng, time(NULL));

    g_message("MCMC spherical deconvolution initiated successfully");

    B.allocate(p_sigs.size(), p_sigs.size());
    Binv.allocate(B);
    N.allocate(p_sigs.size(), FOD.size()-p_sigs.size());
    rcost.allocate(FOD.size()-p_sigs.size());
    ones.allocate(p_sigs.size());
    ones.set_all(1.0);
    pinverter.init(Binv, B);

    uint i, j;
    B_index.resize(B.columns());
    N_index.resize(N.columns());

    for (i = 0; i < B_index.size(); i++)
      B_index[i] = i;

    for (i = 0; i < N_index.size(); i++)
      N_index[i] = i + B_index.size();

    for (j = 0; j < B.columns(); j++)
      for (i = 0; i < B.rows(); i++)
        B(i,j) = fconv(i, B_index[j]);

    for (j = 0; j < N.columns(); j++)
      for (i = 0; i < N.rows(); i++)
        N(i,j) = fconv(i, N_index[j]);

    return (0);
  }







  void MCMCSphericalDeconv::set(Vector& sigs)
  {
    size_t n;

    for (n = 0; n < p_dwis.size(); n++)
      p_sigs[n] = sigs[p_dwis[n]]; //norm;

//     FOD.multiply(rconv, p_sigs);
//     for (n = 0; n < FOD.size(); n++)
//       if (FOD[n] < 0.0) FOD[n] = 0.0;

//     Vector v;
//     v.multiply(fconv, FOD);
//     float f = 0.0;
//     for (n = 0; n < p_sigs.size(); n++)
//       f += p_sigs[n]/v[n];
//     f /= p_sigs.size();
//
//     for (n = 0; n < FOD.size(); n++)
//       FOD[n] *= f;

//     float maxval = 0.0;
//     uint i = 0;
//     for (n = 0; n < FOD.size(); n++) {
//       if (FOD[n] > maxval) {
//       maxval = FOD[n];
//       i = n;
//       }
//     }
    FOD.zero();

    index_pos.clear();
    min_fval = INFINITY;
  }







  float MCMCSphericalDeconv::iterate_MAP()
  {
    double step = 0.0, f_i;

    for (size_t n = 0; n < FOD.size(); n++) {
      f_i = FOD[n];
      FOD[n] = 0.0;
      tmp.multiply(fconv, FOD);
      tmp.sub(p_sigs);
      FOD[n] = -tmp.dot(M_col[n])/M_col_norm2[n];
      if (FOD[n] < 0.0) FOD[n] = 0.0;
      f_i -= FOD[n];
      step += f_i*f_i;
    }

    g_message("step = %g, fval = %g", step, eval_f(tmp));

    return (step);
  }






  int MCMCSphericalDeconv::iterate_MAP2()
  {
    Vector residue, df;
    double fval = eval_f(residue);

    if (fval < min_fval) {
      min_fval = fval;
      min_index_pos = index_pos;
    }

    eval_df(df, residue);

    NumberSequence seq = index_pos;
    index_pos.clear();

    int i;
    for (i = 0; i < (int) FOD.size(); i++) {
      if (FOD[i] > 0.0) index_pos.push_back(i);
      else if (df[i] < 0.0) index_pos.push_back(i);
    }

    bool converged = true;
    if (seq.size() == index_pos.size()) {
      for (i = 0; i < (int) seq.size(); i++) {
        if (seq[i] != index_pos[i]) {
          converged = false;
          break;
        }
      }
    }
    else converged = false;
    if (converged)  return (0);

    if (index_pos.size() > p_sigs.size())
      index_pos.resize(p_sigs.size());

    g_message("*********************************************");
    g_message("%d non-zero directions, fval = %g, obj = %g", seq.size(), fval, FOD.sum());

//      for (i = 0; i < FOD.size(); i++)
//        std::cerr << i << " [ " << FOD[i] << " " << df[i] << " ], ";
//      std::cerr << "\n";

    std::cerr << "[ ";
    for (i = 0; i < (int) seq.size(); i++)
      std::cerr << seq[i] << " ";
    std::cerr << " ]\n";
    std::cerr << "[ ";
    for (i = 0; i < (int) index_pos.size(); i++)
      std::cerr << index_pos[i] << " ";
    std::cerr << "]\n";

    subsolve(index_pos);

    return (1);
  }




  void MCMCSphericalDeconv::get_best_state(Vector& state)
  {
    size_t i;
    std::cerr << "Best guess:\n[ ";
    for (i = 0; i < min_index_pos.size(); i++)
      std::cerr << min_index_pos[i] << " ";
    std::cerr << "]\n";
    FOD.zero();
    subsolve(min_index_pos);
    state.copy(FOD);
  }





  int MCMCSphericalDeconv::iterate_MAP3(double& fval)
  {
    uint i, j;

    std::cerr << "mismatches in B: ";
    for (j = 0; j < B.columns(); j++)
      for (i = 0; i < B.rows(); i++)
        if (B(i,j) != fconv(i, B_index[j]))
          std::cerr << "[ " << i << " " << j << " ] ";
    std::cerr << "\n";

    std::cerr << "mismatches in N: ";
    for (j = 0; j < N.columns(); j++)
      for (i = 0; i < N.rows(); i++)
        if (N(i,j) != fconv(i, N_index[j]))
          std::cerr << "[ " << i << " " << j << " ] ";
    std::cerr << "\n";

    pinverter.invert(Binv, B);

    Matrix poo;
    poo.multiply(Binv, B);
    poo.print();

    std::cerr << "[ ";
    for (i = 0; i < B_index.size(); i++)
      std::cerr << B_index[i] << " ";
    std::cerr << "\n";

    std::cerr << "[ ";
    for (i = 0; i < N_index.size(); i++)
      std::cerr << N_index[i] << " ";
    std::cerr << "\n";


    // not required here: put in get_state() later on:
    tmp.multiply(Binv, p_sigs);
    FOD.zero();
    for (i = 0; i < B_index.size(); i++)
      FOD[B_index[i]] = tmp[i];

    fval = FOD.sum();

    tmp.multiply_trans(Binv, ones);
    rcost.multiply_trans(N, tmp);

    uint enter_index = 0;
    double min_val = 0.0;
    bool converged = true;

    for (i = 0; i < rcost.size(); i++) {
      if (rcost[i] < 0.0) {
        converged = false;
        if (rcost[i] < min_val) {
          min_val = rcost[i];
          enter_index = i;
        }
      }
    }

    VAR(rcost);
    VAR(min_val);
    VAR(enter_index);

    if (converged) return (0);

    for (i = 0; i < N.rows(); i++)
      tmp[i] = N(i, enter_index);

    rcost.multiply(Binv, tmp);
    tmp.multiply(Binv, p_sigs);

    uint leave_index = 0;
    min_val = INFINITY;
    double d;
    for (i = 0; i < tmp.size(); i++) {
      if (rcost[i] > 0.0) {
        d = tmp[i]/rcost[i];
        if (d < min_val) {
          min_val = d;
          leave_index = i;
        }
        else if (d == min_val)
          if (rcost[i] > rcost[leave_index])
            leave_index = i;
      }
    }

    i = B_index[leave_index];
    B_index[leave_index] = N_index[enter_index];
    N_index[enter_index] = i;

    for (i = 0; i < B.rows(); i++) {
      B(i, leave_index) = fconv(i, B_index[leave_index]);
      N(i, enter_index) = fconv(i, N_index[enter_index]);
    }

    FOD[N_index[enter_index]] = 0.0;

    return (1);
  }




  void MCMCSphericalDeconv::iterate_MCMC()
  {
    double mu_i, sigma_i;

    for (size_t n = 0; n < FOD.size(); n++) {
      FOD[n] = 0.0;
      tmp.multiply(fconv, FOD);
      tmp.sub(p_sigs);
      mu_i = -tmp.dot(M_col[n])/M_col_norm2[n];
      sigma_i = sigma/M_col_norm2[n];
      FOD[n] = rand_truncated_Gaussian(rng, mu_i, sigma_i);
//       FOD[n] = mu_i + gsl_ran_gaussian_ratio_method (rng, sigma_i);
    }
  }





  void MCMCSphericalDeconv::FOD2SH(const Vector& fod, Vector& SH, uint lmax)
  {
    if (iHR_trans_final.rows() != (uint) NSHforL(lmax)) {
      Matrix HR_trans_final;
      if (initSHTransform(HR_trans_final, HR_enc, lmax)) return;

      PseudoInverter pinverter(iHR_trans_final, HR_trans_final);
      if (pinverter.invert(iHR_trans_final, HR_trans_final)) {
        g_warning("error computing inverse for final high-resolution transform");
        return;
      }
    }
    SH.allocate(iHR_trans_final.rows());
    SH.multiply(iHR_trans_final, fod);
  }


}
