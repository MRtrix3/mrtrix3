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

#ifndef __dwi_sdeconv_csd_h__
#define __dwi_sdeconv_csd_h__

#include "app.h"
#include "header.h"
#include "dwi/gradient.h"
#include "math/SH.h"
#include "math/ZSH.h"
#include "dwi/directions/predefined.h"
#include "math/least_squares.h"

#define NORM_LAMBDA_MULTIPLIER 0.0002

#define DEFAULT_CSD_NEG_LAMBDA 1.0
#define DEFAULT_CSD_NORM_LAMBDA 1.0
#define DEFAULT_CSD_THRESHOLD 0.0
#define DEFAULT_CSD_NITER 50

namespace MR
{
  namespace DWI
  {
    namespace SDeconv
    {

    extern const App::OptionGroup CSD_options;

    class CSD
    {
      public:
        class Shared
        {
          public:

            Shared (const Header& dwi_header) :
              HR_dirs (Directions::electrostatic_repulsion_300()),
              neg_lambda (DEFAULT_CSD_NEG_LAMBDA),
              norm_lambda (DEFAULT_CSD_NORM_LAMBDA),
              threshold (DEFAULT_CSD_THRESHOLD),
              niter (DEFAULT_CSD_NITER) {
                grad = DWI::get_valid_DW_scheme (dwi_header);
                // Discard b=0 (b=0 normalisation not supported in this version)
                // Only allow selection of one non-zero shell from command line
                dwis = DWI::Shells (grad).select_shells (false, true).largest().get_volumes();
                DW_dirs = DWI::gen_direction_matrix (grad, dwis);

                lmax_data = Math::SH::LforN (dwis.size()); 
                lmax = std::min (8, lmax_data);
                lmax_response = 0;
              }


            void parse_cmdline_options()
            {
              using namespace App;
              auto opt = get_options ("lmax");
              if (opt.size()) {
                auto list = parse_ints (opt[0][0]);
                if (list.size() != 1)
                  throw Exception ("CSD algorithm expects a single lmax to be specified");
                lmax = list.front();
              }
              opt = get_options ("filter");
              if (opt.size())
                init_filter = load_vector<> (opt[0][0]);
              opt = get_options ("directions");
              if (opt.size())
                HR_dirs = load_matrix (opt[0][0]);
              opt = get_options ("neg_lambda");
              if (opt.size())
                neg_lambda = opt[0][0];
              opt = get_options ("norm_lambda");
              if (opt.size())
                norm_lambda = opt[0][0];
              opt = get_options ("threshold");
              if (opt.size())
                threshold = opt[0][0];
              opt = get_options ("niter");
              if (opt.size())
                niter = opt[0][0];
            }


            void set_response (const std::string& path)
            {
              INFO ("loading response function from file \"" + path + "\"");
              response = load_vector (path);

              lmax_response = 2*(response.size()-1);
              INFO ("setting response function using even SH coefficients: " + str (response.transpose()));
            }

            template <class Derived>
              void set_response (const Eigen::MatrixBase<Derived>& in)
              {
                response = in;
                lmax_response = Math::ZSH::LforN (response.size());
              }


