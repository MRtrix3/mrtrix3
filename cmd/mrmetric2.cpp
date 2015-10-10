#include "command.h"
#include "image.h"
#include "algo/loop.h"

#include "image/average_space.h"
// #include "interp/linear.h"
#include "interp/nearest.h"
#include "interp/cubic.h"
// #include "interp/sinc.h"
#include "filter/reslice.h"
#include "registration/metric/mean_squared.h"
#include "registration/metric/params.h"
#include "registration/metric/evaluate.h"
#include "registration/metric/thread_kernel.h"
#include "registration/transform/affine.h"


using namespace MR;
using namespace App;

const char* interp_choices[] = { "nearest", "linear", "cubic", "sinc", NULL }; // TODO
const char* space_choices[] = { "voxel", "image1", "image2", "average", NULL };

template <class ValueType>
void meansquared(const ValueType& value1, const ValueType& value2, ValueType& cost){
  cost += std::pow (value1 - value2, 2);
}
template <class ValueType>
void crosscorrelation(const ValueType& value1, const ValueType& value2, ValueType& cost){
  cost += 0.0;
}

template <class ValueType>
void mutualinformation(const ValueType& value1, const ValueType& value2, ValueType& cost){
  cost += 0.0;
}

void usage ()
{
  AUTHOR = "Maximilian Pietsch";

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

    + Option ("nonormalisation",
        "do not normalise the dissimilarity metric to the number of voxels.");

}

typedef double value_type;

typedef Image<value_type> Image1Type;
typedef Image<value_type> Image2Type;
typedef Image<value_type> MidwayImageType;
typedef Interp::SplineInterp<Image1Type, Math::UniformBSpline<typename Image1Type::value_type>, Math::SplineProcessingType::ValueAndGradient> Image1InterpolatorType;
typedef Interp::SplineInterp<Image2Type, Math::UniformBSpline<typename Image2Type::value_type>, Math::SplineProcessingType::ValueAndGradient> Image2InterpolatorType;
typedef Registration::Transform::Affine TransformType;
typedef Interp::Nearest<Image<bool>> BogusMaskType;
typedef Registration::Metric::Params<TransformType,
                                      Image1Type,
                                      Image2Type,
                                      MidwayImageType,
                                      Image1InterpolatorType,
                                      Image2InterpolatorType,
                                      BogusMaskType,
                                      BogusMaskType,
                                      Image1Type,
                                      Image1InterpolatorType,
                                      Image<bool>,
                                      BogusMaskType> ParamType;

typedef Registration::Metric::MeanSquared MetricType;

void run ()
{
  auto input1 = Image<value_type>::open (argument[0]);
  auto input2 = Image<value_type>::open (argument[1]);

  auto metric = MetricType();
  
  auto transform = TransformType();


  int space = 0;  // voxel
  auto opt = get_options ("space");
  if (opt.size())
    space = opt[0][0];

  bool nonormalisation = false;
  if (get_options ("nonormalisation").size())
    nonormalisation = true;
  ssize_t n_voxels = input1.size(0) * input1.size(1) * input1.size(2);

  value_type out_of_bounds_value = 0.0;

  
  value_type sos = 0.0;
  if (space==0){
    DEBUG("per-voxel");
    check_dimensions (input1, input2);

    for (auto i = Loop() (input1, input2); i ;++i)
      meansquared<value_type>(input1.value(), input2.value(), sos);

  } else {
    DEBUG("scanner space");
    int interp = 2;  // cubic
    opt = get_options ("interp");
    if (opt.size())
      interp = opt[0][0];
    
    auto output1 = Header::scratch(input1.original_header(),"-").get_image<value_type>();
    auto output2 = Header::scratch(input2.original_header(),"-").get_image<value_type>();

    if (interp == 2) {
      if (space == 1){
        DEBUG("image 1");
        output1 = input1;
        output2 = Header::scratch(input1.original_header(),"-").get_image<value_type>();
        {
          LogLevelLatch log_level (0);
          Filter::reslice<Interp::Cubic> (input2, output2, Adapter::NoTransform, Adapter::AutoOverSample, out_of_bounds_value);
        }
      }
      if (space == 2) {
        DEBUG("image 2");
        output1 = Header::scratch(input2.original_header(),"-").get_image<value_type>();
        output2 = input2;
        {
          LogLevelLatch log_level (0);
          Filter::reslice<Interp::Cubic> (input1, output1, Adapter::NoTransform, Adapter::AutoOverSample, out_of_bounds_value);
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

        output1 = Header::scratch(template_header,"-").get_image<value_type>();
        output2 = Header::scratch(template_header,"-").get_image<value_type>();
        { 
          LogLevelLatch log_level (0);
          Filter::reslice<Interp::Cubic> (input1, output1, Adapter::NoTransform,Adapter::AutoOverSample, out_of_bounds_value);
          Filter::reslice<Interp::Cubic> (input2, output2, Adapter::NoTransform, Adapter::AutoOverSample, out_of_bounds_value);
        }
        n_voxels = output1.size(0) * output1.size(1) * output1.size(2);


        // registration: symmetric metric
        auto midspace_image = output1;
        ParamType parameters (transform, input1, input2, midspace_image);
        // parameters.set_extent (kernel_extent);


        Registration::Metric::Evaluate<MetricType, ParamType> evaluate (metric, parameters);
        Eigen::Matrix<default_type, 12, 1> x;
        x << 1.0,0.0,0.0,0.0,1.0,0.0,0.0,0.0,1.0,0.0,0.0,0.0;
        parameters.transformation.set_parameter_vector(x);
        Eigen::Matrix<default_type, Eigen::Dynamic, 1> gradient;
        // Eigen::Vector3d gradient;
        gradient.setZero();
        double cost = 0;

        Registration::Metric::ThreadKernel<MetricType, ParamType> kernel (metric, parameters, cost, gradient);
        ThreadedLoop (parameters.midway_image, 0, 3).run (kernel);

        // double cost = evaluate(x, gradient);
        std::cout << "cost/n: " << cost / static_cast<value_type>(n_voxels) << std::endl;
        std::cout << "cost: " << cost << std::endl;
        std::cout << "gradient " << gradient.transpose() << std::endl;
      }
    } else throw Exception ("Other than cubic interpolation not implemented yet.");

    for (auto i = Loop() (output1, output2); i ;++i)
      meansquared<value_type>(output1.value(), output2.value(), sos);

  }
  if (!nonormalisation)
    sos /= static_cast<value_type>(n_voxels);
  std::cout << str(sos) << std::endl;
}

