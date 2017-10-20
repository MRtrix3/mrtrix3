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

#define DEFAULT_SCALE 1.0

using namespace MR;
using namespace App;

const char* const lossfunc[] = { "linear", "softl1", "cauchy", "arctan", NULL };


void usage ()
{
  AUTHOR = "Daan Christiaens";

  SYNOPSIS = "Detect outlier slices in DWI image.";

  DESCRIPTION
  + "This command takes DWI data and a signal prediction to calculate slice weights, "
    "using Linear, Soft-L1, Cauchy, or Arctan loss function with given scaling.";

  ARGUMENTS
  + Argument ("in", "the input DWI data.").type_image_in()
  + Argument ("pred", "the input signal prediction").type_image_in()
  + Argument ("out", "the output slice weights.").type_file_out();

  OPTIONS
  + Option ("mask", "image mask")
    + Argument ("m").type_file_in()

  + Option ("loss", "loss function (options: " + join(lossfunc, ", ") + ")")
    + Argument ("f").type_choice(lossfunc)

  + Option ("scale", "residual scaling (default = " + str(DEFAULT_SCALE) + ")")
    + Argument ("s").type_float(0.0);

}


typedef float value_type;


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
  float scale = get_option_value("scale", DEFAULT_SCALE);

  // Compute RMSE of each slice
  RMSEFunctor rmse (data, mask);
  ThreadedLoop("Computing RMSE", data, 2, 4).run(rmse, data, pred);
  Eigen::MatrixXf E = rmse.result();
  Eigen::VectorXf Em = E.array().square().rowwise().mean().sqrt();

  Eigen::MatrixXf W (E.rows(), E.cols());
  for (size_t i = 0; i < W.rows(); i++) {
    for (size_t j = 0; j < W.cols(); j++) {
      float e = E(i,j)/scale;
      float e2 = e*e;
      switch (loss) {
        case 0:
          W(i,j) = 1; break;
        case 1:
          W(i,j) = 2 * (std::sqrt(1 + e2) - 1) / e2; break;
        case 2:
          W(i,j) = std::log1p(e2) / e2; break;
        case 3:
          W(i,j) = std::atan(e2) / e2; break;
      }
    }
  }

  save_matrix(W, argument[2]);

}





