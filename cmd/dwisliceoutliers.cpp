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

#include "command.h"
#include "image.h"
#include "algo/threaded_loop.h"
#include "math/median.h"
#include "math/rng.h"
#include "dwi/gradient.h"
#include "dwi/shells.h"
#include "interp/nearest.h"

#include "dwi/svr/param.h"


using namespace MR;
using namespace App;


const char* const lossfunc[] = { "linear", "softl1", "cauchy", "arctan", NULL };


void usage ()
{
  AUTHOR = "Daan Christiaens (daan.christiaens@kcl.ac.uk)";

  SYNOPSIS = "Detect and reweigh outlier slices in DWI image.";

  DESCRIPTION
  + "This command takes DWI data and a signal prediction to calculate slice "
    "weights, using Linear, Soft-L1 (default), Cauchy, or Arctan loss functions."

  + "Unless set otherwise, the errors are scaled with a robust estimate "
    "of standard error based on the median absolute deviation (MAD).";

  ARGUMENTS
  + Argument ("in", "the input DWI data.").type_image_in()
  + Argument ("pred", "the input signal prediction").type_image_in()
  + Argument ("out", "the output slice weights.").type_file_out();

  OPTIONS
  + Option ("loss", "loss function (options: " + join(lossfunc, ", ") + ")")
    + Argument ("f").type_choice(lossfunc)

  + Option ("scale", "residual scaling (default = 1.4826 * MAD per shell)")
    + Argument ("s").type_float(0.0)

  + Option ("mb", "multiband factor (default = 1)")
    + Argument ("f").type_integer(1)

  + Option ("mask", "image mask")
    + Argument ("m").type_image_in()

  + Option ("motion", "rigid motion parameters (used for masking)")
    + Argument ("param").type_file_in()

  + Option ("imscale", "intensity matching scale output")
    + Argument ("s").type_image_out()

  + Option ("export_error", "export RMSE matrix, scaled by the median error in each shell.")
    + Argument ("E").type_file_out()

  + DWI::GradImportOptions();

}


using value_type = float;


class RMSErrorFunctor {
  MEMALIGN(RMSErrorFunctor)
  public:
    RMSErrorFunctor (const Image<value_type>& in, const Image<bool>& mask,
                     const DWI::Shells& shells, const Eigen::MatrixXf& mot, const int mb = 1)
      : nv (in.size(3)), nz (in.size(2)), ne (nz / mb), T0 (in),
        mask (mask, false), shells (shells), motion (mot),
        E (new Eigen::MatrixXf(nz, nv)),
        N (new Eigen::MatrixXi(nz, nv)),
        S (new Eigen::MatrixXf(nz, nv)),
        sample (new vector<vector<float>>(shells.count()))
    {
      E->setZero();
      N->setZero();
      S->setOnes();
    }

    void operator() (Image<value_type>& data, Image<value_type>& pred) {
      size_t v = data.get_index(3);
      size_t z = data.get_index(2);
      size_t bidx = get_shell_idx(v);
      // Get transformation for masking. Note that the MB-factor of the motion table and the OR settings can be different.
      size_t ne_mot = motion.rows() / nv;
      transform_type T { DWI::SVR::se3exp(motion.row(v*ne_mot + z%ne_mot)).cast<double>() };
      // Calculate slice error
      value_type e = 0.0;
      value_type s1 = 0.0, s2 = 0.0;
      int n = 0;
      Eigen::Vector3 pos;
      for (auto l = Loop(0,2) (data, pred); l; l++) {
        if (mask.valid()) {
          assign_pos_of(data, 0, 3).to(pos);
          mask.scanner(T * T0.voxel2scanner * pos);
          if (!mask.value()) continue;
        }
        value_type d = data.value() - pred.value();
        e += d * d;
        n++;
        s1 += data.value()*pred.value();
        s2 += data.value()*data.value();
        if (uniform() < 0.01f) {
          std::lock_guard<std::mutex> lock (mx);
          sample->at(bidx).push_back(std::fabs(d));
        }
      }
      (*E)(z, v) = e;
      (*N)(z, v) = n;
      if (n > 0)
        (*S)(z, v) = s1 / s2;
    }

    Eigen::VectorXf scale() {
      Eigen::VectorXf s (nv);
      for (size_t k = 0; k < shells.count(); k++) {
        float stdev = Math::median(sample->at(k)) * 1.4826;
        for (size_t i : shells[k].get_volumes())
          s[i] = stdev;
      }
      return s;
    }