            void init ()
            {
              using namespace Math::SH;

              if (lmax <= 0 || lmax % 2)
                throw Exception ("lmax must be a positive even integer");

              assert (response.size());
              lmax_response = std::min (lmax_response, std::min (lmax_data, lmax));
              INFO ("calculating even spherical harmonic components up to order " + str (lmax_response) + " for initialisation");

              if (!init_filter.size())
                init_filter = Eigen::VectorXd::Ones(3);
              init_filter.conservativeResizeLike (Eigen::VectorXd::Zero (Math::ZSH::NforL (lmax_response)));

              auto RH = SH2RH (response);
              if (size_t(RH.size()) < Math::ZSH::NforL (lmax))
                RH.conservativeResizeLike (Eigen::VectorXd::Zero (Math::ZSH::NforL (lmax)));

              // inverse sdeconv for initialisation:
              auto fconv = init_transform (DW_dirs, lmax_response);
              rconv.resize (fconv.cols(), fconv.rows());
              fconv.diagonal().array() += 1.0e-2;
              //fconv.save ("fconv.txt");
              rconv = Math::pinv (fconv);
              //rconv.save ("rconv.txt");
              ssize_t l = 0, nl = 1;
              for (ssize_t row = 0; row < rconv.rows(); ++row) {
                if (row >= nl) {
                  l++;
                  nl = NforL (2*l);
                }
                rconv.row (row).array() *= init_filter[l] / RH[l];
              }

              // forward sconv for iteration, using all response function
              // coefficients up to the requested lmax:
              INFO ("calculating even spherical harmonic components up to order " + str (lmax) + " for output");
              fconv = init_transform (DW_dirs, lmax);
              l = 0;
              nl = 1;
              for (ssize_t col = 0; col < fconv.cols(); ++col) {
                if (col >= nl) {
                  l++;
                  nl = NforL (2*l);
                }
                fconv.col (col).array() *= RH[l];
              }

              // high-res sampling to apply constraint:
              HR_trans = init_transform (HR_dirs, lmax);
              default_type constraint_multiplier = neg_lambda * 50.0 * response[0] / default_type (HR_trans.rows());
              HR_trans.array() *= constraint_multiplier;

              // adjust threshold accordingly:
              threshold *= constraint_multiplier;

              // precompute as much as possible ahead of Cholesky decomp:
              assert (fconv.cols() <= HR_trans.cols());
              M.resize (DW_dirs.rows(), HR_trans.cols());
              M.leftCols (fconv.cols()) = fconv;
              M.rightCols (M.cols() - fconv.cols()).setZero();
              Mt_M.resize (M.cols(), M.cols());
              Mt_M.triangularView<Eigen::Lower>() = M.transpose() * M;


              // min-norm constraint:
              if (norm_lambda) {
                norm_lambda *= NORM_LAMBDA_MULTIPLIER * Mt_M (0,0);
#ifndef USE_NON_ORTHONORMAL_SH_BASIS
                Mt_M.diagonal().array() += norm_lambda;
#else
                int l = 0;
                for (size_t i = 0; i < Mt_M.rows(); ++i) {
                  if (Math::SH::index (l,0) == i) {
                    Mt_M(i,i) += norm_lambda;
                    l+=2;
                  }
                  else 
                    Mt_M(i,i) += 0.5 * norm_lambda;
                }
#endif
              }

              INFO ("constrained spherical deconvolution initialised successfully");
            }

            size_t nSH () const {
              return HR_trans.cols();
            }

            Eigen::MatrixXd grad;
            Eigen::VectorXd response, init_filter;
            Eigen::MatrixXd DW_dirs, HR_dirs;
            Eigen::MatrixXd rconv, HR_trans, M, Mt_M;
            default_type neg_lambda, norm_lambda, threshold;
            std::vector<size_t> dwis;
            int lmax_response, lmax_data, lmax;
            size_t niter;
        };








        CSD (const Shared& shared_data) :
          shared (shared_data),
          work (shared.Mt_M.rows(), shared.Mt_M.cols()),
          HR_T (shared.HR_trans.rows(), shared.HR_trans.cols()),
          F (shared.HR_trans.cols()),
          init_F (shared.rconv.rows()),
          HR_amps (shared.HR_trans.rows()),
          Mt_b (shared.HR_trans.cols()),
          llt (work.rows()),
          old_neg (shared.HR_trans.rows()),
          computed_once (false) { }

        CSD (const CSD&) = default;

        ~CSD() { }

        template <class VectorType>
          void set (const VectorType& DW_signals) {
            F.head (shared.rconv.rows()) = shared.rconv * DW_signals;
            F.tail (F.size()-shared.rconv.rows()).setZero();
            old_neg.assign (shared.HR_trans.rows(), -1);
            computed_once = false;

            Mt_b = shared.M.transpose() * DW_signals;
          }

        bool iterate() {
          neg.clear();
          HR_amps = shared.HR_trans * F;
          for (ssize_t n = 0; n < HR_amps.size(); n++)
            if (HR_amps[n] < shared.threshold)
              neg.push_back (n);

          if (computed_once && old_neg == neg)
            if (old_neg == neg)
              return true;

          work.triangularView<Eigen::Lower>() = shared.Mt_M.triangularView<Eigen::Lower>();

          if (neg.size()) {
            for (size_t i = 0; i < neg.size(); i++)
              HR_T.row (i) = shared.HR_trans.row (neg[i]);
            auto HR_T_view = HR_T.topRows (neg.size());
            work.triangularView<Eigen::Lower>() += HR_T_view.transpose() * HR_T_view; 
          }

          F.noalias() = llt.compute (work.triangularView<Eigen::Lower>()).solve (Mt_b);

          computed_once = true;
          old_neg = neg;

          return false;
        }

        const Eigen::VectorXd& FOD () const { return F; }


        const Shared& shared;

      protected:
        Eigen::MatrixXd work, HR_T;
        Eigen::VectorXd F, init_F, HR_amps, Mt_b;
        Eigen::LLT<Eigen::MatrixXd> llt;
        std::vector<int> neg, old_neg;
        bool computed_once;
    };


    }
  }
}

#endif


