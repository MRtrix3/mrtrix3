/* Copyright (c) 2008-2024 the MRtrix3 contributors.
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

#include "algo/threaded_copy.h"
#include "command.h"
#include "dwi/directions/predefined.h"
#include "dwi/gradient.h"
#include "dwi/tensor.h"
#include "file/matrix.h"
#include "image.h"
#include "progressbar.h"
#include <Eigen/Eigenvalues>

using namespace MR;
using namespace App;

using value_type = float;
const char *modulate_choices[] = {"none", "fa", "eigval", NULL};
#define DEFAULT_RK_NDIRS 300

// clang-format off
void usage() {

  ARGUMENTS
  + Argument("tensor", "the input tensor image.").type_image_in();

  OPTIONS
  + Option("mask", "only perform computation within the specified binary brain mask image.")
    + Argument("image").type_image_in()

  + OptionGroup("Diffusion Tensor Imaging")

    + Option("adc",
             "compute the mean apparent diffusion coefficient (ADC) of the diffusion tensor."
             " (sometimes also referred to as the mean diffusivity (MD))")
      + Argument("image").type_image_out()

    + Option("fa", "compute the fractional anisotropy (FA) of the diffusion tensor.")
      + Argument("image").type_image_out()

    + Option("ad",
             "compute the axial diffusivity (AD) of the diffusion tensor."
               " (equivalent to the principal eigenvalue)")
      + Argument("image").type_image_out()

    + Option("rd",
             "compute the radial diffusivity (RD) of the diffusion tensor."
               " (equivalent to the mean of the two non-principal eigenvalues)")
      + Argument("image").type_image_out()

    + Option("value", "compute the selected eigenvalue(s) of the diffusion tensor.")
      + Argument("image").type_image_out()

    + Option("vector", "compute the selected eigenvector(s) of the diffusion tensor.")
      + Argument("image").type_image_out()

    + Option("num",
             "specify the desired eigenvalue/eigenvector(s)."
             " Note that several eigenvalues can be specified as a number sequence."
             " For example, '1,3' specifies the principal (1) and minor (3) eigenvalues/eigenvectors"
             " (default = 1).")
      + Argument("sequence").type_sequence_int()

    + Option("modulate",
             "specify how to modulate the magnitude of the eigenvectors."
             " Valid choices are:"
             " none, FA, eigval"
             " (default = FA).")
      + Argument("choice").type_choice(modulate_choices)

    + Option("cl",
             "compute the linearity metric of the diffusion tensor."
             " (one of the three Westin shape metrics)")
      + Argument("image").type_image_out()

    + Option("cp",
             "compute the planarity metric of the diffusion tensor."
             " (one of the three Westin shape metrics)")
      + Argument("image").type_image_out()

    + Option("cs",
             "compute the sphericity metric of the diffusion tensor."
             " (one of the three Westin shape metrics)")
      + Argument("image").type_image_out()

  + OptionGroup("Diffusion Kurtosis Imaging")

    + Option("dkt", "input diffusion kurtosis tensor.")
      + Argument("image").type_image_in()

    + Option("mk", "compute the mean kurtosis (MK) of the kurtosis tensor.")
      + Argument("image").type_image_out()

    + Option("ak", "compute the axial kurtosis (AK) of the kurtosis tensor.")
      + Argument("image").type_image_out()

    + Option("rk", "compute the radial kurtosis (RK) of the kurtosis tensor.")
      + Argument("image").type_image_out()

    + Option("mk_dirs",
             "specify the directions used to numerically calculate mean kurtosis"
             " (by default, the built-in 300 direction set is used)."
             " These should be supplied as a text file containing [ az el ] pairs for the directions.")
      + Argument("file").type_file_in()

    + Option("rk_ndirs",
             "specify the number of directions used to numerically calculate radial kurtosis"
             " (by default, " + str(DEFAULT_RK_NDIRS) + " directions are used).")
      + Argument("integer").type_integer(0, 1000);

  AUTHOR = "Ben Jeurissen (ben.jeurissen@uantwerpen.be)"
           " and Thijs Dhollander (thijs.dhollander@gmail.com)"
           " and J-Donald Tournier (jdtournier@gmail.com)";

  SYNOPSIS = "Generate maps of tensor-derived parameters";

  REFERENCES
  + "Basser, P. J.; Mattiello, J. & Lebihan, D. "
    "MR diffusion tensor spectroscopy and imaging. "
    "Biophysical Journal, 1994, 66, 259-267"
  + "* If using -cl, -cp or -cs options: \n"
    "Westin, C. F.; Peled, S.; Gudbjartsson, H.; Kikinis, R. & Jolesz, F. A. "
    "Geometrical diffusion measures for MRI from tensor basis analysis. "
    "Proc Intl Soc Mag Reson Med, 1997, 5, 1742";
}
// clang-format on

class Processor {
public:
  Processor(Image<bool> &mask_img,
            Image<value_type> &adc_img,
            Image<value_type> &fa_img,
            Image<value_type> &ad_img,
            Image<value_type> &rd_img,
            Image<value_type> &cl_img,
            Image<value_type> &cp_img,
            Image<value_type> &cs_img,
            Image<value_type> &value_img,
            Image<value_type> &vector_img,
            Image<value_type> &dkt_img,
            Image<value_type> &mk_img,
            Image<value_type> &ak_img,
            Image<value_type> &rk_img,
            std::vector<uint32_t> &vals,
            int modulate,
            Eigen::MatrixXd mk_dirs,
            int rk_ndirs)
      : mask_img(mask_img),
        adc_img(adc_img),
        fa_img(fa_img),
        ad_img(ad_img),
        rd_img(rd_img),
        cl_img(cl_img),
        cp_img(cp_img),
        cs_img(cs_img),
        value_img(value_img),
        vector_img(vector_img),
        dkt_img(dkt_img),
        mk_img(mk_img),
        ak_img(ak_img),
        rk_img(rk_img),
        vals(vals),
        modulate(modulate),
        mk_dirs(mk_dirs),
        rk_ndirs(rk_ndirs),
        need_eigenvalues(value_img.valid() || vector_img.valid() || ad_img.valid() || rd_img.valid() ||
                         cl_img.valid() || cp_img.valid() || cs_img.valid() || ak_img.valid() || rk_img.valid()),
        need_eigenvectors(vector_img.valid() || ak_img.valid() || rk_img.valid()),
        need_dkt(dkt_img.valid() || mk_img.valid() || ak_img.valid() || rk_img.valid()) {
    for (auto &n : this->vals)
      --n;
    if (mk_img.valid())
      mk_bmat = DWI::grad2bmatrix<double>(mk_dirs, true);
    if (rk_img.valid()) {
      rk_dirs.resize(rk_ndirs, 3);
      rk_bmat.resize(rk_ndirs, 22);
    }
  }

  void operator()(Image<value_type> &dt_img) {
    /* check mask */
    if (mask_img.valid()) {
      assign_pos_of(dt_img, 0, 3).to(mask_img);
      if (!mask_img.value())
        return;
    }

    /* input dt */
    Eigen::Matrix<double, 6, 1> dt;
    for (dt_img.index(3) = 0; dt_img.index(3) < 6; ++dt_img.index(3))
      dt[dt_img.index(3)] = dt_img.value();

    /* output adc */
    if (adc_img.valid()) {
      assign_pos_of(dt_img, 0, 3).to(adc_img);
      adc_img.value() = DWI::tensor2ADC(dt);
    }

    double fa = 0.0;
    if (fa_img.valid() || (vector_img.valid() && (modulate == 1)))
      fa = DWI::tensor2FA(dt);

    /* output fa */
    if (fa_img.valid()) {
      assign_pos_of(dt_img, 0, 3).to(fa_img);
      fa_img.value() = fa;
    }

    Eigen::SelfAdjointEigenSolver<Eigen::Matrix3d> es;
    if (need_eigenvalues || vector_img.valid()) {
      Eigen::Matrix3d M;
      M(0, 0) = dt[0];
      M(1, 1) = dt[1];
      M(2, 2) = dt[2];
      M(0, 1) = M(1, 0) = dt[3];
      M(0, 2) = M(2, 0) = dt[4];
      M(1, 2) = M(2, 1) = dt[5];
      es = Eigen::SelfAdjointEigenSolver<Eigen::Matrix3d>(
          M, need_eigenvectors ? Eigen::ComputeEigenvectors : Eigen::EigenvaluesOnly);
    }

    Eigen::Vector3d eigval;
    ssize_t ith_eig[3] = {2, 1, 0};
    if (need_eigenvalues) {
      eigval = es.eigenvalues();
      ith_eig[0] = 0;
      ith_eig[1] = 1;
      ith_eig[2] = 2;
      std::sort(std::begin(ith_eig), std::end(ith_eig), [&eigval](size_t a, size_t b) {
        return abs(eigval[a]) > abs(eigval[b]);
      });
    }

    /* output value */
    if (value_img.valid()) {
      assign_pos_of(dt_img, 0, 3).to(value_img);
      if (vals.size() > 1) {
        auto l = Loop(3)(value_img);
        for (size_t i = 0; i < vals.size(); i++) {
          value_img.value() = eigval(ith_eig[vals[i]]);
          l++;
        }
      } else {
        value_img.value() = eigval(ith_eig[vals[0]]);
      }
    }

    /* output ad */
    if (ad_img.valid()) {
      assign_pos_of(dt_img, 0, 3).to(ad_img);
      ad_img.value() = eigval(2);
    }

    /* output rd */
    if (rd_img.valid()) {
      assign_pos_of(dt_img, 0, 3).to(rd_img);
      rd_img.value() = (eigval(1) + eigval(0)) / 2;
    }

    /* output shape measures */
    if (cl_img.valid() || cp_img.valid() || cs_img.valid()) {
      double eigsum = eigval.sum();
      if (eigsum != 0.0) {
        if (cl_img.valid()) {
          assign_pos_of(dt_img, 0, 3).to(cl_img);
          cl_img.value() = (eigval(2) - eigval(1)) / eigsum;
        }
        if (cp_img.valid()) {
          assign_pos_of(dt_img, 0, 3).to(cp_img);
          cp_img.value() = 2.0 * (eigval(1) - eigval(0)) / eigsum;
        }
        if (cs_img.valid()) {
          assign_pos_of(dt_img, 0, 3).to(cs_img);
          cs_img.value() = 3.0 * eigval(0) / eigsum;
        }
      }
    }

    /* output vector */
    if (vector_img.valid()) {
      Eigen::Matrix3d eigvec = es.eigenvectors();
      assign_pos_of(dt_img, 0, 3).to(vector_img);
      auto l = Loop(3)(vector_img);
      for (size_t i = 0; i < vals.size(); i++) {
        double fact = 1.0;
        if (modulate == 1)
          fact = fa;
        else if (modulate == 2)
          fact = eigval(ith_eig[vals[i]]);
        vector_img.value() = eigvec(0, ith_eig[vals[i]]) * fact;
        l++;
        vector_img.value() = eigvec(1, ith_eig[vals[i]]) * fact;
        l++;
        vector_img.value() = eigvec(2, ith_eig[vals[i]]) * fact;
        l++;
      }
    }

    /* input dkt */
    Eigen::Matrix<double, 15, 1> dkt;
    if (dkt_img.valid()) {
      double adc_sq = Math::pow2(DWI::tensor2ADC(dt));
      assign_pos_of(dt_img, 0, 3).to(dkt_img);
      for (auto l = Loop(3)(dkt_img); l; ++l)
        dkt[dkt_img.index(3)] = dkt_img.value() * adc_sq;
    }

    /* output mk */
    if (mk_img.valid()) {
      assign_pos_of(dt_img, 0, 3).to(mk_img);
      mk_img.value() = kurtosis(mk_bmat, dt, dkt);
    }

    /* output ak */
    if (ak_img.valid()) {
      Eigen::Matrix<double, 1, 22> ak_bmat =
          DWI::grad2bmatrix<double>(es.eigenvectors().col(ith_eig[0]).transpose(), true);
      assign_pos_of(dt_img, 0, 3).to(ak_img);
      ak_img.value() = kurtosis(ak_bmat, dt, dkt);
    }

    /* output rk */
    if (rk_img.valid()) {
      Eigen::Vector3d dir1 = es.eigenvectors().col(ith_eig[0]);
      Eigen::Vector3d dir2 = es.eigenvectors().col(ith_eig[1]);
      const double delta = Math::pi / rk_ndirs;
      double a = 0;
      for (int i = 0; i < rk_ndirs; i++) {
        rk_dirs.row(i) = Eigen::AngleAxisd(a, dir1) * dir2;
        a += delta;
      }
      rk_bmat.noalias() = DWI::grad2bmatrix<double>(rk_dirs, true);
      assign_pos_of(dt_img, 0, 3).to(rk_img);
      rk_img.value() = kurtosis(rk_bmat, dt, dkt);
    }
  }

