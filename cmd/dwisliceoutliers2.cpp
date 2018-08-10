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


void usage ()
{
  AUTHOR = "Daan Christiaens (daan.christiaens@kcl.ac.uk)";

  SYNOPSIS = "Detect and reweigh outlier slices in DWI image.";

  DESCRIPTION
  + "This command takes DWI data and a signal prediction to calculate "
    "slice inlier probabilities using Bayesian GMM modelling.";

  ARGUMENTS
  + Argument ("in", "the input DWI data.").type_image_in()
  + Argument ("pred", "the input signal prediction").type_image_in()
  + Argument ("out", "the output slice weights.").type_file_out();

  OPTIONS
  + Option ("mb", "multiband factor (default = 1)")
    + Argument ("f").type_integer(1)

  + Option ("mask", "image mask")
    + Argument ("m").type_image_in()

  + Option ("motion", "rigid motion parameters (used for masking)")
    + Argument ("param").type_file_in()

  + DWI::GradImportOptions();

}


using value_type = float;


class RMSErrorFunctor {
  MEMALIGN(RMSErrorFunctor)
  public:
    RMSErrorFunctor (const Image<value_type>& in, const Image<bool>& mask,
                     const Eigen::MatrixXf& mot, const int mb = 1)
      : nv (in.size(3)), nz (in.size(2)), ne (nz / mb),
        T0(in), mask (mask, false), motion (mot),
        E (new Eigen::MatrixXf(nz, nv)),
        N (new Eigen::MatrixXi(nz, nv))
    {
      E->setZero();
      N->setZero();
    }

    void operator() (Image<value_type>& data, Image<value_type>& pred) {
      size_t v = data.get_index(3);
      size_t z = data.get_index(2);
      // Get transformation for masking. Note that the MB-factor of the motion table and the OR settings can be different.
      size_t ne_mot = motion.rows() / nv;
      transform_type T { DWI::SVR::se3exp(motion.row(v*ne_mot + z%ne_mot)).cast<double>() };
      // Calculate slice error
      value_type e = 0.0;
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
      }
      (*E)(z, v) = e;
      (*N)(z, v) = n;
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

  private:
    const size_t nv, nz, ne;
    const Transform T0;
    Interp::Nearest<Image<bool>> mask;
    const Eigen::MatrixXf motion;

    std::shared_ptr<Eigen::MatrixXf> E;
    std::shared_ptr<Eigen::MatrixXi> N;

    static std::mutex mx;

};

std::mutex RMSErrorFunctor::mx;


class GMModel {
  MEMALIGN(GMModel)
  public:
    GMModel (const int niters = 10) : niter (niters) { }

    void fit(const Eigen::VectorXf& x) {
      init(x);
      float eps = 1e-5;
      for (int n = 0; n < niter; n++) {
        // E-Step
        p1 = gaussian(x, Min, Sin) + eps*ones;
        p2 = gaussian(x, Mout, Sout) + eps*ones;
        w = Pin * p1.cwiseQuotient(Pin * p1 + Pout * p2);
        // M-Step
        Pin = w.sum();
        Pout = (ones - w).sum();
        Min = x.dot(w) / Pin; 
        Mout = x.dot(ones - w) / Pout;
        Sin = std::sqrt( (x.array() - Min).square().matrix().dot(w) / Pin );
        Sout = std::sqrt( (x.array() - Mout).square().matrix().dot(ones - w) / Pout );
      }
    }

    Eigen::VectorXf get_prob() const { return w; }

  private:
    const int niter;
    float Min, Mout;
    float Sin, Sout;
    float Pin, Pout;
    Eigen::VectorXf p1, p2, w, ones;

    void init(const Eigen::VectorXf& x) {
      ones.resizeLike(x); ones.setOnes();
      float med = median(x);
      float mad = median(Eigen::abs(x.array() - med)) * 1.4826;
      Min = med; Mout = 3*med;
      Sin = mad; Sout = 3*mad;
      Pin = 0.9; Pout = 0.1;
    }

    Eigen::VectorXf gaussian(const Eigen::VectorXf& x, float mu = 0., float std = 1.) const {
      return Eigen::exp((x.array() - mu).square() / (-2*std*std)) / (std::sqrt(2*M_PI) * std);
    }

    float median(const Eigen::VectorXf& x) const {
      vector<float> vec (x.size());
      for (size_t i = 0; i < x.size(); i++)
        vec[i] = x[i];
      return Math::median(vec);
    }
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

  int mb = get_option_value("mb", 1);
  if (data.size(2) % mb)
    throw Exception ("Multiband factor incompatible with image dimensions.");

  auto grad = DWI::get_valid_DW_scheme (data);
  DWI::Shells shells (grad);

  // Compute RMSE of each slice
  RMSErrorFunctor rmse (data, mask, motion, mb);
  ThreadedLoop("Computing root-mean-squared error", data, 2, 4).run(rmse, data, pred);
  Eigen::MatrixXf E = rmse.result();
  
  // Compute weights
  Eigen::MatrixXf W = Eigen::MatrixXf::Ones(E.rows(), E.cols());
  GMModel gmm;
  for (size_t s = 0; s < shells.count(); s++) {
    int k = 0;
    Eigen::VectorXf r2 (E.rows() * shells[s].count());
    for (size_t v : shells[s].get_volumes())
      r2.segment(E.rows() * (k++), E.rows()) = E.col(v); //.array().square();
    gmm.fit(r2);
    Eigen::VectorXf p = gmm.get_prob();
    k = 0;
    for (size_t v : shells[s].get_volumes())
      W.col(v) = p.segment(E.rows() * (k++), E.rows());
  }

  // Output
  Eigen::ArrayXXf Wfull = W.replicate(mb, 1);
  save_matrix(Wfull, argument[2]);

}


