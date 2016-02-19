#include "command.h"
#include "image.h"
#include "algo/loop.h"
#include "math/math.h"

#include "image/average_space.h"
#include "interp/linear.h"
#include "interp/nearest.h"
#include "interp/cubic.h"
#include "interp/sinc.h"
#include "filter/reslice.h"

using namespace MR;
using namespace App;

const char* interp_choices[] = { "nearest", "linear", "cubic", "sinc", NULL }; // TODO
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

// TODO: there must be a better way of doing this?
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

void usage ()
{
  AUTHOR = "David Raffelt (david.raffelt@florey.edu.au)";

  DESCRIPTION
  + "computes a dissimilarity metric between two images. Currently only the mean squared difference is implemented";

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
        "set the interpolation method to use when reslicing (choices: nearest, linear, cubic, sinc. Default: cubic).")
    + Argument ("method").type_choice (interp_choices)

    + Option ("mask1", "mask for image 1")
    + Argument ("image").type_image_in ()

    + Option ("mask2", "mask for image 2")
    + Argument ("image").type_image_in ()

    + Option ("nonormalisation",
        "do not normalise the dissimilarity metric to the number of voxels.")

    + Option ("overlap",
        "output number of voxels that were used.");

}

typedef double value_type;
typedef Image<bool> MaskType;

