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

#define DEFAULT_THR 3.0

using namespace MR;
using namespace App;

void usage ()
{
  AUTHOR = "Daan Christiaens";

  SYNOPSIS = "Detect outlier slices in DWI image.";

  DESCRIPTION
  + "This command takes DWI data and a signal prediction to detect slices "
    "with RMS error beyond 3.0 (default) times the average RMSE.";

  ARGUMENTS
  + Argument ("in", "the input DWI data.").type_image_in()
  + Argument ("pred", "the input signal prediction").type_image_in()
  + Argument ("out", "the output slice weights.").type_file_out();

  OPTIONS
  + Option ("mask", "image mask")
    + Argument ("m").type_file_in()
  + Option ("thr", "RMSE threshold (default = " + str(DEFAULT_THR) + ")")
    + Argument ("t").type_float(0.0, 1e3);

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
      for (auto l = Loop(0,2) (data, pred); l; l++, n++) {
        value_type d = data.value() - pred.value();
        e += d * d;
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

  float thr = get_option_value("thr", DEFAULT_THR);

  // Compute RMSE of each slice
  RMSEFunctor rmse (data, mask);
  ThreadedLoop("Computing RMSE", data, 2, 4).run(rmse, data, pred);
  Eigen::MatrixXf E = rmse.result();

  Eigen::VectorXf Em = E.rowwise().mean();
  E.array().colwise() /= Em.array();

  Eigen::MatrixXf W (E.rows(), E.cols());
  for (size_t i = 0; i < W.rows(); i++) {
    for (size_t j = 0; j < W.cols(); j++) {
      W(i,j) = (E(i,j) < thr*Em[i]) ? 1.0 : 0.0;
    }
  }

  save_matrix(W, argument[2]);

}