    Eigen::MatrixXf result() const {
      Eigen::MatrixXf Emb (ne, nv); Emb.setZero();
      Eigen::MatrixXi Nmb (ne, nv); Nmb.setZero();
      for (size_t b = 0; b < nz/ne; b++) {
        Emb += E->block(b*ne,0,ne,nv);
        Nmb += N->block(b*ne,0,ne,nv);
      }
      Eigen::MatrixXf R = (Nmb.array() > 0).select( Emb.cwiseQuotient(Nmb.cast<float>()) , Eigen::MatrixXf::Zero(ne, nv) );
      return R.cwiseSqrt();
    }

    Eigen::MatrixXf imscale() const {
      return *S;
    }

  private:
    const size_t nv, nz, ne;
    const Transform T0;
    Interp::Nearest<Image<bool>> mask;
    const DWI::Shells shells;
    const Eigen::MatrixXf motion;

    std::shared_ptr<Eigen::MatrixXf> E;
    std::shared_ptr<Eigen::MatrixXi> N;
    std::shared_ptr<Eigen::MatrixXf> S;

    std::shared_ptr< vector<vector<float>> > sample;
    Math::RNG::Uniform<float> uniform;
    static std::mutex mx;

    inline size_t get_shell_idx(const size_t v) const {
      for (size_t k = 0; k < shells.count(); k++) {
        for (size_t i : shells[k].get_volumes()) {
          if (i == v) return k;
        }
      }
      return 0;
    }

};

std::mutex RMSErrorFunctor::mx;


void run ()
{
  auto data = Image<value_type>::open(argument[0]);
  auto pred = Image<value_type>::open(argument[1]);
  check_dimensions(data, pred, 0, 4);

  auto mask = Image<bool>();
  auto opt = get_options("mask");
  if (opt.size()) {
    mask = Image<bool>::open(opt[0][0]);
    check_dimensions(data, mask, 0, 3);
  } else {
    throw Exception ("mask is required.");
  }

  Eigen::MatrixXf motion (data.size(3), 6); motion.setZero();
  opt = get_options("motion");
  if (opt.size()) {
    motion = load_matrix<float>(opt[0][0]);
    if (motion.cols() != 6 || ((data.size(3)*data.size(2)) % motion.rows()))
      throw Exception("dimension mismatch in motion initialisaton.");
  }

  int loss = get_option_value("loss", 1);

  int mb = get_option_value("mb", 1);
  if (data.size(2) % mb)
    throw Exception ("Multiband factor incompatible with image dimensions.");

  auto grad = DWI::get_valid_DW_scheme (data);
  DWI::Shells shells (grad);

  // Compute RMSE of each slice
  RMSErrorFunctor rmse (data, mask, shells, motion, mb);
  ThreadedLoop("Computing root-mean-squared error", data, 2, 4).run(rmse, data, pred);
  Eigen::MatrixXf E = rmse.result();
  Eigen::VectorXf scale = rmse.scale();
  scale *= get_option_value("scale", 1.0f);

  // Compute weights
  Eigen::MatrixXf W (E.rows(), E.cols());
  for (size_t i = 0; i < W.rows(); i++) {
    for (size_t j = 0; j < W.cols(); j++) {
      value_type e2 = Math::pow2( E(i,j)/scale[j] );
      switch (loss) {
        case 0:
          W(i,j) = 1.0;
          break;
        case 1:
          W(i,j) = 1.0 / std::sqrt(1.0 + e2);
          break;
        case 2:
          W(i,j) = 1.0 / (1.0 + e2);
          break;
        case 3:
          W(i,j) = 1.0 / (1.0 + e2*e2);
          break;
      }
    }
  }

  // Output
  Eigen::ArrayXXf Wfull = W.replicate(mb, 1);
  save_matrix(Wfull, argument[2]);

  opt = get_options("export_error");
  if (opt.size()) {
    save_matrix(E.cwiseQuotient(scale.replicate(E.rows(), 1)).replicate(mb, 1), opt[0][0]);
  }

  // Image scale output
  opt = get_options("imscale");
  if (opt.size()) {
    Eigen::ArrayXXf S = rmse.imscale().array();
    Eigen::ArrayXXf logS = S.abs().log();
    logS.rowwise() -= (Wfull * logS).colwise().sum() / Wfull.colwise().sum();
    S = logS.exp();
    // save as image
    Header header (data);
    header.size(0) = 1;
    header.size(1) = 1;
    auto imscale = Image<value_type>::create(opt[0][0], header);
    for (size_t i = 0; i < S.rows(); i++) {
      imscale.index(2) = i;
      for (size_t j = 0; j < S.cols(); j++) {
        imscale.index(3) = j;
        imscale.value() = S(i,j);
      }
    }
  }

}


