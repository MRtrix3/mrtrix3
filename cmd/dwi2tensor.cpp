/* Copyright (c) 2008-2019 the MRtrix3 contributors.
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

using namespace MR;
using namespace App;

using value_type = float;

#define DEFAULT_NITER 2

const char* encoding_description =
  "The tensor coefficients are stored in the output image as follows: \n"
  "volumes 0-5: D11, D22, D33, D12, D13, D23 ; \n\n"
  "If diffusion kurtosis is estimated using the -dkt option, these are stored as follows: \n"
  "volumes 0-2: W1111, W2222, W3333 ; \n"
  "volumes 3-8: W1112, W1113, W1222, W1333, W2223, W2333 ; \n"
  "volumes 9-11: W1122, W1133, W2233 ; \n"
  "volumes 12-14: W1123, W1223, W1233 ;";


void usage ()
{
  AUTHOR = "Ben Jeurissen (ben.jeurissen@uantwerpen.be)";

  SYNOPSIS = "Diffusion (kurtosis) tensor estimation.";

  DESCRIPTION
    + "Estimate diffusion tensor (and optionally its kurtosis) by fitting to "
    "the log-signal using either ordinary least-squares (OLS) or weighted least-squares (WLS), "
    "followed by further re-weighted iterations using the previous signals predictions (IWLS)."
    "By default, the initial fit is performed using WLS, followed by 2 IWLS iterations."

    + encoding_description;

  ARGUMENTS
    + Argument ("dwi", "the input dwi image.").type_image_in ()
    + Argument ("dt", "the output dt image.").type_image_out ();

  OPTIONS
    + Option ("ols", "perform initial fit using an ordinary least-squares (OLS) fit, "
        "instead of the default weighted least-squares (WLS) strategy.")

    + Option ("mask", "only perform computation within the specified binary brain mask image.")
    +   Argument ("image").type_image_in()

    + Option ("b0", "the output b0 image.")
    +   Argument ("image").type_image_out()

    + Option ("dkt", "the output dkt image.")
    +   Argument ("image").type_image_out()

    + Option ("iter","number of iterative reweightings for IWLS algorithm (default: "
        + str(DEFAULT_NITER) + ". Set to zero to perform standard OLS or WLS.")
    +   Argument ("integer").type_integer (0, 10)

    + Option ("predicted_signal", "the predicted dwi image.")
    +   Argument ("image").type_image_out()

    + DWI::GradImportOptions();

  REFERENCES
   + "References based on fitting algorithm used:"

   + "* OLS, WLS:\n"
     "Basser, P.J.; Mattiello, J.; LeBihan, D."
     "Estimation of the effective self-diffusion tensor from the NMR spin echo."
     "J Magn Reson B., 1994, 103, 247–254."


  +  "* IWLS:\n"
     "Veraart, J.; Sijbers, J.; Sunaert, S.; Leemans, A. & Jeurissen, B. " // Internal
     "Weighted linear least squares estimation of diffusion MRI parameters: strengths, limitations, and pitfalls. "
     "NeuroImage, 2013, 81, 335-346";
}



template <class MASKType, class B0Type, class DKTType, class PredictType>
class Processor { MEMALIGN(Processor)
  public:
    Processor (const Eigen::MatrixXd& b, const bool ols, const int iter,
        MASKType* mask_image, B0Type* b0_image, DKTType* dkt_image, PredictType* predict_image) :
      mask_image (mask_image),
      b0_image (b0_image),
      dkt_image (dkt_image),
      predict_image (predict_image),
      dwi(b.rows()),
      p(b.cols()),
      w(Eigen::VectorXd::Ones (b.rows())),
      work(b.cols(),b.cols()),
      llt(work.rows()),
      b(b),
      ols (ols),
      maxit(iter) { }

    template <class DWIType, class DTType>
      void operator() (DWIType& dwi_image, DTType& dt_image)
      {
        if (mask_image) {
          assign_pos_of (dwi_image, 0, 3).to (*mask_image);
          if (!mask_image->value())
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
          work.setZero();
          work.selfadjointView<Eigen::Lower>().rankUpdate (b.transpose()*w.asDiagonal());
          p = llt.compute (work.selfadjointView<Eigen::Lower>()).solve(b.transpose()*w.asDiagonal()*w.asDiagonal()*dwi);
          if (maxit > 1)
            w = (b*p).array().exp();
        }

        for (auto l = Loop(3)(dt_image); l; ++l) {
          dt_image.value() = p[dt_image.index(3)];
        }

        if (b0_image) {
          assign_pos_of (dwi_image, 0, 3).to (*b0_image);
          b0_image->value() = exp(p[6]);
        }

        if (dkt_image) {
          assign_pos_of (dwi_image, 0, 3).to (*dkt_image);
          double adc_sq = (p[0]+p[1]+p[2])*(p[0]+p[1]+p[2])/9.0;
          for (auto l = Loop(3)(*dkt_image); l; ++l) {
            dkt_image->value() = p[dkt_image->index(3)+7]/adc_sq;
          }
        }

        if (predict_image) {
          assign_pos_of (dwi_image, 0, 3).to (*predict_image);
          dwi = (b*p).array().exp();
          for (auto l = Loop(3)(*predict_image); l; ++l) {
            predict_image->value() = dwi[predict_image->index(3)];
          }
        }

      }

  private:
    copy_ptr<MASKType> mask_image;
    copy_ptr<B0Type> b0_image;
    copy_ptr<DKTType> dkt_image;
    copy_ptr<PredictType> predict_image;
    Eigen::VectorXd dwi;
    Eigen::VectorXd p;
    Eigen::VectorXd w;
    Eigen::MatrixXd work;
    Eigen::LLT<Eigen::MatrixXd> llt;
    const Eigen::MatrixXd& b;
    const bool ols;
    const int maxit;
};

template <class MASKType, class B0Type, class DKTType, class PredictType>
inline Processor<MASKType, B0Type, DKTType, PredictType> processor (const Eigen::MatrixXd& b, const bool ols, const int iter, MASKType* mask_image, B0Type* b0_image, DKTType* dkt_image, PredictType* predict_image) {
  return { b, ols, iter, mask_image, b0_image, dkt_image, predict_image };
}

void run ()
{
  auto dwi = Header::open (argument[0]).get_image<value_type>();
  auto grad = DWI::get_valid_DW_scheme (dwi);

  Image<bool>* mask = nullptr;
  auto opt = get_options ("mask");
  if (opt.size()) {
    mask = new Image<bool> (Image<bool>::open (opt[0][0]));
    check_dimensions (dwi, *mask, 0, 3);
  }

  bool ols = get_options ("ols").size();

  // depending on whether first (initialisation) loop should be considered an iteration
  auto iter = get_option_value ("iter", DEFAULT_NITER);

  Header header (dwi);
  header.datatype() = DataType::Float32;
  header.ndim() = 4;
  DWI::stash_DW_scheme (header, grad);
  PhaseEncoding::clear_scheme (header);

  Image<value_type>* predict = nullptr;
  opt = get_options ("predicted_signal");
  if (opt.size())
    predict = new Image<value_type> (Image<value_type>::create (opt[0][0], header));

  header.size(3) = 6;
  auto dt = Image<value_type>::create (argument[1], header);

  Image<value_type>* b0 = nullptr;
  opt = get_options ("b0");
  if (opt.size()) {
    header.ndim() = 3;
    b0 = new Image<value_type> (Image<value_type>::create (opt[0][0], header));
  }

  Image<value_type>* dkt = nullptr;
  opt = get_options ("dkt");
  if (opt.size()) {
    header.ndim() = 4;
    header.size(3) = 15;
    dkt = new Image<value_type> (Image<value_type>::create (opt[0][0], header));
  }

  Eigen::MatrixXd b = -DWI::grad2bmatrix<double> (grad, opt.size()>0);

  ThreadedLoop("computing tensors", dwi, 0, 3).run (processor (b, ols, iter, mask, b0, dkt, predict), dwi, dt);
}

