/* Copyright (c) 2017-2019 Daan Christiaens
 *
 * MRtrix and this add-on module are distributed in the hope
 * that it will be useful, but WITHOUT ANY WARRANTY; without
 * even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.
 */

#include "algo/threaded_loop.h"
#include "command.h"
#include "dwi/gradient.h"
#include "dwi/shells.h"
#include "file/matrix.h"
#include "image.h"
#include "interp/nearest.h"
#include "math/median.h"
#include "math/rng.h"

#include "dwi/svr/param.h"

using namespace MR;
using namespace App;

// clang-format off
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

  + Option ("export_error", "export RMSE matrix, scaled by the median error in each shell.")
    + Argument ("E").type_file_out()

  + DWI::GradImportOptions();

}
// clang-format on

using value_type = float;

/**
 * @brief RMSE Functor
 */
class RMSErrorFunctor {
public:
  RMSErrorFunctor(const Image<value_type> &in, const Image<bool> &mask, const Eigen::MatrixXf &mot, const int mb = 1)
      : nv(in.size(3)),
        nz(in.size(2)),
        ne(nz / mb),
        T0(in),
        mask(mask, false),
        motion(mot),
        E(new Eigen::MatrixXf(nz, nv)),
        N(new Eigen::MatrixXi(nz, nv)) {
    E->setZero();
    N->setZero();
  }

  void operator()(Image<value_type> &data, Image<value_type> &pred) {
    ssize_t const v = data.get_index(3);
    ssize_t const z = data.get_index(2);
    // Get transformation for masking. Note that the MB-factor of the motion table and the OR settings can be different.
    ssize_t const ne_mot = motion.rows() / nv;
    transform_type const T{DWI::SVR::se3exp(motion.row(v * ne_mot + z % ne_mot)).cast<double>()};
    // Calculate slice error
    value_type e = 0.0;
    int n = 0;
    Eigen::Vector3d pos;
    for (auto l = Loop(0, 2)(data, pred); l; l++) {
      if (mask.valid()) {
        assign_pos_of(data, 0, 3).to(pos);
        mask.scanner(T * T0.voxel2scanner * pos);
        if (!mask.value())
          continue;
      }
      value_type const d = data.value() - pred.value();
      e += d * d;
      n++;
    }
    (*E)(z, v) = e;
    (*N)(z, v) = n;
  }

  Eigen::MatrixXf result() const {
    Eigen::MatrixXf Emb(ne, nv);
    Emb.setZero();
    Eigen::MatrixXi Nmb(ne, nv);
    Nmb.setZero();
    for (ssize_t b = 0; b < nz / ne; b++) {
      Emb += E->block(b * ne, 0, ne, nv);
      Nmb += N->block(b * ne, 0, ne, nv);
    }
    Eigen::MatrixXf const R = (Nmb.array() > 0).select(Emb.cwiseQuotient(Nmb.cast<float>()), Eigen::MatrixXf::Zero(ne, nv));
    return R.cwiseSqrt();
  }

private:
  const ssize_t nv, nz, ne;
  const Transform T0;
  Interp::Nearest<Image<bool>> mask;
  const Eigen::MatrixXf motion;

  std::shared_ptr<Eigen::MatrixXf> E;
  std::shared_ptr<Eigen::MatrixXi> N;
};

/**
 * @brief 2-component Gaussian Mixture Model
 */
