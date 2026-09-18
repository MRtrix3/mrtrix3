/* Copyright (c) 2008-2026 the MRtrix3 contributors.
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

#include "algo/iterator.h"
#include "algo/random_loop.h"
#include "algo/random_threaded_loop.h"
#include "algo/threaded_loop.h"
#include "image.h"
#include "registration/reduction.h"
#include "transform.h"

namespace MR::Registration::Metric {

//! \cond skip
namespace {
template <class T> struct Void {
  using type = void;
};

template <class MetricType, typename U = void> struct is_neighbourhood_metric {
  using no = int;
};

template <class MetricType>
struct is_neighbourhood_metric<MetricType, typename Void<typename MetricType::is_neighbourhood>::type> {
  using yes = int;
};

template <class MetricType, typename U = void> struct use_processed_image {
  using no = int;
};

template <class MetricType>
struct use_processed_image<MetricType, typename Void<typename MetricType::requires_precompute>::type> {
  using yes = int;
};

template <class MetricType, typename U = void> struct cost_is_vector {
  using no = int;
};

template <class MetricType>
struct cost_is_vector<MetricType, typename Void<typename MetricType::is_vector_type>::type> {
  using yes = int;
};

template <class MetricType, typename U = void> struct is_asymmetric {
  using no = int;
};

template <class MetricType>
struct is_asymmetric<MetricType, typename Void<typename MetricType::is_asymmetric_type>::type> {
  using yes = int;
};
} // namespace
//! \endcond

//! Accumulate the cost function and its gradient over a set of voxels
/*! The accumulators are private to the instance; a caller that evaluates
 *  multiple instances in parallel is responsible for merging them, in an order
 *  of its choosing, once those instances have completed. */
