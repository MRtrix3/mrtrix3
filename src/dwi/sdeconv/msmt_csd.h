/* Copyright (c) 2008-2023 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Covered Software is provided under this License on an "as is"
 * basis, without warranty of any kind, either expressed, implied, or
 * statutory, including, without limitation, warranties that the
 * Covered Software is free of defects, merchantable, fit for a
 * particular purpose or non-infringing.
 * See the Mozilla Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
 */

#ifndef __dwi_sdeconv_msmt_csd_h__
#define __dwi_sdeconv_msmt_csd_h__

#include "header.h"
#include "types.h"
#include "dwi/gradient.h"
#include "dwi/shells.h"
#include "math/constrained_least_squares.h"
#include "math/math.h"
#include "math/SH.h"
#include "math/ZSH.h"

#include "dwi/directions/predefined.h"

#define DEFAULT_MSMTCSD_LMAX 8
#define DEFAULT_MSMTCSD_NORM_LAMBDA 1.0e-10
#define DEFAULT_MSMTCSD_NEG_LAMBDA 1.0e-10

namespace MR
{
  namespace DWI
  {
    namespace SDeconv
    {

      extern const App::OptionGroup MSMT_CSD_options;

      class MSMT_CSD { MEMALIGN(MSMT_CSD)
        public:

          class Shared { MEMALIGN(Shared)
            public:
              Shared (const Header& dwi_header) :
                  grad (DWI::get_DW_scheme (dwi_header)),
                  shells (grad),
                  HR_dirs (DWI::Directions::electrostatic_repulsion_300()),
                  solution_min_norm_regularisation (DEFAULT_MSMTCSD_NORM_LAMBDA),
                  constraint_min_norm_regularisation (DEFAULT_MSMTCSD_NEG_LAMBDA) { shells.select_shells(false,false,false); }


              void parse_cmdline_options()
              {
                using namespace App;
                auto opt = get_options ("lmax");
                if (opt.size())
                  lmax = parse_ints<uint32_t> (opt[0][0]);
                opt = get_options ("directions");
                if (opt.size())
                  HR_dirs = load_matrix (opt[0][0]);
                opt = get_options ("norm_lambda");
                if (opt.size())
                  solution_min_norm_regularisation = opt[0][0];
                opt = get_options ("neg_lambda");
                if (opt.size())
                  constraint_min_norm_regularisation = opt[0][0];
              }



              void set_responses (const vector<std::string>& files)
              {
                lmax_response.clear();
                for (const auto& s : files) {
                  Eigen::MatrixXd r;
                  try {
                    r = load_matrix (s);
                  } catch (Exception& e) {
                    throw Exception (e, "File \"" + s + "\" is not a valid response function file");
                  }
                  responses.push_back (std::move (r));
                }
                prepare_responses();
                response_files = files;
              }

              void set_responses (const vector<Eigen::MatrixXd>& matrices)
              {
                responses = matrices;
                prepare_responses();
              }



