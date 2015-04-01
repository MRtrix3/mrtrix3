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

#ifndef __dwi_sdeconv_constrained_h__
#define __dwi_sdeconv_constrained_h__

#include "app.h"
#include "image/header.h"
#include "dwi/gradient.h"
#include "math/SH.h"
#include "dwi/directions/predefined.h"
#include "math/least_squares.h"

#define NORM_LAMBDA_MULTIPLIER 0.0002

namespace MR
{
  namespace DWI
  {

    extern const App::OptionGroup CSD_options;

    template <typename T> class CSDeconv
    {
      public:
        typedef T value_type;

        class Shared
        {
          public:

            Shared (const Image::Header& dwi_header) :
                neg_lambda (1.0),
                norm_lambda (1.0),
                threshold (0.0),
                niter (50)
            {
              grad = DWI::get_valid_DW_scheme<value_type> (dwi_header);
              // Discard b=0 (b=0 normalisation not supported in this version)
              // Only allow selection of one non-zero shell from command line
              dwis = DWI::Shells (grad).select_shells (false, true).largest().get_volumes();
              DW_dirs = DWI::gen_direction_matrix (grad, dwis);

              lmax = 8;
              lmax_data = Math::SH::LforN (dwis.size());

              DWI::Directions::electrostatic_repulsion_300 (HR_dirs);

              init_filter.allocate (lmax/2 + 1);
              init_filter.zero();
              for (size_t i = 0; i < std::min (size_t(3), init_filter.size()); ++i)
                init_filter[i] = 1.0;
            }


            void parse_cmdline_options()
            {
              using namespace App;
              Options opt = get_options ("lmax");
              if (opt.size())
                lmax = opt[0][0];
              opt = get_options ("filter");
              if (opt.size())
                init_filter.load (opt[0][0]);
              opt = get_options ("directions");
              if (opt.size())
                HR_dirs.load (opt[0][0]);
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
              response.load (path);

              lmax_data = 2*(response.size()-1);
              INFO ("setting response function using even SH coefficients: " + str (response));
            }

            void set_response (const Math::Vector<float>& in)
            {
              response = in;
              lmax_data = 2*(response.size()-1);
            }


            void init ()
            {
              assert (response.size());
              if (lmax_data > lmax) lmax_data = lmax;
              INFO ("calculating even spherical harmonic components up to order " + str (lmax_data) + " for initialisation");

              if (init_filter.size() < size_t (lmax_data/2) +1)
                throw Exception ("not enough initial filter coefficients supplied for lmax = " + str (lmax_data));

              Math::Vector<value_type> RH;
              Math::SH::SH2RH (RH, response);
              RH.resize (1+lmax/2,0);

              // inverse sdeconv for initialisation:
              Math::Matrix<value_type> fconv;
              Math::SH::init_transform (fconv, DW_dirs, lmax_data);
              rconv.allocate (fconv.columns(), fconv.rows());
              fconv.diagonal() += 1.0e-2;
              //fconv.save ("fconv.txt");
              Math::pinv (rconv, fconv);
              //rconv.save ("rconv.txt");
              size_t l = 0, nl = 1;
              for (size_t row = 0; row < rconv.rows(); ++row) {
                if (row >= nl) {
                  l++;
                  nl = Math::SH::NforL (2*l);
                }
                for (size_t col = 0; col < rconv.columns(); col++) 
                  rconv (row, col) *= init_filter[l] / RH[l];
              }

              // forward sconv for iteration, using all response function
              // coefficients up to the requested lmax:
              Math::SH::init_transform (fconv, DW_dirs, lmax);
              l = 0;
              nl = 1;
              for (size_t col = 0; col < fconv.columns(); ++col) {
                if (col >= nl) {
                  l++;
                  nl = Math::SH::NforL (2*l);
                }
                for (size_t row = 0; row < fconv.rows(); ++row) 
                  fconv (row,col) *= RH[l];
              }

              // high-res sampling to apply constraint:
              Math::SH::init_transform (HR_trans, HR_dirs, lmax);
              value_type constraint_multiplier = neg_lambda * 50.0 * response[0] / value_type (HR_trans.rows());
              HR_trans *= constraint_multiplier;

              // adjust threshold accordingly:
              threshold *= constraint_multiplier;

              // precompute as much as possible ahead of Cholesky decomp:
              assert (fconv.columns() <= HR_trans.columns());
              M.allocate (DW_dirs.rows(), HR_trans.columns());
              M.sub (0,M.rows(),0,fconv.columns()) = fconv;
              M.sub (0,M.rows(),fconv.columns(),M.columns()) = 0.0;
              Mt_M.allocate (M.columns(), M.columns());
              rankN_update (Mt_M, M, CblasTrans, CblasLower, value_type (1.0), value_type (0.0));

              INFO ("constrained spherical deconvolution initialised successfully");
            }

            size_t nSH () const {
              return HR_trans.columns();
            }

            Math::Matrix<value_type> grad;
            Math::Vector<value_type> response, init_filter;
            Math::Matrix<value_type> DW_dirs, HR_dirs;
            Math::Matrix<value_type> rconv, HR_trans, M, Mt_M;
            value_type neg_lambda, norm_lambda, threshold;
            std::vector<size_t> dwis;
            int lmax_data, lmax;
            size_t niter;
        };


        CSDeconv (const Shared& shared_data) :
          shared (shared_data),
          work (shared.Mt_M.rows(), shared.Mt_M.columns()),
          HR_T (shared.HR_trans.rows(), shared.HR_trans.columns()),
          F (shared.HR_trans.columns()),
          init_F (shared.rconv.rows()),
          HR_amps (shared.HR_trans.rows()),
          Mt_b (shared.HR_trans.columns()),
          old_neg (shared.HR_trans.rows()),
          computed_once (false) {
            norm_lambda = NORM_LAMBDA_MULTIPLIER * shared.norm_lambda * shared.Mt_M (0,0);
          }

        CSDeconv (const CSDeconv& c) :
          shared (c.shared),
          work (shared.Mt_M.rows(), shared.Mt_M.columns()),
          HR_T (shared.HR_trans.rows(), shared.HR_trans.columns()),
          F (shared.HR_trans.columns()),
          init_F (shared.rconv.rows()),
          HR_amps (shared.HR_trans.rows()),
          Mt_b (shared.HR_trans.columns()),
          old_neg (shared.HR_trans.rows()),
          computed_once (false) {
            norm_lambda = NORM_LAMBDA_MULTIPLIER * shared.norm_lambda * shared.Mt_M (0,0);
          }

        ~CSDeconv() { }

        void set (const Math::Vector<value_type>& DW_signals) {
          Math::mult (init_F, shared.rconv, DW_signals);
          F.sub (0, init_F.size()) = init_F;
          F.sub (init_F.size(), F.size()) = 0.0;
          old_neg.assign (shared.HR_trans.rows(), -1);
          computed_once = false;

          mult (Mt_b, value_type (0.0), value_type (1.0), CblasTrans, shared.M, DW_signals);
        }

        bool iterate() {
          neg.clear();
          Math::mult (HR_amps, shared.HR_trans, F);
          for (size_t n = 0; n < HR_amps.size(); n++)
            if (HR_amps[n] < shared.threshold)
              neg.push_back (n);

          if (computed_once && old_neg == neg)
            if (old_neg == neg)
              return true;

          for (size_t i = 0; i < work.rows(); i++) 
            for (size_t j = 0; j <= i; j++)
              work (i,j) = shared.Mt_M (i,j);

          // min-norm constraint:
          if (norm_lambda) {
#ifndef USE_NON_ORTHONORMAL_SH_BASIS
            work.diagonal() += norm_lambda;
#else
            int l = 0;
            for (size_t i = 0; i < work.rows(); ++i) {
              if (Math::SH::index (l,0) == i) {
                work(i,i) += norm_lambda;
                l+=2;
              }
              else 
                work(i,i) += 0.5 * norm_lambda;
            }
#endif
          }

          if (neg.size()) {
            HR_T.allocate (neg.size(), shared.HR_trans.columns());
            for (size_t i = 0; i < neg.size(); i++)
              HR_T.row (i) = shared.HR_trans.row (neg[i]);
            rankN_update (work, HR_T, CblasTrans, CblasLower, value_type (1.0), value_type (1.0));
          }

          F = Mt_b;

          Math::Cholesky::decomp (work);
          Math::Cholesky::solve (F, work);
          computed_once = true;
          old_neg = neg;

          return false;
        }

        const Math::Vector<value_type>& FOD () const         {
          return F;
        }


        const Shared& shared;

      protected:
        value_type norm_lambda;
        Math::Matrix<value_type> work, HR_T;
        Math::Vector<value_type> F, init_F, HR_amps, Mt_b;
        std::vector<int> neg, old_neg;
        bool computed_once;
    };


  }
}

#endif