template <class MetricType, class ParamType> class ThreadKernel {
public:
  ThreadKernel(const MetricType &metric,
               const ParamType &parameters,
               const ssize_t cost_size,
               const ssize_t gradient_size)
      : metric(metric),
        params(parameters),
        cost_function(cost_size),
        cnt(0),
        gradient(gradient_size),
        voxel2scanner(MR::Transform(params.midway_image).voxel2scanner) {
    if (params.robust_estimate_subset) {
      // iterates over subset --> adjust voxel to scanner
      assert(params.robust_estimate_subset_from.size() == 3);
      transform_type i2s(params.midway_image.transform());
      auto spacing = Eigen::DiagonalMatrix<default_type, 3>(
          params.midway_image.spacing(0), params.midway_image.spacing(1), params.midway_image.spacing(2));
      for (size_t j = 0; j < 3; ++j)
        for (size_t i = 0; i < 3; ++i)
          i2s(i, 3) += params.robust_estimate_subset_from[j] * params.midway_image.spacing(j) * i2s(i, j);
      voxel2scanner = i2s * spacing;
    }
    gradient.setZero();
    cost_function.setZero();
  }

  //! discard the accumulated sums, ready for the next set of voxels
  void reset() {
    cost_function.setZero();
    gradient.setZero();
    cnt = 0;
  }

  const Eigen::VectorXd &partial_cost() const { return cost_function; }
  const Eigen::VectorXd &partial_gradient() const { return gradient; }
  ssize_t partial_count() const { return cnt; }

  template <class U = MetricType>
  void operator()(const Iterator &iter,
                  typename is_neighbourhood_metric<U>::no = 0,
                  typename use_processed_image<U>::no = 0,
                  typename cost_is_vector<U>::no = 0,
                  typename is_asymmetric<U>::no = 0) {

    const Eigen::Vector3d voxel_pos{static_cast<default_type>(iter.index(0)),
                                    static_cast<default_type>(iter.index(1)),
                                    static_cast<default_type>(iter.index(2))};
    Eigen::Vector3d midway_point = voxel2scanner * voxel_pos;

    Eigen::Vector3d im2_point;
    params.transformation.transform_half_inverse(im2_point, midway_point);
    if (params.im2_mask_interp) {
      params.im2_mask_interp->scanner(im2_point);
      if (params.im2_mask_interp->value() < 0.5)
        return;
    }
    if (params.robust_estimate_use_score && params.robust_estimate_score2_interp) {
      params.robust_estimate_score2_interp->scanner(im2_point);
      if (!(params.robust_estimate_score2_interp->value() >= 0.5))
        return;
    }

    Eigen::Vector3d im1_point;
    params.transformation.transform_half(im1_point, midway_point);
    if (params.im1_mask_interp) {
      params.im1_mask_interp->scanner(im1_point);
      if (params.im1_mask_interp->value() < 0.5)
        return;
    }
    if (params.robust_estimate_use_score && params.robust_estimate_score1_interp) {
      params.robust_estimate_score1_interp->scanner(im1_point);
      if (!(params.robust_estimate_score1_interp->value() >= 0.5))
        return;
    }

    params.im1_image_interp->scanner(im1_point);
    if (!(*params.im1_image_interp))
      return;

    params.im2_image_interp->scanner(im2_point);
    if (!(*params.im2_image_interp))
      return;

    ++cnt;
    cost_function(0) += metric(params, im1_point, im2_point, midway_point, gradient);
  }

  template <class U = MetricType>
  void operator()(const Iterator &iter,
                  typename is_neighbourhood_metric<U>::no = 0,
                  typename use_processed_image<U>::no = 0,
                  typename cost_is_vector<U>::no = 0,
                  typename is_asymmetric<U>::yes = 0) {

    const Eigen::Vector3d voxel_pos{static_cast<default_type>(iter.index(0)),
                                    static_cast<default_type>(iter.index(1)),
                                    static_cast<default_type>(iter.index(2))};
    Eigen::Vector3d im2_point = voxel2scanner * voxel_pos; // image 2 == midway_point == fixed image

    // shift voxel position as evaluate iterates over a subset of the image
    if (params.robust_estimate_subset) {
      assert(params.robust_estimate_subset_from.size() == 3);
      voxel_pos[0] += params.robust_estimate_subset_from[0];
      voxel_pos[1] += params.robust_estimate_subset_from[1];
      voxel_pos[2] += params.robust_estimate_subset_from[2];
    }

    if (!params.robust_estimate_subset && params.robust_estimate_score2_interp) {
      params.robust_estimate_score2_interp->scanner(im2_point);
      if (!(params.robust_estimate_score2_interp->value() >= 0.5))
        return;
    }

    params.im2_image.index(0) = voxel_pos[0];
    params.im2_image.index(1) = voxel_pos[1];
    params.im2_image.index(2) = voxel_pos[2];

    // assumes that im2_mask shares the coordinate system and voxel grid with image 2
    // if (params.im2_mask.valid()) {
    //   params.im2_mask.index(0) = voxel_pos[0];
    //   params.im2_mask.index(1) = voxel_pos[1];
    //   params.im2_mask.index(2) = voxel_pos[2];
    //   if (params.im2_mask.value() < 0.5)
    //     return;
    // }

    if (params.im2_mask_interp) {
      params.im2_mask_interp->scanner(im2_point);
      if (params.im2_mask_interp->value() < 0.5)
        return;
    }

    Eigen::Vector3d im1_point; // moving
    // transform_half is full transformation, transform_half_inverse is identity
    params.transformation.transform_half(im1_point, im2_point);
    if (params.im1_mask_interp) {
      params.im1_mask_interp->scanner(im1_point);
      if (params.im1_mask_interp->value() < 0.5)
        return;
    }
    if (params.robust_estimate_use_score && params.robust_estimate_score1_interp) {
      params.robust_estimate_score1_interp->scanner(im1_point);
      if (!(params.robust_estimate_score1_interp->value() >= 0.5))
        return;
    }

    params.im1_image_interp->scanner(im1_point);
    if (!(*params.im1_image_interp))
      return;

    ++cnt;
    cost_function(0) += metric(params, im1_point, im2_point, im2_point, gradient);
  }

  template <class U = MetricType>
  void operator()(const Iterator &iter,
                  typename is_neighbourhood_metric<U>::no = 0,
                  typename use_processed_image<U>::no = 0,
                  typename cost_is_vector<U>::yes = 0,
                  typename is_asymmetric<U>::no = 0) {

    const Eigen::Vector3d voxel_pos{static_cast<default_type>(iter.index(0)),
                                    static_cast<default_type>(iter.index(1)),
                                    static_cast<default_type>(iter.index(2))};

    Eigen::Vector3d midway_point = voxel2scanner * voxel_pos;

    Eigen::Vector3d im2_point;
    params.transformation.transform_half_inverse(im2_point, midway_point);
    if (params.im2_mask_interp) {
      params.im2_mask_interp->scanner(im2_point);
      if (params.im2_mask_interp->value() < 0.5)
        return;
    }

    Eigen::Vector3d im1_point;
    params.transformation.transform_half(im1_point, midway_point);
    if (params.im1_mask_interp) {
      params.im1_mask_interp->scanner(im1_point);
      if (params.im1_mask_interp->value() < 0.5)
        return;
    }

    params.im1_image_interp->scanner(im1_point);
    if (!(*params.im1_image_interp))
      return;

    params.im2_image_interp->scanner(im2_point);
    if (!(*params.im2_image_interp))
      return;

    ++cnt;
    cost_function.noalias() = cost_function + metric(params, im1_point, im2_point, midway_point, gradient);
  }

  template <class U = MetricType>
  void operator()(const Iterator &iter,
                  typename is_neighbourhood_metric<U>::no = 0,
                  typename use_processed_image<U>::yes = 0,
                  typename cost_is_vector<U>::no = 0,
                  typename is_asymmetric<U>::no = 0) {
    assert(params.processed_image.valid());
    assign_pos_of(iter, 0, 3).to(params.processed_image);

    if (params.processed_mask.valid()) {
      assign_pos_of(iter, 0, 3).to(params.processed_mask);
      if (!params.processed_mask.value())
        return;
    }
    ++cnt;
    cost_function(0) += metric(params, iter, gradient);
  }

  template <class U = MetricType>
  void operator()(const Iterator &iter,
                  typename is_neighbourhood_metric<U>::yes = 0,
                  typename use_processed_image<U>::yes = 0,
                  typename cost_is_vector<U>::no = 0,
                  typename is_asymmetric<U>::no = 0) {
    assert(params.processed_image.valid());

    const Eigen::Vector3d voxel_pos{static_cast<default_type>(iter.index(0)),
                                    static_cast<default_type>(iter.index(1)),
                                    static_cast<default_type>(iter.index(2))};

    if (params.processed_mask.valid()) {
      assign_pos_of(iter, 0, 3).to(params.processed_mask);
      assert(params.processed_mask.index(0) == iter.index(0));
      assert(params.processed_mask.index(1) == iter.index(1));
      assert(params.processed_mask.index(2) == iter.index(2));
      if (!params.processed_mask.value())
        return;
    }

    ++cnt;
    cost_function(0) += metric(params, iter, gradient);
  }

protected:
  MetricType metric;
  ParamType params;

  Eigen::VectorXd cost_function;
  ssize_t cnt;
  Eigen::VectorXd gradient;
  transform_type voxel2scanner;
  // MR::Transform transform;
};

