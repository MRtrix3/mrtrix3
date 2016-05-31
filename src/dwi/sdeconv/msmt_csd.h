/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 * 
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * For more details, see www.mrtrix.org
 * 
 */

#ifndef __dwi_sdeconv_msmt_csd_h__
#define __dwi_sdeconv_msmt_csd_h__

#include <vector>

#include "app.h"

#include "math/constrained_least_squares.h"

#include "dwi/shells.h"


namespace MR
{
  namespace DWI
  {
    namespace SDeconv
    {



      class MSMT_CSD {
        public:
          MSMT_CSD (std::vector<int>& lmax_, std::vector<Eigen::MatrixXd>& response_, Eigen::MatrixXd& grad_, Eigen::MatrixXd& HR_dirs) :
            lmax(lmax_),
            response(response_),
            grad(grad_)
        {
          DWI::Shells shells (grad);
          const size_t nbvals = shells.count();
          const size_t nsamples = grad.rows();
          const size_t ntissues = lmax.size();
          size_t nparams = 0;

          int maxlmax = 0;
          for (size_t i = 0; i < lmax.size(); i++) {
            nparams += Math::SH::NforL (lmax[i]);
            maxlmax = std::max (maxlmax, lmax[i]);
          }
          Eigen::MatrixXd C (nsamples, nparams);

          INFO ("initialising multi-tissue CSD for " + str(ntissues) + " tissue types, with " + str (nparams) + " parameters");

          std::vector<size_t> dwilist;
          for (size_t i = 0; i < nsamples; i++)
            dwilist.push_back(i);

          Eigen::MatrixXd directions = DWI::gen_direction_matrix (grad, dwilist);
          Eigen::MatrixXd SHT = Math::SH::init_transform (directions, maxlmax);
          for (ssize_t i = 0; i < SHT.rows(); i++)
            for (ssize_t j = 0; j < SHT.cols(); j++)
              if (std::isnan (SHT(i,j)))
                SHT(i,j) = 0.0;

          // TODO: is this just computing the Associated Legrendre polynomials...?
          Eigen::MatrixXd delta(1,2); delta << 0, 0;
          Eigen::MatrixXd DSH__ = Math::SH::init_transform (delta, maxlmax);
          Eigen::VectorXd DSH_ = DSH__.row(0);
          Eigen::VectorXd DSH (maxlmax/2+1);
          size_t j = 0;
          for (ssize_t i = 0; i < DSH_.size(); i++)
            if (DSH_[i] != 0.0) {
              DSH[j] = DSH_[i];
              j++;
            }

          size_t pbegin = 0;
          for (size_t tissue_idx = 0; tissue_idx < ntissues; ++tissue_idx) {
            const size_t tissue_lmax = lmax[tissue_idx];
            const size_t tissue_n = Math::SH::NforL (tissue_lmax);
            const size_t tissue_nmzero = tissue_lmax/2+1;

            for (size_t shell_idx = 0; shell_idx < nbvals; ++shell_idx) {
              Eigen::VectorXd response_ = response[tissue_idx].row (shell_idx);
              response_ = (response_.array() / DSH.head (tissue_nmzero).array()).matrix();
              Eigen::VectorXd fconv (tissue_n);
              int li = 0; int mi = 0;
              for (int l = 0; l <= int(tissue_lmax); l+=2) {
                for (int m = -l; m <= l; m++) {
                  fconv[mi] = response_[li];
                  mi++;
                }
                li++;
              }
              std::vector<size_t> vols = shells[shell_idx].get_volumes();
              for (size_t idx = 0; idx < vols.size(); idx++) {
                Eigen::VectorXd SHT_(SHT.row (vols[idx]).head (tissue_n));
                SHT_ = (SHT_.array()*fconv.array()).matrix();
                C.row (vols[idx]).segment (pbegin,tissue_n) = SHT_;
              }
            }
            pbegin += tissue_n;
          }

          std::vector<size_t> m(lmax.size());
          std::vector<size_t> n(lmax.size());
          size_t M = 0;
          size_t N = 0;

          Eigen::MatrixXd SHT300 = Math::SH::init_transform (HR_dirs, maxlmax);

          for(size_t i = 0; i < lmax.size(); i++) {
            if (lmax[i] > 0)
              m[i] = HR_dirs.rows();
            else
              m[i] = 1;
            M += m[i];
            n[i] = Math::SH::NforL (lmax[i]);
            N += n[i];
          }

          Eigen::MatrixXd A (M,N);
          size_t b_m = 0; size_t b_n = 0;
          for(size_t i = 0; i < lmax.size(); i++) {
            A.block(b_m,b_n,m[i],n[i]) = SHT300.block(0,0,m[i],n[i]);
            b_m += m[i];
            b_n += n[i];
          }

          problem = Math::ICLS::Problem<double>(C, A, 1.0e-10, 1.0e-10);
        }

        public:
          std::vector<int> lmax;
          std::vector<Eigen::MatrixXd> response;
          Eigen::MatrixXd& grad;
          Math::ICLS::Problem<double> problem;
      };



    }
  }
}

#endif


