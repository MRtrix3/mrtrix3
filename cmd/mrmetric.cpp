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
#include "algo/loop.h"
#include "algo/threaded_loop.h"
#include "math/math.h"

#include "math/average_space.h"
#include "interp/linear.h"
#include "interp/nearest.h"
#include "interp/cubic.h"
#include "interp/sinc.h"
#include "filter/reslice.h"

#include "transform.h"
#include "registration/transform/rigid.h"
#include "registration/metric/cross_correlation.h"
#include "registration/metric/mean_squared.h"
#include "registration/metric/params.h"
#include "registration/metric/thread_kernel.h"

using namespace MR;
using namespace App;

const char* interp_choices[] = { "nearest", "linear", "cubic", "sinc", NULL };
const char* space_choices[] = { "voxel", "image1", "image2", "average", NULL };

template <class ValueType>
inline void meansquared (const ValueType& value1,
  const ValueType& value2,
  Eigen::Matrix<ValueType, Eigen::Dynamic, 1>& cost){
  cost.array() += Math::pow2 (value1 - value2);
}

template <class ValueType>
inline void meansquared (const Eigen::Matrix<ValueType,Eigen::Dynamic, 1>& value1,
  const Eigen::Matrix<ValueType,Eigen::Dynamic, 1>& value2,
  Eigen::Matrix<ValueType, Eigen::Dynamic, 1>& cost) {
  cost.array() += (value1 - value2).array().square();
}

template <class ImageType1, class ImageType2, class TransformType, class OversampleType, class ValueType>
void reslice(size_t interp, ImageType1& input, ImageType2& output, const TransformType& trafo = Adapter::NoTransform, const OversampleType& oversample = Adapter::AutoOverSample, const ValueType out_of_bounds_value = 0.f){
  switch(interp){
    case 0:
      Filter::reslice<Interp::Nearest> (input, output, trafo, Adapter::AutoOverSample, out_of_bounds_value);
      DEBUG("Nearest");
      break;
    case 1:
      Filter::reslice<Interp::Linear> (input, output, trafo, Adapter::AutoOverSample, out_of_bounds_value);
      DEBUG("Linear");
      break;
    case 2:
      Filter::reslice<Interp::Cubic> (input, output, trafo, Adapter::AutoOverSample, out_of_bounds_value);
      DEBUG("Cubic");
      break;
    case 3:
      Filter::reslice<Interp::Sinc> (input, output, trafo, Adapter::AutoOverSample, out_of_bounds_value);
      DEBUG("Sinc");
      break;
    default:
      throw Exception ("Fixme: interpolation value invalid");
  }
}