//! Evaluate a metric over one chunk of a multi-threaded loop
/*! The sums accumulated over the chunk are stored in the slot that \a partials
 *  reserves for it, from where they are merged in a fixed order once the loop
 *  has completed. */
template <class MetricType, class ParamType> class ChunkKernel {
public:
  ChunkKernel(const MetricType &metric,
              const ParamType &parameters,
              const std::vector<size_t> &inner_axes,
              const ssize_t cost_size,
              const ssize_t gradient_size,
              PartialSums &partials)
      : kernel(metric, parameters, cost_size, gradient_size),
        inner_loop(Loop(inner_axes)),
        cost_size(cost_size),
        gradient_size(gradient_size),
        partials(partials) {}

  void operator()(Iterator &pos) {
    const size_t index = partials.chunk(pos);
    kernel.reset();
    for (auto i = inner_loop(pos); i; ++i)
      kernel(pos);
    auto slot = partials[index];
    slot.head(cost_size) = kernel.partial_cost();
    slot.segment(cost_size, gradient_size) = kernel.partial_gradient();
    // voxel counts are small enough to be summed exactly in floating-point
    slot[slot.size() - 1] = static_cast<default_type>(kernel.partial_count());
  }

protected:
  ThreadKernel<MetricType, ParamType> kernel;
  const decltype(Loop(std::vector<size_t>())) inner_loop;
  const ssize_t cost_size;
  const ssize_t gradient_size;
  PartialSums &partials;
};