template <typename T> class GMModel {
public:
  using float_t = T;
  using VecType = Eigen::Matrix<T, Eigen::Dynamic, 1>;

  GMModel(const int max_iters = 50, const float_t eps = 1e-3, const float_t reg_covar = 1e-6)
      : niter(max_iters), tol(eps), reg(reg_covar) {}

  /**
   * Fit GMM to vector x.
   */
  void fit(const VecType &x) {
    // initialise
    init(x);
    float_t ll;
    float_t ll0 = -std::numeric_limits<float_t>::infinity();
    // Expectation-Maximization
    for (int n = 0; n < niter; n++) {
      ll = e_step(x);
      m_step(x);
      // check convergence
      if (std::fabs(ll - ll0) < tol)
        break;
      ll0 = ll;
    }
  }

  /**
   * Get posterior probability of the fit.
   */
  VecType posterior() const { return Rin.array().exp(); }

private:
  const int niter;
  const float_t tol, reg;

  float_t Min, Mout;
  float_t Sin, Sout;
  float_t Pin, Pout;
  VecType Rin, Rout;

  // initialise inlier and outlier classes.
  inline void init(const VecType &x) {
    float_t med = median(x);
    float_t mad = median(Eigen::abs(x.array() - med)) * 1.4826;
    Min = med;
    Mout = med + 1.0; // shift +1 only valid for log-Gaussians,
    Sin = mad;
    Sout = mad + 1.0; // corresp. to approx. x 3 med/mad error.
    Pin = 0.9;
    Pout = 0.1;
  }

  // E-step: update sample log-responsabilies and return log-likelohood.
  inline float_t e_step(const VecType &x) {
    Rin = log_gaussian(x, Min, Sin);
    Rin = Rin.array() + std::log(Pin);
    Rout = log_gaussian(x, Mout, Sout);
    Rout = Rout.array() + std::log(Pout);
    VecType log_prob_norm = Eigen::log(Rin.array().exp() + Rout.array().exp());
    Rin -= log_prob_norm;
    Rout -= log_prob_norm;
    return log_prob_norm.mean();
  }

  // M-step: update component mean and variance.
  inline void m_step(const VecType &x) {
    VecType w1 = Rin.array().exp() + std::numeric_limits<float_t>::epsilon();
    VecType w2 = Rout.array().exp() + std::numeric_limits<float_t>::epsilon();
    Pin = w1.mean();
    Pout = w2.mean();
    Min = average(x, w1);
    Mout = average(x, w2);
    Sin = std::sqrt(average((x.array() - Min).square(), w1) + reg);
    Sout = std::sqrt(average((x.array() - Min).square(), w2) + reg);
  }

  inline VecType log_gaussian(const VecType &x, float_t mu = 0., float_t sigma = 1.) const {
    VecType resp = x.array() - mu;
    resp /= sigma;
    resp = resp.array().square() + std::log(2 * M_PI);
    resp *= -0.5f;
    resp = resp.array() - std::log(sigma);
    return resp;
  }

  inline float_t median(const VecType &x) const {
    std::vector<float_t> vec(x.size());
    for (size_t i = 0; i < x.size(); i++)
      vec[i] = x[i];
    return Math::median(vec);
  }

  inline float_t average(const VecType &x, const VecType &w) const { return x.dot(w) / w.sum(); }
};

void run() {
  auto data = Image<value_type>::open(argument[0]);
  auto pred = Image<value_type>::open(argument[1]);
  check_dimensions(data, pred, 0, 4);

  auto mask = Image<bool>();
  auto opt = get_options("mask");
  if (!opt.empty()) {
    mask = Image<bool>::open(opt[0][0]);
    check_dimensions(data, mask, 0, 3);
  } else {
    throw Exception("mask is required.");
  }

  Eigen::MatrixXf motion(data.size(3), 6);
  motion.setZero();
  opt = get_options("motion");
  if (!opt.empty()) {
    motion = File::Matrix::load_matrix<float>(opt[0][0]);
    if (motion.cols() != 6 || ((data.size(3) * data.size(2)) % motion.rows()))
      throw Exception("dimension mismatch in motion initialisaton.");
  }

  int mb = get_option_value("mb", 1);
  if (data.size(2) % mb)
    throw Exception("Multiband factor incompatible with image dimensions.");

  auto grad = DWI::get_DW_scheme(data);
  DWI::Shells shells(grad);

  // Compute RMSE of each slice
  RMSErrorFunctor rmse(data, mask, motion, mb);
  ThreadedLoop("Computing root-mean-squared error", data, 2, 4).run(rmse, data, pred);
  Eigen::MatrixXf E = rmse.result();

  opt = get_options("export_error");
  if (!opt.empty()) {
    File::Matrix::save_matrix(E.replicate(mb, 1), opt[0][0]);
  }

  // Compute weights
  Eigen::MatrixXf W = Eigen::MatrixXf::Ones(E.rows(), E.cols());
  GMModel<value_type> gmm;
  for (size_t s = 0; s < shells.count(); s++) {
    // Log-residuals
    int k = 0;
    Eigen::VectorXf res(E.rows() * shells[s].count());
    for (size_t v : shells[s].get_volumes())
      res.segment(E.rows() * (k++), E.rows()) = E.col(v);
    // clip at non-zero minimum
    value_type nzmin = res.redux([](value_type a, value_type b) {
      if (a > 0)
        return (b > 0) ? std::min(a, b) : a;
      else
        return (b > 0) ? b : std::numeric_limits<value_type>::infinity();
    });
    Eigen::VectorXf logres = res.array().max(nzmin).log();
    // Fit GMM
    gmm.fit(logres);
    // Save posterior probabilities
    Eigen::VectorXf p = gmm.posterior();
    k = 0;
    for (size_t v : shells[s].get_volumes())
      W.col(v) = p.segment(E.rows() * (k++), E.rows());
  }

  // Output
  Eigen::ArrayXXf Wfull = 1e6 * W.replicate(mb, 1);
  Wfull = 1e-6 * Wfull.round();
  File::Matrix::save_matrix(Wfull, argument[2]);
}