template <class InType1, class InType2, class MaskType1, class MaskType2>
  void evaluate_voxelwise_msq ( InType1& in1,
                            InType2& in2,
                            MaskType1& in1mask,
                            MaskType2& in2mask,
                            const size_t dimensions,
                            bool use_mask1,
                            bool use_mask2,
                            ssize_t& n_voxels,
                            Eigen::VectorXd& sos) {

    using value_type = typename InType1::value_type;

    if (use_mask1 or use_mask2)
      n_voxels = 0;
    if (use_mask1 and use_mask2) {
      if (dimensions == 3) {
        for (auto i = Loop() (in1, in2, in1mask, in2mask); i ;++i)
          if (in1mask.value() and in2mask.value()) {
            ++n_voxels;
            meansquared<value_type>(in1.value(), in2.value(), sos);
          }
      } else { // 4D
        Eigen::Matrix<value_type,Eigen::Dynamic,1> a (in1.size(3)), b (in2.size(3));
        for (auto i = Loop(0, 3) (in1, in2, in1mask, in2mask); i ;++i) {
          if (in1mask.value() and in2mask.value()) {
            ++n_voxels;
            a = in1.row(3);
            b = in2.row(3);
            meansquared<value_type>(a, b, sos);
          }
        }
      }
    } else if (use_mask1) {
      if (dimensions == 3) {
        for (auto i = Loop() (in1, in2, in1mask); i ;++i)
          if (in1mask.value()){
            ++n_voxels;
            meansquared<value_type>(in1.value(), in2.value(), sos);
          }
      } else { // 4D
        Eigen::Matrix<value_type,Eigen::Dynamic,1> a (in1.size(3)), b (in2.size(3));
        for (auto i = Loop(0, 3) (in1, in2, in1mask); i ;++i) {
          if (in1mask.value()){
            ++n_voxels;
            a = in1.row(3);
            b = in2.row(3);
            meansquared<value_type>(a, b, sos);
          }
        }
      }
    } else if (use_mask2) {
      if (dimensions == 3) {
        for (auto i = Loop() (in1, in2, in2mask); i ;++i)
          if (in2mask.value()){
            ++n_voxels;
            meansquared<value_type>(in1.value(), in2.value(), sos);
          }
      } else { // 4D
        Eigen::Matrix<value_type,Eigen::Dynamic,1> a (in1.size(3)), b (in2.size(3));
        for (auto i = Loop(0, 3) (in1, in2, in2mask); i ;++i) {
          if (in2mask.value()){
            ++n_voxels;
            a = in1.row(3);
            b = in2.row(3);
            meansquared<value_type>(a, b, sos);
          }
        }
      }
    } else {
      if (dimensions == 3) {
        for (auto i = Loop() (in1, in2); i ;++i)
          meansquared<value_type>(in1.value(), in2.value(), sos);
      } else { // 4D
        Eigen::Matrix<value_type,Eigen::Dynamic,1> a (in1.size(3)), b (in2.size(3));
        for (auto i = Loop(0, 3) (in1, in2); i ;++i) {
          a = in1.row(3);
          b = in2.row(3);
          meansquared<value_type>(a, b, sos);
        }
      }
    }
  }

enum MetricType {MeanSquared, CrossCorrelation};
const char* metric_choices[] = { "diff", "cc", NULL };


void usage ()
{
  AUTHOR = "David Raffelt (david.raffelt@florey.edu.au) and Max Pietsch (maximilian.pietsch@kcl.ac.uk)";

  SYNOPSIS = "Computes a dissimilarity metric between two images";

  DESCRIPTION
  + "Currently only the mean squared difference is implemented.";

  ARGUMENTS
  + Argument ("image1", "the first input image.").type_image_in ()
  + Argument ("image2", "the second input image.").type_image_in ();

  OPTIONS
  + Option ("space",
        "voxel (default): per voxel "
        "image1: scanner space of image 1 "
        "image2: scanner space of image 2 "
        "average: scanner space of the average affine transformation of image 1 and 2 ")
    +   Argument ("iteration method").type_choice (space_choices)

    + Option ("interp",
        "set the interpolation method to use when reslicing (choices: nearest, linear, cubic, sinc. Default: linear).")
    + Argument ("method").type_choice (interp_choices)

    + Option ("metric",
        "define the dissimilarity metric used to calculate the cost. "
        "Choices: diff (squared differences), cc (negative cross correlation). Default: diff). "
        "cc is only implemented for -space average and -interp linear.")
    + Argument ("method").type_choice (metric_choices)

    + Option ("mask1", "mask for image 1")
    + Argument ("image").type_image_in ()

    + Option ("mask2", "mask for image 2")
    + Argument ("image").type_image_in ()

    + Option ("nonormalisation",
        "do not normalise the dissimilarity metric to the number of voxels.")

    + Option ("overlap",
        "output number of voxels that were used.");

}

using value_type = double;
using MaskType = Image<bool>;

