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

#pragma once

#include "adapter/jacobian.h"
#include "algo/threaded_loop.h"
#include "math/SH.h"
#include "math/least_squares.h"
#include "registration/multi_contrast.h"

namespace MR::Registration::Transform {

FORCE_INLINE Eigen::MatrixXd aPSF_weights_to_FOD_transform(const int num_SH, const Eigen::MatrixXd &directions) {
  const size_t lmax = Math::SH::LforN(num_SH);
  Eigen::MatrixXd delta_matrix = Math::SH::init_transform_cart(directions.transpose(), lmax);
  Math::SH::aPSF<default_type> aPSF(lmax);
  Math::SH::sconv_mat(delta_matrix, aPSF.RH_coefs());
  return delta_matrix.transpose();
}

FORCE_INLINE std::vector<std::vector<ssize_t>>
multiContrastSetting2start_nvols(const std::vector<MultiContrastSetting> &mcsettings, size_t &max_n_SH) {
  max_n_SH = 0;
  std::vector<std::vector<ssize_t>> start_nvols;
  if (mcsettings.size() != 0) {
    for (const auto &mc : mcsettings) {
      if (mc.do_reorientation && mc.lmax > 0) {
        start_nvols.emplace_back(std::initializer_list<ssize_t>{(ssize_t)mc.start, (ssize_t)mc.nvols});
        max_n_SH = std::max(mc.nvols, max_n_SH);
      }
    }
  }
  assert(max_n_SH > 1);
  assert(start_nvols.size());
  return start_nvols;
}

template <class FODImageType> class LinearKernelMultiContrast {

public:
  LinearKernelMultiContrast(ssize_t n_vol,
                            ssize_t max_n_SH,
                            const transform_type &linear_transform,
                            const Eigen::MatrixXd &directions,
                            const std::vector<std::vector<ssize_t>> &vstart_nvols,
                            const bool modulate)
      : fod(n_vol), max_n_SH(max_n_SH), start_nvols(vstart_nvols) {
    assert(n_vol > max_n_SH);
    assert(start_nvols.size());
    Eigen::MatrixXd transformed_directions = linear_transform.linear().inverse() * directions;
    // precompute projection for maximum requested lmax
    if (modulate) {
      Eigen::VectorXd modulation_factors =
          transformed_directions.colwise().norm() / linear_transform.linear().inverse().determinant();
      transformed_directions.colwise().normalize();
      transform.noalias() = aPSF_weights_to_FOD_transform(max_n_SH, transformed_directions) *
                            modulation_factors.asDiagonal() *
                            Math::pinv(aPSF_weights_to_FOD_transform(max_n_SH, directions));
    } else {
      transformed_directions.colwise().normalize();
      transform.noalias() = aPSF_weights_to_FOD_transform(max_n_SH, transformed_directions) *
                            Math::pinv(aPSF_weights_to_FOD_transform(max_n_SH, directions));
    }
  }

  void operator()(FODImageType &in, FODImageType &out) {
    in.index(3) = 0; // TODO do we need this?
    // TODO is it faster to check whether any voxel contains an FOD before copying the row?
    fod = in.row(3);
    for (auto const &sn : start_nvols) {
      if (fod[sn[0]] > 0.0) { // only reorient voxels that contain an FOD
        fod.segment(sn[0], sn[1]) = transform.block(0, 0, sn[1], sn[1]) * fod.segment(sn[0], sn[1]);
      }
    }
    in.index(3) = 0; // TODO do we need this?
    out.row(3) = fod;
  }

protected:
  Eigen::VectorXd fod;
  ssize_t max_n_SH;
  std::vector<std::vector<ssize_t>> start_nvols;
  Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> transform;
};

template <class FODImageType> class LinearKernel {

public:
  LinearKernel(const ssize_t n_SH,
               const transform_type &linear_transform,
               const Eigen::MatrixXd &directions,
               const bool modulate)
      : fod(n_SH) {
    Eigen::MatrixXd transformed_directions = linear_transform.linear().inverse() * directions;

    if (modulate) {
      Eigen::VectorXd modulation_factors =
          transformed_directions.colwise().norm() / linear_transform.linear().inverse().determinant();
      transformed_directions.colwise().normalize();
      transform.noalias() = aPSF_weights_to_FOD_transform(n_SH, transformed_directions) *
                            modulation_factors.asDiagonal() *
                            Math::pinv(aPSF_weights_to_FOD_transform(n_SH, directions));
    } else {
      transformed_directions.colwise().normalize();
      transform.noalias() = aPSF_weights_to_FOD_transform(n_SH, transformed_directions) *
                            Math::pinv(aPSF_weights_to_FOD_transform(n_SH, directions));
    }
  }

  void operator()(FODImageType &in, FODImageType &out) {
    in.index(3) = 0;
    if (in.value() > 0.0) { // only reorient voxels that contain an FOD
      fod = in.row(3);
      fod = transform * fod;
      out.row(3) = fod;
    }
  }

protected:
  Eigen::VectorXd fod;
  Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> transform;
};

/*
 * reorient all FODs in an image with a linear transform.
 * Note the input image can be the same as the output image
 */
template <class FODImageType>
void reorient(FODImageType &input_fod_image,
              FODImageType &output_fod_image,
              const transform_type &transform,
              const Eigen::MatrixXd &directions,
              bool modulate = false,
              std::vector<MultiContrastSetting> multi_contrast_settings = std::vector<MultiContrastSetting>()) {
  assert(directions.cols() > directions.rows());
  std::vector<std::vector<ssize_t>> start_nvols;
  size_t max_n_SH(0);
  if (multi_contrast_settings.size())
    start_nvols = multiContrastSetting2start_nvols(multi_contrast_settings, max_n_SH);

  if (start_nvols.size()) {
    assert(max_n_SH > 1);
    ThreadedLoop(input_fod_image, 0, 3)
        .run(LinearKernelMultiContrast<FODImageType>(
                 input_fod_image.size(3), max_n_SH, transform, directions, start_nvols, modulate),
             input_fod_image,
             output_fod_image);
  } else {
    ThreadedLoop(input_fod_image, 0, 3)
        .run(LinearKernel<FODImageType>(input_fod_image.size(3), transform, directions, modulate),
             input_fod_image,
             output_fod_image);
  }
}

/*
 * reorient all FODs in an image with a linear transform.
 * Note the input image can be the same as the output image
 */
template <class FODImageType>
void reorient(const std::string progress_message,
              FODImageType &input_fod_image,
              FODImageType &output_fod_image,
              const transform_type &transform,
              const Eigen::MatrixXd &directions,
              bool modulate = false,
              std::vector<MultiContrastSetting> multi_contrast_settings = std::vector<MultiContrastSetting>()) {
  assert(directions.cols() > directions.rows());
  std::vector<std::vector<ssize_t>> start_nvols;
  size_t max_n_SH(0);
  if (multi_contrast_settings.size())
    start_nvols = multiContrastSetting2start_nvols(multi_contrast_settings, max_n_SH);

  if (start_nvols.size()) {
    assert(max_n_SH > 1);
    ThreadedLoop(progress_message, input_fod_image, 0, 3)
        .run(LinearKernelMultiContrast<FODImageType>(
                 input_fod_image.size(3), max_n_SH, transform, directions, start_nvols, modulate),
             input_fod_image,
             output_fod_image);
  } else {
    ThreadedLoop(progress_message, input_fod_image, 0, 3)
        .run(LinearKernel<FODImageType>(input_fod_image.size(3), transform, directions, modulate),
             input_fod_image,
             output_fod_image);
  }
}

template <class FODImageType> class NonLinearKernelMultiContrast {

public:
  NonLinearKernelMultiContrast(ssize_t n_vol,
                               ssize_t max_n_SH,
                               Image<default_type> &warp,
                               const Eigen::MatrixXd &directions,
                               const std::vector<std::vector<ssize_t>> &vstart_nvols,
                               const bool modulate)
      : max_n_SH(max_n_SH),
        n_dirs(directions.cols()),
        jacobian_adapter(warp),
        directions(directions),
        modulate(modulate),
        start_nvols(vstart_nvols),
        fod(n_vol) {
    for (auto const &sn : start_nvols)
      map_FOD_to_aPSF_transform[sn[1]] = Math::pinv(aPSF_weights_to_FOD_transform(sn[1], directions));
    assert(n_vol > 0);
    assert(start_nvols.size());
  }

  void operator()(FODImageType &image) {
    // get highest n_SH for compartments that contain a non-zero FOD in this voxel
    ssize_t max_n_SHvox = 0;
    for (auto const &sn : start_nvols) {
      image.index(3) = sn[0];
      if (image.value() > 0.0) {
        max_n_SHvox = std::max(max_n_SHvox, sn[1]);
      }
    }
    if (max_n_SHvox == 0)
      return;

    image.index(3) = 0;
    fod = image.row(3);

    for (size_t dim = 0; dim < 3; ++dim)
      jacobian_adapter.index(dim) = image.index(dim);
    Eigen::MatrixXd jacobian = jacobian_adapter.value().inverse().template cast<default_type>();
    Eigen::MatrixXd transformed_directions = jacobian * directions;

    if (modulate)
      modulation_factors = transformed_directions.colwise().norm() / jacobian.determinant();

    transformed_directions.colwise().normalize();

    if (modulate) {
      Eigen::MatrixXd temp = aPSF_weights_to_FOD_transform(max_n_SHvox, transformed_directions);
      for (ssize_t i = 0; i < n_dirs; ++i)
        temp.col(i) = temp.col(i) * modulation_factors(0, i);

      transform.noalias() = temp * map_FOD_to_aPSF_transform[max_n_SHvox];
      // TODO: make this work?
      // transform.noalias() = (aPSF_weights_to_FOD_transform (max_n_SHvox, transformed_directions).colwise() *
      // modulation_factors.transpose()) * map_FOD_to_aPSF_transform[max_n_SHvox];
    } else {
      transform.noalias() =
          aPSF_weights_to_FOD_transform(max_n_SHvox, transformed_directions) * map_FOD_to_aPSF_transform[max_n_SHvox];
    }

    // reorient voxels that contain an FOD
    for (auto const &sn : start_nvols)
      if (fod[sn[0]] > 0.0)
        fod.segment(sn[0], sn[1]) = transform.block(0, 0, sn[1], sn[1]) * fod.segment(sn[0], sn[1]);

    image.index(3) = 0; // TODO do we need this?
    image.row(3) = fod;
  }

protected:
  const ssize_t max_n_SH, n_dirs;
  Adapter::Jacobian<Image<default_type>> jacobian_adapter;
  const Eigen::MatrixXd &directions;
  const bool modulate;
  const std::vector<std::vector<ssize_t>> start_nvols;
  Eigen::VectorXd fod;
  std::map<ssize_t, Eigen::MatrixXd> map_FOD_to_aPSF_transform;
  ssize_t max_n_SHvox;
  Eigen::MatrixXd transform;
  Eigen::MatrixXd modulation_factors;
};

template <class FODImageType> class NonLinearKernel {

public:
  NonLinearKernel(const ssize_t n_SH, Image<default_type> &warp, const Eigen::MatrixXd &directions, const bool modulate)
      : n_SH(n_SH),
        jacobian_adapter(warp),
        directions(directions),
        modulate(modulate),
        FOD_to_aPSF_transform(Math::pinv(aPSF_weights_to_FOD_transform(n_SH, directions))),
        fod(n_SH) {}

  void operator()(FODImageType &image) {
    image.index(3) = 0;
    if (image.value() > 0) { // only reorient voxels that contain a FOD
      for (size_t dim = 0; dim < 3; ++dim)
        jacobian_adapter.index(dim) = image.index(dim);
      Eigen::MatrixXd jacobian = jacobian_adapter.value().inverse().template cast<default_type>();
      Eigen::MatrixXd transformed_directions = jacobian * directions;

      if (modulate) {
        Eigen::MatrixXd modulation_factors = transformed_directions.colwise().norm() / jacobian.determinant();
        transformed_directions.colwise().normalize();

        Eigen::MatrixXd temp = aPSF_weights_to_FOD_transform(n_SH, transformed_directions);
        for (ssize_t i = 0; i < temp.cols(); ++i)
          temp.col(i) = temp.col(i) * modulation_factors(0, i);

        transform.noalias() = temp * FOD_to_aPSF_transform;
      } else {
        transformed_directions.colwise().normalize();
        transform.noalias() = aPSF_weights_to_FOD_transform(n_SH, transformed_directions) * FOD_to_aPSF_transform;
      }
      fod = image.row(3);
      fod = transform * fod;
      image.row(3) = fod;
    }
  }

protected:
  const ssize_t n_SH;
  Adapter::Jacobian<Image<default_type>> jacobian_adapter;
  const Eigen::MatrixXd &directions;
  const bool modulate;
  const Eigen::MatrixXd FOD_to_aPSF_transform;
  Eigen::MatrixXd transform;
  Eigen::VectorXd fod;
};

template <class FODImageType>
void reorient_warp(const std::string progress_message,
                   FODImageType &fod_image,
                   Image<default_type> &warp,
                   const Eigen::MatrixXd &directions,
                   const bool modulate = false,
                   std::vector<MultiContrastSetting> multi_contrast_settings = std::vector<MultiContrastSetting>()) {
  assert(directions.cols() > directions.rows());
  check_dimensions(fod_image, warp, 0, 3);
  std::vector<std::vector<ssize_t>> start_nvols;
  size_t max_n_SH(0);
  if (multi_contrast_settings.size())
    start_nvols = multiContrastSetting2start_nvols(multi_contrast_settings, max_n_SH);
  if (start_nvols.size()) {
    DEBUG("reorienting warp using MultiContrast NonLinearKernel");
    ThreadedLoop(progress_message, fod_image, 0, 3)
        .run(NonLinearKernelMultiContrast<FODImageType>(
                 fod_image.size(3), (ssize_t)max_n_SH, warp, directions, start_nvols, modulate),
             fod_image);
  } else {
    DEBUG("reorienting warp using NonLinearKernel");
    ThreadedLoop(progress_message, fod_image, 0, 3)
        .run(NonLinearKernel<FODImageType>(fod_image.size(3), warp, directions, modulate), fod_image);
  }
}

template <class FODImageType>
void reorient_warp(FODImageType &fod_image,
                   Image<default_type> &warp,
                   const Eigen::MatrixXd &directions,
                   const bool modulate = false,
                   std::vector<MultiContrastSetting> multi_contrast_settings = std::vector<MultiContrastSetting>()) {
  assert(directions.cols() > directions.rows());
  check_dimensions(fod_image, warp, 0, 3);
  std::vector<std::vector<ssize_t>> start_nvols;
  size_t max_n_SH(0);
  if (multi_contrast_settings.size())
    start_nvols = multiContrastSetting2start_nvols(multi_contrast_settings, max_n_SH);

  if (start_nvols.size()) {
    DEBUG("reorienting warp using MultiContrast NonLinearKernel");
    ThreadedLoop(fod_image, 0, 3)
        .run(NonLinearKernelMultiContrast<FODImageType>(
                 fod_image.size(3), (ssize_t)max_n_SH, warp, directions, start_nvols, modulate),
             fod_image);
  } else {
    DEBUG("reorienting warp using NonLinearKernel");
    ThreadedLoop(fod_image, 0, 3)
        .run(NonLinearKernel<FODImageType>(fod_image.size(3), warp, directions, modulate), fod_image);
  }
}

} // namespace MR::Registration::Transform