void run ()
{
  auto input1 = Image<value_type>::open (argument[0]).with_direct_io (Stride::contiguous_along_axis (3));;
  auto input2 = Image<value_type>::open (argument[1]).with_direct_io (Stride::contiguous_along_axis (3));;

  const size_t dimensions = input1.ndim();
  if (input1.ndim() != input2.ndim())
    throw Exception ("both images have to have the same number of dimensions");
  DEBUG("dimensions: " + str(dimensions));
  if (dimensions > 4) throw Exception ("images have to be 3D or 4D");

  size_t volumes(1);
  if (dimensions == 4) {
    volumes = input1.size(3);
    if (input1.size(3) != input2.size(3)){
      throw Exception ("both images have to have the same number of volumes");
    }
  }
  DEBUG("volumes: " + str(volumes));

  int space = 0;  // voxel
  auto opt = get_options ("space");
  if (opt.size())
    space = opt[0][0];

  MaskType mask1;
  bool use_mask1 = get_options ("mask1").size()==1;
  if (use_mask1) {
    mask1 = Image<bool>::open(get_options ("mask1")[0][0]);
    if (mask1.ndim() != 3) throw Exception ("mask has to be 3D");
  }

  MaskType mask2;
  bool use_mask2 = get_options ("mask2").size()==1;
  if (use_mask2){
    mask2 = Image<bool>::open(get_options ("mask2")[0][0]);
    if (mask2.ndim() != 3) throw Exception ("mask has to be 3D");
  }

  bool nonormalisation = false;
  if (get_options ("nonormalisation").size())
    nonormalisation = true;
  ssize_t n_voxels = input1.size(0) * input1.size(1) * input1.size(2);

  value_type out_of_bounds_value = 0.0;

  Eigen::Matrix<value_type, Eigen::Dynamic, 1> sos = Eigen::Matrix<value_type, Eigen::Dynamic, 1>::Zero (volumes);
  if (space==0){
    DEBUG("per-voxel");
    check_dimensions (input1, input2);
    if (use_mask1 or use_mask2)
      n_voxels = 0;
    if (use_mask1 and use_mask2) {
      if (dimensions == 3) {
        for (auto i = Loop() (input1, input2, mask1, mask2); i ;++i)
          if (mask1.value() and mask2.value()){
            ++n_voxels;
            meansquared<value_type>(input1.value(), input2.value(), sos);
          }
      } else { // 4D
        for (auto i = Loop(0, 3) (input1, input2, mask1, mask2); i ;++i) {
          if (mask1.value() and mask2.value()){
            ++n_voxels;
            meansquared<value_type>(input1.row(3), input2.row(3), sos);
          }
        }
      }
    } else if (use_mask1) {
        if (dimensions == 3) {
          for (auto i = Loop() (input1, input2, mask1); i ;++i)
            if (mask1.value()){
              ++n_voxels;
              meansquared<value_type>(input1.value(), input2.value(), sos);
            }
        } else { // 4D
          for (auto i = Loop(0, 3) (input1, input2, mask1); i ;++i) {
            if (mask1.value()){
              ++n_voxels;
              meansquared<value_type>(input1.row(3), input2.row(3), sos);
            }
          }
        }
    } else if (use_mask2){
      if (dimensions == 3) {
          for (auto i = Loop() (input1, input2, mask2); i ;++i)
            if (mask2.value()){
              ++n_voxels;
              meansquared<value_type>(input1.value(), input2.value(), sos);
            }
        } else { // 4D
          for (auto i = Loop(0, 3) (input1, input2, mask2); i ;++i) {
            if (mask2.value()){
              ++n_voxels;
              meansquared<value_type>(input1.row(3), input2.row(3), sos);
            }
          }
        }
    } else {
      for (auto i = Loop() (input1, input2); i ;++i)
        meansquared<value_type>(input1.value(), input2.value(), sos);
    }
  } else {
    DEBUG("scanner space");
    int interp = 2;  // cubic
    opt = get_options ("interp");
    if (opt.size())
      interp = opt[0][0];

    auto output1 = Header::scratch(input1.original_header(),"-").get_image<value_type>();
    auto output2 = Header::scratch(input2.original_header(),"-").get_image<value_type>();

    MaskType output1mask;
    MaskType output2mask;

    if (space == 1){
      DEBUG("image 1");
      output1 = input1;
      output1mask = mask1;
      output2 = Header::scratch(input1.original_header(),"-").get_image<value_type>();
      output2mask = Header::scratch(input1.original_header(),"-").get_image<bool>();
      {
        LogLevelLatch log_level (0);
        reslice(interp,input2, output2, Adapter::NoTransform, Adapter::AutoOverSample, out_of_bounds_value);
        if (use_mask2)
          Filter::reslice<Interp::Nearest> (mask2, output2mask, Adapter::NoTransform, Adapter::AutoOverSample, 0);
      }
    }

    if (space == 2) {
      DEBUG("image 2");
      output1 = Header::scratch(input2.original_header(),"-").get_image<value_type>();
      output1mask = Header::scratch(input2.original_header(),"-").get_image<bool>();
      output2 = input2;
      output2mask = mask2;
      {
        LogLevelLatch log_level (0);
        reslice(interp, input1, output1, Adapter::NoTransform, Adapter::AutoOverSample, out_of_bounds_value);
        if (use_mask1)
          Filter::reslice<Interp::Nearest> (mask1, output1mask, Adapter::NoTransform, Adapter::AutoOverSample, 0);
      }
      n_voxels = input2.size(0) * input2.size(1) * input2.size(2);
    }

    if (space == 3) {
      DEBUG("average space");
      std::vector<Header> headers;
      headers.push_back (input1.original_header());
      headers.push_back (input2.original_header());
      default_type template_res = 1.0;
      auto padding = Eigen::Matrix<default_type, 4, 1>(0, 0, 0, 1.0);
      std::vector<Eigen::Transform<double, 3, Eigen::Projective>> transform_header_with;

      auto template_header = compute_minimum_average_header<double,Eigen::Transform<double, 3, Eigen::Projective>>(headers, template_res, padding, transform_header_with);

      output1mask = Header::scratch(template_header,"-").get_image<bool>();
      output2mask = Header::scratch(template_header,"-").get_image<bool>();

      Header new_header;
      new_header.set_ndim(input1.ndim());
      for (ssize_t dim=0; dim < 3; ++dim){
        new_header.size(dim) = template_header.size(dim);
        new_header.spacing(dim) = template_header.spacing(dim);
      }
      if (dimensions == 4 ){
        new_header.size(3) = input1.size(3);
        new_header.spacing(3) = input1.spacing(3);
      }
      new_header.transform() = template_header.transform();
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
    }

    if (use_mask1 or use_mask2)
      n_voxels = 0;
    if (use_mask1 and use_mask2){
      if (dimensions == 3) {
        for (auto i = Loop() (output1, output2, output1mask, output2mask); i ;++i)
          if (output1mask.value() and output2mask.value()){
            ++n_voxels;
            meansquared<value_type>(output1.value(), output2.value(), sos);
          }
      } else { // 4D
        for (auto i = Loop(0, 3) (output1, output2, output1mask, output2mask); i ;++i) {
          if (output1mask.value() and output2mask.value()){
            ++n_voxels;
            meansquared<value_type>(output1.row(3), output2.row(3), sos);
          }
        }
      }
    } else if (use_mask1){
      if (dimensions == 3) {
        for (auto i = Loop() (output1, output2, output1mask); i ;++i)
          if (output1mask.value()){
            ++n_voxels;
            meansquared<value_type>(output1.value(), output2.value(), sos);
          }
      } else { // 4D
        for (auto i = Loop(0, 3) (output1, output2, output1mask); i ;++i) {
          if (output1mask.value()){
            ++n_voxels;
            meansquared<value_type>(output1.row(3), output2.row(3), sos);
          }
        }
      }
    } else if (use_mask2){
      if (dimensions == 3) {
        for (auto i = Loop() (output1, output2, output2mask); i ;++i)
          if (output2mask.value()){
            ++n_voxels;
            meansquared<value_type>(output1.value(), output2.value(), sos);
          }
      } else { // 4D
        for (auto i = Loop(0, 3) (output1, output2, output2mask); i ;++i) {
          if (output2mask.value()){
            ++n_voxels;
            meansquared<value_type>(output1.row(3), output2.row(3), sos);
          }
        }
      }
    } else {
      if (dimensions == 3) {
        for (auto i = Loop() (output1, output2); i ;++i)
          meansquared<value_type>(output1.value(), output2.value(), sos);
      } else { // 4D
        for (auto i = Loop(0, 3) (output1, output2); i ;++i)
          meansquared<value_type>(output1.row(3), output2.row(3), sos);
      }
    }

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