private:
  Image<bool> mask_img;
  Image<value_type> adc_img;
  Image<value_type> fa_img;
  Image<value_type> ad_img;
  Image<value_type> rd_img;
  Image<value_type> cl_img;
  Image<value_type> cp_img;
  Image<value_type> cs_img;
  Image<value_type> value_img;
  Image<value_type> vector_img;
  Image<value_type> dkt_img;
  Image<value_type> mk_img;
  Image<value_type> ak_img;
  Image<value_type> rk_img;
  std::vector<uint32_t> vals;
  const int modulate;
  Eigen::MatrixXd mk_dirs;
  Eigen::MatrixXd mk_bmat, rk_bmat;
  Eigen::MatrixXd rk_dirs;
  const int rk_ndirs;
  const bool need_eigenvalues;
  const bool need_eigenvectors;
  const bool need_dkt;

  template <class BMatType, class DTType, class DKTType>
  double kurtosis(const BMatType &bmat, const DTType &dt, const DKTType &dkt) {
    return -6.0 *
           ((bmat.template middleCols<15>(7) * dkt).array() / (bmat.template middleCols<6>(1) * dt).array().square())
               .mean();
  }
};

void run() {
  auto dt_img = Image<value_type>::open(argument[0]);
  Header header(dt_img);
  if (header.ndim() != 4 || header.size(3) != 6) {
    throw Exception("input tensor image is not a valid tensor.");
  }

  auto mask_img = Image<bool>();
  auto opt = get_options("mask");
  if (!opt.empty()) {
    mask_img = Image<bool>::open(opt[0][0]);
    check_dimensions(dt_img, mask_img, 0, 3);
  }

  size_t metric_count = 0;
  size_t dki_metric_count = 0;

  auto adc_img = Image<value_type>();
  opt = get_options("adc");
  if (!opt.empty()) {
    header.ndim() = 3;
    adc_img = Image<value_type>::create(opt[0][0], header);
    metric_count++;
  }

  auto fa_img = Image<value_type>();
  opt = get_options("fa");
  if (!opt.empty()) {
    header.ndim() = 3;
    fa_img = Image<value_type>::create(opt[0][0], header);
    metric_count++;
  }

  auto ad_img = Image<value_type>();
  opt = get_options("ad");
  if (!opt.empty()) {
    header.ndim() = 3;
    ad_img = Image<value_type>::create(opt[0][0], header);
    metric_count++;
  }

  auto rd_img = Image<value_type>();
  opt = get_options("rd");
  if (!opt.empty()) {
    header.ndim() = 3;
    rd_img = Image<value_type>::create(opt[0][0], header);
    metric_count++;
  }

  auto cl_img = Image<value_type>();
  opt = get_options("cl");
  if (!opt.empty()) {
    header.ndim() = 3;
    cl_img = Image<value_type>::create(opt[0][0], header);
    metric_count++;
  }

  auto cp_img = Image<value_type>();
  opt = get_options("cp");
  if (!opt.empty()) {
    header.ndim() = 3;
    cp_img = Image<value_type>::create(opt[0][0], header);
    metric_count++;
  }

  auto cs_img = Image<value_type>();
  opt = get_options("cs");
  if (!opt.empty()) {
    header.ndim() = 3;
    cs_img = Image<value_type>::create(opt[0][0], header);
    metric_count++;
  }

  std::vector<uint32_t> vals = {1};
  opt = get_options("num");
  if (!opt.empty()) {
    vals = parse_ints<uint32_t>(opt[0][0]);
    if (vals.empty())
      throw Exception("invalid eigenvalue/eigenvector number specifier");
    for (size_t i = 0; i < vals.size(); ++i)
      if (vals[i] < 1 || vals[i] > 3)
        throw Exception("eigenvalue/eigenvector number is out of bounds");
  }

  float modulate = get_option_value("modulate", 1);

  auto value_img = Image<value_type>();
  opt = get_options("value");
  if (!opt.empty()) {
    header.ndim() = 3;
    if (vals.size() > 1) {
      header.ndim() = 4;
      header.size(3) = vals.size();
    }
    value_img = Image<value_type>::create(opt[0][0], header);
    metric_count++;
  }

  auto vector_img = Image<value_type>();
  opt = get_options("vector");
  if (!opt.empty()) {
    header.ndim() = 4;
    header.size(3) = vals.size() * 3;
    vector_img = Image<value_type>::create(opt[0][0], header);
    metric_count++;
  }

  auto dkt_img = Image<value_type>();
  opt = get_options("dkt");
  if (!opt.empty()) {
    dkt_img = Image<value_type>::open(opt[0][0]);
    check_dimensions(dt_img, dkt_img, 0, 3);
  }

  auto mk_img = Image<value_type>();
  opt = get_options("mk");
  if (!opt.empty()) {
    header.ndim() = 3;
    mk_img = Image<value_type>::create(opt[0][0], header);
    metric_count++;
    dki_metric_count++;
  }

  auto ak_img = Image<value_type>();
  opt = get_options("ak");
  if (!opt.empty()) {
    header.ndim() = 3;
    ak_img = Image<value_type>::create(opt[0][0], header);
    metric_count++;
    dki_metric_count++;
  }

  auto rk_img = Image<value_type>();
  opt = get_options("rk");
  if (!opt.empty()) {
    header.ndim() = 3;
    rk_img = Image<value_type>::create(opt[0][0], header);
    metric_count++;
    dki_metric_count++;
  }

  opt = get_options("mk_dirs");
  const Eigen::MatrixXd mk_dirs =
      !opt.empty() ? File::Matrix::load_matrix(opt[0][0])
                   : Math::Sphere::spherical2cartesian(DWI::Directions::electrostatic_repulsion_300());

  auto rk_ndirs = get_option_value("rk_ndirs", DEFAULT_RK_NDIRS);

  if (dki_metric_count && !dkt_img.valid()) {
    throw Exception(
        "Cannot calculate diffusion kurtosis metrics; must provide the kurtosis tensor using the -dkt input option");
  }

  if (!metric_count)
    throw Exception(
        "No output specified; must request at least one metric of interest using the available command-line options");

  ThreadedLoop(std::string("computing metric") + (metric_count > 1 ? "s" : ""), dt_img, 0, 3)
      .run(Processor(mask_img,
                     adc_img,
                     fa_img,
                     ad_img,
                     rd_img,
                     cl_img,
                     cp_img,
                     cs_img,
                     value_img,
                     vector_img,
                     dkt_img,
                     mk_img,
                     ak_img,
                     rk_img,
                     vals,
                     modulate,
                     mk_dirs,
                     rk_ndirs),
           dt_img);
}
