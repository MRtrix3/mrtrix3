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


using namespace MR;
using namespace App;


const char* const lossfunc[] = { "linear", "softl1", "cauchy", "arctan", NULL };

static constexpr float EPS = std::numeric_limits<float>::epsilon();


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
  + Option ("mask", "image mask")
    + Argument ("m").type_file_in()

  + Option ("loss", "loss function (options: " + join(lossfunc, ", ") + ")")
    + Argument ("f").type_choice(lossfunc)

  + Option ("scale", "residual scaling (default = 1.4826 * MAD per shell)")
    + Argument ("s").type_float(0.0)

  + DWI::GradImportOptions();

}


using value_type = float;


class RMSEFunctor {
  MEMALIGN(RMSEFunctor)
  public:
    RMSEFunctor (const Image<value_type>& in, const Image<bool>& mask)
      : mask (mask),
        E (new Eigen::Matrix<value_type, Eigen::Dynamic, Eigen::Dynamic>(in.size(2), in.size(3)))
    {
      E->setZero();
    }

    void operator() (Image<value_type>& data, Image<value_type>& pred) {
      value_type e = 0.0;
      size_t n = 0;
      for (auto l = Loop(0,2) (data, pred); l; l++) {
        if (mask.valid()) {
          assign_pos_of(data).to(mask);
          if (!mask.value()) continue;
        }
        value_type d = data.value() - pred.value();
        e += d * d;
        n++;
      }
      if (n > 0)
        (*E)(data.get_index(2), data.get_index(3)) = std::sqrt(e/n);
    }

    Eigen::Matrix<value_type, Eigen::Dynamic, Eigen::Dynamic> result() const { return *E; }

  private:
    Image<bool> mask;
    std::shared_ptr<Eigen::Matrix<value_type, Eigen::Dynamic, Eigen::Dynamic> > E;

};


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
  }

  int loss = get_option_value("loss", 1);

  // Compute RMSE of each slice
  RMSEFunctor rmse (data, mask);
  ThreadedLoop("Computing RMSE", data, 2, 4).run(rmse, data, pred);
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
          e.push_back(E(i,j));
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
      float e2 = Math::pow2( E(i,j)/scale[j] );
      switch (loss) {
        case 0:
          W(i,j) = 1.0;
          break;
        case 1:
          W(i,j) = (e2 <= EPS) ? 0 : 2 * (std::sqrt(1.0 + e2) - 1.0) / e2;
          break;
        case 2:
          W(i,j) = (e2 <= EPS) ? 0 : std::log1p(e2) / e2;
          break;
        case 3:
          W(i,j) = (e2 <= EPS) ? 0 : std::atan(e2) / e2;
          break;
      }
    }
  }

  save_matrix(W, argument[2]);

}