void run ()
{
  int space = 0;  // voxel
  auto opt = get_options ("space");
  if (opt.size())
    space = opt[0][0];

  int interp = 1;  // linear
  opt = get_options ("interp");
  if (opt.size())
    interp = opt[0][0];

  MetricType metric_type = MetricType::MeanSquared;
  opt = get_options ("metric");
  if (opt.size()) {
    if (int(opt[0][0]) == 1) { // CC
      if (space != 3)
        throw Exception ("CC metric only implemented for use in average space");
      if (interp != 1 and interp != 2)
        throw Exception ("CC metric only implemented for use with linear and cubic interpolation");
      metric_type = MetricType::CrossCorrelation;
    }
  }

  auto input1 = Image<value_type>::open (argument[0]).with_direct_io (Stride::contiguous_along_axis (3));
  auto input2 = Image<value_type>::open (argument[1]).with_direct_io (Stride::contiguous_along_axis (3));

  const size_t dimensions = input1.ndim();
  if (input1.ndim() != input2.ndim())
    throw Exception ("both images have to have the same number of dimensions");
  DEBUG ("dimensions: " + str(dimensions));
  if (dimensions > 4) throw Exception ("images have to be 3 or 4 dimensional");

  if (dimensions != 3 and metric_type == MetricType::CrossCorrelation)
    throw Exception ("CC metric requires 3D images");

  size_t volumes(1);
  if (dimensions == 4) {
    volumes = input1.size(3);
    if (input1.size(3) != input2.size(3)){
      throw Exception ("both images have to have the same number of volumes");
    }
  }
  INFO ("volumes: " + str(volumes));

  MaskType mask1;
  bool use_mask1 = get_options ("mask1").size()==1;
  if (use_mask1) {
    mask1 = Image<bool>::open (get_options ("mask1")[0][0]);
    if (mask1.ndim() != 3) throw Exception ("mask has to be 3D");
  }

  MaskType mask2;
  bool use_mask2 = get_options ("mask2").size()==1;
  if (use_mask2){
    mask2 = Image<bool>::open (get_options ("mask2")[0][0]);
    if (mask2.ndim() != 3) throw Exception ("mask has to be 3D");
  }

  bool nonormalisation = false;
  if (get_options ("nonormalisation").size())
    nonormalisation = true;
  ssize_t n_voxels = input1.size(0) * input1.size(1) * input1.size(2);

  value_type out_of_bounds_value = 0.0;

  Eigen::Matrix<value_type, Eigen::Dynamic, 1> sos = Eigen::Matrix<value_type, Eigen::Dynamic, 1>::Zero (volumes, 1);
  if (space==0) {
    INFO ("per-voxel");
    check_dimensions (input1, input2);
    if (!use_mask1 and !use_mask2)
      n_voxels = input1.size(0) * input1.size(1) * input1.size(2);
    evaluate_voxelwise_msq (input1, input2, mask1, mask2, dimensions, use_mask1, use_mask2, n_voxels, sos);
  } else {
    DEBUG ("scanner space");
    auto output1 = Header::scratch (input1, "-").get_image<value_type>();
    auto output2 = Header::scratch (input2, "-").get_image<value_type>();

    MaskType output1mask;
    MaskType output2mask;

    if (space == 1){
      INFO ("space: image 1");
      output1 = input1;
      output1mask = mask1;
      output2 = Header::scratch (input1, "-").get_image<value_type>();
      output2mask = Header::scratch (input1, "-").get_image<bool>();
      {
        LogLevelLatch log_level (0);
        reslice(interp, input2, output2, Adapter::NoTransform, Adapter::AutoOverSample, out_of_bounds_value);
        if (use_mask2)
          Filter::reslice<Interp::Nearest> (mask2, output2mask, Adapter::NoTransform, Adapter::AutoOverSample, 0);
      }
      evaluate_voxelwise_msq (output1, output2, output1mask, output2mask, dimensions, use_mask1, use_mask2, n_voxels, sos);
    }

    if (space == 2) {
      INFO ("space: image 2");
      output1 = Header::scratch (input2, "-").get_image<value_type>();
      output1mask = Header::scratch (input2, "-").get_image<bool>();
      output2 = input2;
      output2mask = mask2;
      {
        LogLevelLatch log_level (0);
        reslice(interp, input1, output1, Adapter::NoTransform, Adapter::AutoOverSample, out_of_bounds_value);
        if (use_mask1)
          Filter::reslice<Interp::Nearest> (mask1, output1mask, Adapter::NoTransform, Adapter::AutoOverSample, 0);
      }
      n_voxels = input2.size(0) * input2.size(1) * input2.size(2);
      evaluate_voxelwise_msq (output1, output2, output1mask, output2mask, dimensions, use_mask1, use_mask2, n_voxels, sos);
    }

    if (space == 3) {
      INFO ("space: average space");

      using ImageType1 = Image<value_type>;
      using ImageType2 = Image<value_type>;
      using ImageTypeM = Header;

      n_voxels = 0;
      vector<Header> headers;
      Registration::Transform::Rigid transform;
      vector<Eigen::Transform<default_type, 3, Eigen::Projective>> init_transforms;
      Eigen::Matrix<default_type, 4, 1> padding (0.0, 0.0, 0.0, 0.0);
      headers.push_back (Header (input1));
      headers.push_back (Header (input2));

      Header midway_image_header = compute_minimum_average_header (headers, 1, padding, init_transforms);

      using LinearInterpolatorType1 = Interp::LinearInterp<Image<value_type>, Interp::LinearInterpProcessingType::Value>;
      using LinearInterpolatorType2 = Interp::LinearInterp<Image<value_type>, Interp::LinearInterpProcessingType::Value>;
      using CubicInterpolatorType1 = Interp::SplineInterp<ImageType1, Math::UniformBSpline<typename ImageType1::value_type>, Math::SplineProcessingType::Value>;
      using CubicInterpolatorType2 = Interp::SplineInterp<ImageType2, Math::UniformBSpline<typename ImageType2::value_type>, Math::SplineProcessingType::Value>;
      using MaskInterpolatorType1 = Interp::Nearest<Image<bool>>;
      using MaskInterpolatorType2 = Interp::Nearest<Image<bool>>;
      using ProcessedImageType = Image<default_type>;
      using ProcessedMaskType = Image<bool>;

      using LinearParamType = Registration::Metric::Params <
                               Registration::Transform::Rigid,
                               ImageType1,
                               ImageType2,
                               ImageTypeM,
                               MaskType,
                               MaskType,
                               LinearInterpolatorType1,
                               LinearInterpolatorType2,
                               MaskInterpolatorType1,
                               MaskInterpolatorType2,
                               Image<default_type>,
                               Interp::LinearInterp<ProcessedImageType, Interp::LinearInterpProcessingType::Value>,
                               ProcessedMaskType,
                               Interp::Nearest<ProcessedMaskType>
                               >;
      using CubicParamType = Registration::Metric::Params <
                               Registration::Transform::Rigid,
                               ImageType1,
                               ImageType2,
                               ImageTypeM,
                               MaskType,
                               MaskType,
                               CubicInterpolatorType1,
                               CubicInterpolatorType2,
                               MaskInterpolatorType1,
                               MaskInterpolatorType2,
                               ProcessedImageType,
                               Interp::LinearInterp<ProcessedImageType, Interp::LinearInterpProcessingType::Value>,
                               ProcessedMaskType,
                               Interp::Nearest<ProcessedMaskType>
                               >;

        ImageTypeM midway_image (midway_image_header);

        Eigen::VectorXd gradient = Eigen::VectorXd::Zero(1);
        // interp == 1 or 2, metric, dimensions, interp
        if (interp == 1 or interp == 2) {
          if ( metric_type == MetricType::MeanSquared ) {
            if ( dimensions == 3 ) {
              Registration::Metric::MeanSquaredNoGradient  metric;
              if (interp == 1) {
                LinearParamType parameters (transform, input1, input2, midway_image, mask1, mask2);
                Registration::Metric::ThreadKernel<decltype(metric), LinearParamType> kernel
                  (metric, parameters, sos, gradient, &n_voxels);
                parameters.transformation.debug();
                ThreadedLoop (parameters.midway_image, 0, 3).run (kernel);
              } else if (interp == 2) {
                CubicParamType parameters (transform, input1, input2, midway_image, mask1, mask2);
                Registration::Metric::ThreadKernel<decltype(metric), CubicParamType> kernel
                  (metric, parameters, sos, gradient, &n_voxels);
                ThreadedLoop (parameters.midway_image, 0, 3).run (kernel);
              }
            } else if ( dimensions == 4) {
              Registration::Metric::MeanSquaredVectorNoGradient4D<ImageType1, ImageType2>
                metric ( input1, input2 );
              if (interp == 1) {
                LinearParamType parameters (transform, input1, input2, midway_image, mask1, mask2);
                Registration::Metric::ThreadKernel<decltype(metric), LinearParamType> kernel
                  (metric, parameters, sos, gradient, &n_voxels);

                ThreadedLoop (parameters.midway_image, 0, 3).run (kernel);
              } else if (interp == 2) {
                CubicParamType parameters (transform, input1, input2, midway_image, mask1, mask2);
                Registration::Metric::ThreadKernel<decltype(metric), CubicParamType> kernel
                  (metric, parameters, sos, gradient, &n_voxels);
                ThreadedLoop (parameters.midway_image, 0, 3).run (kernel);
              } else { throw Exception ("Fixme: invalid metric choice "); }
            }
          } else if ( metric_type == MetricType::CrossCorrelation) {
            Registration::Metric::CrossCorrelationNoGradient metric;
            if (interp == 1) {
              LinearParamType parameters (transform, input1, input2, midway_image, mask1, mask2);
              metric.precompute (parameters);
              Registration::Metric::ThreadKernel<decltype(metric), LinearParamType> kernel
                (metric, parameters, sos, gradient, &n_voxels);
              ThreadedLoop (parameters.processed_image, 0, 3).run (kernel);
            } else if (interp == 2) {
              CubicParamType parameters (transform, input1, input2, midway_image, mask1, mask2);
              metric.precompute (parameters);
              Registration::Metric::ThreadKernel<decltype(metric), CubicParamType> kernel
                (metric, parameters, sos, gradient, &n_voxels);
              ThreadedLoop (parameters.processed_image, 0, 3).run (kernel);
            }
          }
      } else { // interp != 1 or 2 --> reslice and run voxel-wise comparison
        if (metric_type != MetricType::MeanSquared)
          throw Exception ("Fixme: invalid metric choice ");
        output1mask = Header::scratch (midway_image_header, "-").get_image<bool>();
        output2mask = Header::scratch (midway_image_header, "-").get_image<bool>();

        Header new_header;
        new_header.ndim() = input1.ndim();
        for (ssize_t dim=0; dim < 3; ++dim){
          new_header.size(dim) = midway_image_header.size(dim);
          new_header.spacing(dim) = midway_image_header.spacing(dim);
        }
        if (dimensions == 4 ){
          new_header.size(3) = input1.size(3);
          new_header.spacing(3) = input1.spacing(3); // doesn't matter what spacing(3) is
        }
        new_header.transform() = midway_image_header.transform();
        output1 = Header::scratch (new_header,"-").get_image<value_type>();
        output2 = Header::scratch (new_header,"-").get_image<value_type>();
        {
          LogLevelLatch log_level (0);
          reslice(interp, input1, output1, Adapter::NoTransform, Adapter::AutoOverSample, out_of_bounds_value);
          reslice(interp, input2, output2, Adapter::NoTransform, Adapter::AutoOverSample, out_of_bounds_value);
          if (use_mask1)
            Filter::reslice<Interp::Nearest> (mask1, output1mask, Adapter::NoTransform, Adapter::AutoOverSample, 0);
          if (use_mask2)
            Filter::reslice<Interp::Nearest> (mask2, output2mask, Adapter::NoTransform, Adapter::AutoOverSample, 0);
        }
        n_voxels = output1.size(0) * output1.size(1) * output1.size(2);
        evaluate_voxelwise_msq (output1, output2, output1mask, output2mask, dimensions, use_mask1, use_mask2, n_voxels, sos);
      }
    } // "average space"
  }
  DEBUG ("n_voxels:" + str(n_voxels));
  if (n_voxels==0)
    WARN("number of overlapping voxels is zero");

  if (!nonormalisation)
    sos.array() /= static_cast<value_type>(n_voxels);
  std::cout << str(sos.transpose());

  if (get_options ("overlap").size())
    std::cout << " " << str(n_voxels);
  std::cout << std::endl;
}

