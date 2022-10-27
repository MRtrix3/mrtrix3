/* Copyright (c) 2008-2022 the MRtrix3 contributors.
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

#include "command.h"
#include "phase_encoding.h"
#include "progressbar.h"
#include "image.h"
#include "algo/threaded_copy.h"
#include "dwi/gradient.h"
#include "dwi/tensor.h"
#include "dwi/directions/predefined.h"
#include "math/constrained_least_squares.h"

using namespace MR;
using namespace App;

using value_type = float;

#define DEFAULT_NITER 2

const char* const encoding_description[] = {
  "The tensor coefficients are stored in the output image as follows:\n"
  "volumes 0-5: D11, D22, D33, D12, D13, D23",
  "If diffusion kurtosis is estimated using the -dkt option, these are stored as follows:\n"
  "volumes 0-2: W1111, W2222, W3333\n"
  "volumes 3-8: W1112, W1113, W1222, W1333, W2223, W2333\n"
  "volumes 9-11: W1122, W1133, W2233\n"
  "volumes 12-14: W1123, W1223, W1233",
  nullptr
};


void usage ()
{
  AUTHOR = "Ben Jeurissen (ben.jeurissen@uantwerpen.be)";

  SYNOPSIS = "Diffusion (kurtosis) tensor estimation";

  DESCRIPTION
  + "By default, the diffusion tensor (and optionally the kurtosis tensor) is fitted to "
    "the log-signal in two steps: firstly, using weighted least-squares (WLS) with "
    "weights based on the empirical signal intensities; secondly, by further iterated weighted "
    "least-squares (IWLS) with weights determined by the signal predictions from the "
    "previous iteration (by default, 2 iterations will be performed). This behaviour can "
    "be altered in two ways:"

  + "* The -ols option will cause the first fitting step to be performed using ordinary "
    "least-squares (OLS); that is, all measurements contribute equally to the fit, instead of "
    "the default behaviour of weighting based on the empirical signal intensities."

  + "* The -iter option controls the number of iterations of the IWLS prodedure. If this is "
    "set to zero, then the output model parameters will be those resulting from the first "
    "fitting step only: either WLS by default, or OLS if the -ols option is used in conjunction "
    "with -iter 0."

  + "By default, the diffusion tensor (and optionally the kurtosis tensor) is fitted using "
    "unconstrained optimization. This can result in unexpected diffusion parameters, "
    "e.g. parameters that represent negative apparent diffusivities or negative apparent kurtoses, "
    "or parameters that correspond to non-monotonic decay of the predicted signal. "
    "By supplying the -constrain option, constrained optimization is performed instead "
    "and such physically implausible parameters can be avoided. Depending on the presence "
    " of the -dkt option, the -constrain option will enforce the following constraints:"

  + "* Non-negative apparent diffusivity (always)."

  + "* Non-negative apparent kurtosis (when the -dkt option is provided)."

  + "* Monotonic signal decay in the b = [0 b_max] range (when the -dkt option is provided)."

    + encoding_description;

  ARGUMENTS
    + Argument ("dwi", "the input dwi image.").type_image_in ()
    + Argument ("dt", "the output dt image.").type_image_out ();

  OPTIONS
    + Option ("ols", "perform initial fit using an ordinary least-squares (OLS) fit (see Description).")

    + Option ("iter","number of iterative reweightings for IWLS algorithm (default: "
              + str(DEFAULT_NITER) + ") (see Description).")
    +   Argument ("integer").type_integer (0, 10)

    + Option ("constrain", "constrain fit to non-negative diffusivity and kurtosis as well as monotonic signal decay (see Description).")

    + Option ("mask", "only perform computation within the specified binary brain mask image.")
    +   Argument ("image").type_image_in()

    + Option ("b0", "the output b0 image.")
    +   Argument ("image").type_image_out()

    + Option ("dkt", "the output dkt image.")
    +   Argument ("image").type_image_out()

    + Option ("predicted_signal", "the predicted dwi image.")
    +   Argument ("image").type_image_out()

    + DWI::GradImportOptions();

  REFERENCES
   + "References based on fitting algorithm used:"

   + "* OLS, WLS:\n"
     "Basser, P.J.; Mattiello, J.; LeBihan, D. "
     "Estimation of the effective self-diffusion tensor from the NMR spin echo. "
     "J Magn Reson B., 1994, 103, 247–254."

   + "* IWLS:\n"
     "Veraart, J.; Sijbers, J.; Sunaert, S.; Leemans, A. & Jeurissen, B. " // Internal
     "Weighted linear least squares estimation of diffusion MRI parameters: strengths, limitations, and pitfalls. "
     "NeuroImage, 2013, 81, 335-346"
  
   + "* any of above with constraints:\n"
     "Morez, J.; Szczepankiewicz, F; den Dekker, A. J.; Vanhevel, F.; Sijbers, J. &  Jeurissen, B. " // Internal
     "Optimal experimental design and estimation for q-space trajectory imaging. "
     "Human Brain Mapping, In press";
}



template <class MASKType, class B0Type, class DTType, class DKTType, class PredictType>
class Processor { MEMALIGN(Processor)
  public:
    Processor (const Eigen::MatrixXd& A, const Eigen::MatrixXd& Aneq, const bool ols, const int iter,
        const MASKType& mask_image, const B0Type& b0_image, const DTType& dt_image, const DKTType& dkt_image, const PredictType& predict_image) :
      mask_image (mask_image),
      b0_image (b0_image),
      dt_image (dt_image),
      dkt_image (dkt_image),
      predict_image (predict_image),
      dwi(A.rows()),
      x(A.cols()),
      w(Eigen::VectorXd::Ones (A.rows())),
      work(A.cols(),A.cols()),
      llt(work.rows()),
      A(A),
      Aneq(Aneq),
      ols (ols),
      maxit(iter) { }

    template <class DWIType>
      void operator() (DWIType& dwi_image)
      {
        if (mask_image.valid()) {
          assign_pos_of (dwi_image, 0, 3).to (mask_image);
          if (!mask_image.value())
            return;
        }

        for (auto l = Loop (3) (dwi_image); l; ++l)
          dwi[dwi_image.index(3)] = dwi_image.value();

        double small_intensity = 1.0e-6 * dwi.maxCoeff();
        for (int i = 0; i < dwi.rows(); i++) {
          if (dwi[i] < small_intensity)
            dwi[i] = small_intensity;
          w[i] = ( ols ? 1.0 : dwi[i] );
          dwi[i] = std::log (dwi[i]);
        }

        for (int it = 0; it <= maxit; it++) {
          if (Aneq.rows() > 0) {
            auto problem = Math::ICLS::Problem<double> (w.asDiagonal()*A, Aneq);
            Math::ICLS::Solver<double> solver (problem);
            solver (x, w.asDiagonal()*dwi);
          } else {
            work.setZero();
            work.selfadjointView<Eigen::Lower>().rankUpdate (A.transpose()*w.asDiagonal());
            x = llt.compute (work.selfadjointView<Eigen::Lower>()).solve(A.transpose()*w.asDiagonal()*w.asDiagonal()*dwi);
          }
          if (maxit > 1)
            w = (A*x).array().exp();
        }

        if (b0_image.valid()) {
          assign_pos_of (dwi_image, 0, 3).to (b0_image);
          b0_image.value() = exp(x[0]);
        }

        if (dt_image.valid()) {
          assign_pos_of (dwi_image, 0, 3).to (dt_image);
          for (auto l = Loop(3)(dt_image); l; ++l) {
            dt_image.value() = x[dt_image.index(3)+1];
          }
        }

        if (dkt_image.valid()) {
          assign_pos_of (dwi_image, 0, 3).to (dkt_image);
          double adc_sq = (x[1]+x[2]+x[3])*(x[1]+x[2]+x[3])/9.0;
          for (auto l = Loop(3)(dkt_image); l; ++l) {
            dkt_image.value() = x[dkt_image.index(3)+7]/adc_sq;
          }
        }

        if (predict_image.valid()) {
          assign_pos_of (dwi_image, 0, 3).to (predict_image);
          dwi = (A*x).array().exp();
          for (auto l = Loop(3)(predict_image); l; ++l) {
            predict_image.value() = dwi[predict_image.index(3)];
          }
        }

      }

  private:
    MASKType mask_image;
    B0Type b0_image;
    DTType dt_image;
    DKTType dkt_image;
    PredictType predict_image;
    Eigen::VectorXd dwi;
    Eigen::VectorXd x;
    Eigen::VectorXd w;
    Eigen::MatrixXd work;
    Eigen::LLT<Eigen::MatrixXd> llt;
    const Eigen::MatrixXd& A;
    const Eigen::MatrixXd& Aneq;
    const bool ols;
    const int maxit;
};

template <class MASKType, class B0Type, class DTType, class DKTType, class PredictType>
inline Processor<MASKType, B0Type, DTType, DKTType, PredictType> processor (const Eigen::MatrixXd& A, const Eigen::MatrixXd& Aneq, const bool ols, const int iter, const MASKType& mask_image, const B0Type& b0_image, const DTType& dt_image, const DKTType& dkt_image, const PredictType& predict_image) {
  return { A, Aneq, ols, iter, mask_image, b0_image, dt_image, dkt_image, predict_image };
}

void run ()
{
  auto dwi = Header::open (argument[0]).get_image<value_type>();
  auto grad = DWI::get_DW_scheme (dwi);

  Image<bool> mask;
  auto opt = get_options ("mask");
  if (opt.size()) {
    mask = Image<bool>::open (opt[0][0]);
    check_dimensions (dwi, mask, 0, 3);
  }

  bool ols = get_options ("ols").size();

  // depending on whether first (initialisation) loop should be considered an iteration
  auto iter = get_option_value ("iter", DEFAULT_NITER);

  Header header (dwi);
  header.datatype() = DataType::Float32;
  header.ndim() = 4;
  DWI::stash_DW_scheme (header, grad);
  PhaseEncoding::clear_scheme (header);

  Image<value_type> predict;
  opt = get_options ("predicted_signal");
  if (opt.size())
    predict = Image<value_type>::create (opt[0][0], header);

  header.size(3) = 6;
  auto dt = Image<value_type>::create (argument[1], header);

  Image<value_type> b0;
  opt = get_options ("b0");
  if (opt.size()) {
    header.ndim() = 3;
    b0 = Image<value_type>::create (opt[0][0], header);
  }

  Image<value_type> dkt;
  opt = get_options ("dkt");
  bool dki = opt.size()>0;

  if (dki) {
    header.ndim() = 4;
    header.size(3) = 15;
    dkt = Image<value_type>::create (opt[0][0], header);
  }

  Eigen::MatrixXd A = -DWI::grad2bmatrix<double> (grad, dki);

  bool constrain = get_options ("constrain").size();

  Eigen::MatrixXd Aneq;
  if (constrain) {
    auto maxb = grad.col(3).maxCoeff();
    Eigen::MatrixXd constr_dirs = Math::Sphere::spherical2cartesian(DWI::Directions::electrostatic_repulsion_300());
    Eigen::MatrixXd tmp = DWI::grad2bmatrix<double> (constr_dirs, dki);
    if (dki) {
      Aneq = Eigen::MatrixXd::Zero(tmp.rows()*2,tmp.cols());
      //Aneq.block(tmp.rows()*0,1,tmp.rows(), 6) =                tmp.block(0,1,tmp.rows(), 6); --> redundant constraint
      Aneq.block(tmp.rows()*0,7,tmp.rows(),15) =           -6.0*tmp.block(0,7,tmp.rows(),15);
      Aneq.block(tmp.rows()*1,1,tmp.rows(), 6) =                tmp.block(0,1,tmp.rows(), 6);
      Aneq.block(tmp.rows()*1,7,tmp.rows(),15) = (maxb/3.0)*6.0*tmp.block(0,7,tmp.rows(),15);
    } else {
      Aneq = Eigen::MatrixXd::Zero(tmp.rows()*1,tmp.cols());
      Aneq.block(tmp.rows()*0,1,tmp.rows(), 6) =                tmp.block(0,1,tmp.rows(), 6);
    }
  }

  ThreadedLoop("computing tensors", dwi, 0, 3).run (processor (A, Aneq, ols, iter, mask, b0, dt, dkt, predict), dwi);
}
