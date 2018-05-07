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
#include "dwi/gradient.h"
#include "dwi/shells.h"
#include "interp/linear.h"


using namespace MR;
using namespace App;


const char* const lossfunc[] = { "linear", "softl1", "cauchy", "arctan", "asym", NULL };


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

  + DWI::GradImportOptions();

}


using value_type = float;


class MeanErrorFunctor {
  MEMALIGN(MeanErrorFunctor)
  public:
    MeanErrorFunctor (const Image<value_type>& in, const Image<float>& mask,
                      const Eigen::MatrixXf& mot, const int mb = 1)
      : ne (in.size(2) / mb), nv (in.size(3)), T0 (in),
        mask (mask, 0.0f), motion (mot),
        E (new Eigen::MatrixXf(ne, nv)),
        N (new Eigen::MatrixXi(ne, nv)),
        S (new Eigen::MatrixXf(in.size(2), nv))
    {
      E->setZero();
      N->setZero();
      S->setOnes();
    }

    void operator() (Image<value_type>& data, Image<value_type>& pred) {
      value_type e = 0.0;
      value_type s1 = 0.0, s2 = 0.0;
      int n = 0;
      Eigen::Vector3 pos;
      for (auto l = Loop(0,2) (data, pred); l; l++) {
        if (mask.valid()) {
          assign_pos_of(data, 0, 3).to(pos);
          mask.scanner(T0.voxel2scanner * pos);
          if (mask.value() < 0.5f) continue;
        }
        value_type d = data.value() - pred.value();
        e += d;
        n++;
        s1 += data.value()*pred.value();
        s2 += data.value()*data.value();
      }
      (*E)(data.get_index(2) % ne, data.get_index(3)) += e;
      (*N)(data.get_index(2) % ne, data.get_index(3)) += n;
      if (n > 0)
        (*S)(data.get_index(2), data.get_index(3)) = s1 / s2;
    }

    Eigen::MatrixXf result() const {
      Eigen::MatrixXf R (ne, nv);
      for (size_t i = 0; i < R.rows(); i++)
        for (size_t j = 0; j < R.cols(); j++)
          R(i,j) = (*N)(i,j) ? (*E)(i,j) / (*N)(i,j) : 0.0;
      return R;
    }

    Eigen::MatrixXf imscale() const {
      return *S;
    }

  private:
    const size_t ne, nv;
    const Transform T0;
    Interp::Linear<Image<float>> mask;
    const Eigen::MatrixXf motion;

    std::shared_ptr<Eigen::MatrixXf> E;
    std::shared_ptr<Eigen::MatrixXi> N;
    std::shared_ptr<Eigen::MatrixXf> S;

};


void run ()
{
  auto data = Image<value_type>::open(argument[0]);
  auto pred = Image<value_type>::open(argument[1]);
  check_dimensions(data, pred, 0, 4);

  auto mask = Image<float>();
  auto opt = get_options("mask");
  if (opt.size()) {
    mask = Image<float>::open(opt[0][0]);
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

  // Compute RMSE of each slice
  MeanErrorFunctor rmse (data, mask, motion, mb);
  ThreadedLoop("Computing mean residual error", data, 2, 4).run(rmse, data, pred);
  Eigen::MatrixXf E = rmse.result();

  // Calculate scale
  Eigen::VectorXf scale (data.size(3));
  scale.setOnes();
  opt = get_options("scale");
  if (opt.size()) {
    scale *= float(opt[0][0]);
  }
  else {
    // Select shells
    auto grad = DWI::get_valid_DW_scheme (data);
    DWI::Shells shells (grad);
    vector<value_type> e;
    for (size_t k = 0; k < shells.count(); k++) {
      // Compute scale
      e.clear();
      for (size_t j : shells[k].get_volumes()) {
        for (size_t i = 0; i < E.rows(); i++)
          e.push_back(std::fabs(E(i,j)));
      }
      value_type s = Math::median(e) * 1.4826;
      // Update scale vector
      for (size_t i : shells[k].get_volumes())
        scale[i] = s;
    }
  }

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
        case 4:
          W(i,j) = (E(i,j) < 0) ? 1.0 / (1.0 + e2*e2) : 1.0 / std::sqrt(1.0 + e2);
          break;
      }
    }
  }

  // Output
  Eigen::ArrayXXf Wfull = W.replicate(mb, 1);
  save_matrix(Wfull, argument[2]);

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


