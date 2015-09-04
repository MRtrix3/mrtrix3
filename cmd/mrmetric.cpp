#include "command.h"
#include "image.h"
#include "algo/loop.h"

#include "image/average_space.h"
// #include "interp/linear.h"
// #include "interp/nearest.h"
#include "interp/cubic.h"
// #include "interp/sinc.h"
#include "filter/reslice.h"

using namespace MR;
using namespace App;

const char* interp_choices[] = { "nearest", "linear", "cubic", "sinc", NULL }; // TODO
const char* space_choices[] = { "voxel", "image1", "image2", "average", NULL };

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

    + Option ("nonormalisation",
        "do not normalise the dissimilarity metric to the number of voxels.");

}

typedef double value_type;

void run ()
{
  auto input1 = Image<value_type>::open (argument[0]);
  auto input2 = Image<value_type>::open (argument[1]);

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
      sos += std::pow (input1.value() - input2.value(), 2);
  } else {
    DEBUG("scanner space");
    int interp = 2;  // cubic
    opt = get_options ("interp");
    if (opt.size())
      interp = opt[0][0];
    
    auto output1 = Header::scratch(input1.header(),"-").get_image<value_type>();
    auto output2 = Header::scratch(input2.header(),"-").get_image<value_type>();

    if (interp == 2) {
      if (space == 1){
        DEBUG("image 1");
        output1 = input1;
        output2 = Header::scratch(input1.header(),"-").get_image<value_type>();
        {
          LogLevelLatch log_level (0);
          Filter::reslice<Interp::Cubic> (input2, output2, Adapter::NoTransform, Adapter::AutoOverSample, out_of_bounds_value);
        }
      }
      if (space == 2) {
        DEBUG("image 2");
        output1 = Header::scratch(input2.header(),"-").get_image<value_type>();
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
        headers.push_back (input1.header());
        headers.push_back (input2.header());
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
      }
    } else throw Exception ("Other than cubic interpolation not implemented yet.");

    for (auto i = Loop() (output1, output2); i ;++i)
      sos += std::pow (output1.value() - output2.value(), 2);

  }
  if (!nonormalisation)
    sos /= static_cast<value_type>(n_voxels);
  std::cout << str(sos) << std::endl;
}