//! Accumulate \a metric and its gradient over the voxels of \a image
/*! The multi-threaded evaluation is reduced in a fixed order, such that
 *  \a cost, \a gradient and \a count are independent of both the number of
 *  threads and the order in which those threads are scheduled. */
template <class MetricType, class ParamType, class HeaderType>
void reduce(const MetricType &metric,
            const ParamType &params,
            const HeaderType &image,
            Eigen::VectorXd &cost,
            Eigen::VectorXd &gradient,
            ssize_t *count = nullptr) {
  auto loop = ThreadedLoop(image, 0, 3);
  PartialSums partials(image, loop.outer_loop.axes, cost.size() + gradient.size() + 1);
  ChunkKernel<MetricType, ParamType> functor(metric, params, loop.inner_axes, cost.size(), gradient.size(), partials);
  loop.run_outer(functor);
  check_app_exit_code();
  const Eigen::VectorXd total = partials.sum();
  cost += total.head(cost.size());
  gradient += total.segment(cost.size(), gradient.size());
  if (count != nullptr)
    *count += static_cast<ssize_t>(total[total.size() - 1]);
}

template <class MetricType, class ParamType> struct StochasticThreadKernel {
public:
  StochasticThreadKernel(const std::vector<size_t> &inner_axes,
                         const default_type density,
                         const MetricType &metric,
                         const ParamType &parameters,
                         Eigen::VectorXd &overall_cost_function,
                         Eigen::VectorXd &overall_grad,
                         Math::RNG &rng_engine,
                         ssize_t *overlap_count = nullptr)
      : inner_axes(inner_axes),
        density(density),
        metric(metric),
        params(parameters),
        cost_function(overall_cost_function.size()),
        gradient(overall_grad.size()),
        cnt(0),
        overall_cost_function(overall_cost_function),
        overall_gradient(overall_grad),
        rng(rng_engine),
        overlap_count(overlap_count) {
    gradient.setZero();
    cost_function.setZero();
    assert(inner_axes.size() >= 2);
    assert(overall_cost_function.size() == 1);
  }

  //! merge into the shared sums; ThreadedLoop() destroys its functors after all threads have re-joined
  ~StochasticThreadKernel() {
    overall_cost_function += cost_function;
    overall_gradient += gradient;
    if (overlap_count != nullptr)
      (*overlap_count) += cnt;
  }

  void operator()(const Iterator &iter) {
    auto engine = std::default_random_engine{static_cast<std::default_random_engine::result_type>(rng.get_seed())};
    auto kern = ThreadKernel<MetricType, ParamType>(metric, params, cost_function.size(), gradient.size());
    Iterator iterator(iter);
    assign_pos_of(iter).to(iterator);
    auto inner_loop = Random_loop<Iterator, std::default_random_engine>(
        iterator, engine, inner_axes[1], static_cast<default_type>(iterator.size(inner_axes[1])) * density);
    for (auto j = inner_loop; j; ++j) {
      for (auto k = Loop(inner_axes[0])(iterator); k; ++k)
        kern(iterator);
    }
    cost_function += kern.partial_cost();
    gradient += kern.partial_gradient();
    cnt += kern.partial_count();
  }

protected:
  std::vector<size_t> inner_axes;
  default_type density;
  MetricType metric;
  ParamType params;
  Eigen::VectorXd cost_function;
  Eigen::VectorXd gradient;
  ssize_t cnt;
  Eigen::VectorXd &overall_cost_function;
  Eigen::VectorXd &overall_gradient;
  Math::RNG rng;
  ssize_t *overlap_count;
};

} // namespace MR::Registration::Metric