              void init()
              {
                if (lmax.empty()) {
                  lmax = lmax_response;
                  for (size_t t = 0; t != num_tissues(); ++t) {
                    lmax[t] = std::min (uint32_t(DEFAULT_MSMTCSD_LMAX), lmax[t]);
                  }
                } else {
                  if (lmax.size() != num_tissues())
                    throw Exception ("Number of lmaxes specified (" + str(lmax.size()) + ") does not match number of tissues (" + str(num_tissues()) + ")");
                  for (const auto i : lmax) {
                    if (i % 2)
                      throw Exception ("Each value of lmax must be a non-negative even integer");
                  }
                }

                for (size_t t = 0; t != num_tissues(); ++t) {
                  if (size_t(responses[t].rows()) != num_shells())
                    throw Exception ("number of rows in response functions must match number of b-value shells; "
                                     "number of shells is " + str(num_shells()) + ", but file \"" + response_files[t] + "\" contains " + str(responses[t].rows()) + " rows");
                  // Pad response functions out to the requested lmax for this tissue
                  responses[t].conservativeResizeLike (Eigen::MatrixXd::Zero (num_shells(), Math::ZSH::NforL (lmax[t])));
                }

                //////////////////////////////////////////////////
                // Set up the constrained least squares problem //
                //////////////////////////////////////////////////

                size_t nparams = 0;
                uint32_t maxlmax = 0;
                for (size_t i = 0; i < num_tissues(); i++) {
                  nparams += Math::SH::NforL (lmax[i]);
                  maxlmax = std::max (maxlmax, lmax[i]);
                }

                INFO ("initialising multi-tissue CSD for " + str(num_tissues()) + " tissue types, with " + str (nparams) + " parameters");

                Eigen::MatrixXd C = Eigen::MatrixXd::Zero (grad.rows(), nparams);

                vector<size_t> dwilist;
                for (size_t i = 0; i != size_t(grad.rows()); i++)
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
                for (size_t tissue_idx = 0; tissue_idx < num_tissues(); ++tissue_idx) {
                  const size_t tissue_lmax = lmax[tissue_idx];
                  const size_t tissue_n = Math::SH::NforL (tissue_lmax);
                  const size_t tissue_nmzero = tissue_lmax/2+1;

                  for (size_t shell_idx = 0; shell_idx < num_shells(); ++shell_idx) {
                    Eigen::VectorXd response_ = responses[tissue_idx].row (shell_idx);
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
                    vector<size_t> vols = shells[shell_idx].get_volumes();
                    for (size_t idx = 0; idx < vols.size(); idx++) {
                      Eigen::VectorXd SHT_(SHT.row (vols[idx]).head (tissue_n));
                      SHT_ = (SHT_.array()*fconv.array()).matrix();
                      C.row (vols[idx]).segment (pbegin, tissue_n) = SHT_;
                    }
                  }
                  pbegin += tissue_n;
                }

                vector<size_t> m (num_tissues());
                vector<size_t> n (num_tissues());
                size_t M = 0;
                size_t N = 0;

                Eigen::MatrixXd HR_SHT = Math::SH::init_transform (HR_dirs, maxlmax);

                for (size_t i = 0; i != num_tissues(); i++) {
                  if (lmax[i] > 0)
                    m[i] = HR_dirs.rows();
                  else
                    m[i] = 1;
                  M += m[i];
                  n[i] = Math::SH::NforL (lmax[i]);
                  N += n[i];
                }

                Eigen::MatrixXd A (Eigen::MatrixXd::Zero (M, N));
                size_t b_m = 0; size_t b_n = 0;
                for (size_t i = 0; i != num_tissues(); i++) {
                  A.block (b_m,b_n,m[i],n[i]) = HR_SHT.block (0,0,m[i],n[i]);
                  b_m += m[i];
                  b_n += n[i];
                }
                problem = Math::ICLS::Problem<double> (C, A, Eigen::VectorXd(), 0,
                  solution_min_norm_regularisation, constraint_min_norm_regularisation);

                INFO ("Multi-shell, multi-tissue CSD initialised successfully");
              }


              size_t num_shells()  const { return shells.count(); }
              size_t num_tissues() const { return responses.size(); }


              const Eigen::MatrixXd grad;
              DWI::Shells shells;
              Eigen::MatrixXd HR_dirs;
              vector<uint32_t> lmax, lmax_response;
              vector<Eigen::MatrixXd> responses;
              vector<std::string> response_files;
              Math::ICLS::Problem<double> problem;
              double solution_min_norm_regularisation, constraint_min_norm_regularisation;


            private:

              void prepare_responses() {
                for (size_t t = 0; t != num_tissues(); ++t) {
                  Eigen::MatrixXd& r (responses[t]);
                  size_t n = 0;
                  for (size_t row = 0; row < size_t(r.rows()); row++) {
                    for (size_t col = 0; col < size_t(r.cols()); col++) {
                      if (r(row,col))
                        n = std::max (n, col+1);
                    }
                  }
                  // Clip off any empty columns, i.e. degrees containing zero coefficients for all shells
                  r.conservativeResize (r.rows(), n);
                  // Store the lmax for each tissue based on their response functions;
                  //   if the user doesn't manually specify lmax, these will determine the
                  //   lmax of each tissue ODF output, with a further default lmax=8
                  //   restriction at that stage
                  lmax_response.push_back (Math::ZSH::LforN (r.cols()));
                }
              }

          };




          MSMT_CSD (const Shared& shared_data) :
              niter (0),
              shared (shared_data),
              solver (shared.problem) { }

          void operator() (const Eigen::VectorXd& data, Eigen::VectorXd& output) {
            niter = solver (output, data);
          }

          size_t niter;
          const Shared& shared;

        private:
          Math::ICLS::Solver<double> solver;


      };



    }
  }
}

#endif


