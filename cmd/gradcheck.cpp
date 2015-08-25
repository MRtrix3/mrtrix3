#include "command.h"
#include "debug.h"
#include "image.h"
#include "interp/linear.h"
#include "interp/cubic.h"

namespace MR {
}


using namespace MR;
using namespace App;


void usage ()
{
  AUTHOR = "Joe Bloggs (joe.bloggs@acme.org)";

  DESCRIPTION
  + "test gradient calculation";

  ARGUMENTS
  + Argument ("in", "the input image.").type_image_in ();
}


void run ()
{
  std::cout << argument[0] << std::endl;

  auto input = Image<float>::open (argument[0]); //.with_direct_io (Stride::contiguous_along_axis(3));
  Interp::SplineInterp<Image<float>, Math::UniformBSpline<float>, Math::SplineProcessingType::ValueAndGradient> interp_cubic (input, 0.0, true); // bool: gradient with respect to scanner 


  // Interp::SplineInterp<Image<float>, Math::UniformBSpline<float>, Math::SplineProcessingType::Value> interp_cubic_value (input, 0.0);
  
  Interp::Linear<Image<float>> interp_linear (input, 0.0);


  Eigen::Vector3 voxel(input.size(0)/2,input.size(1)/2,input.size(2)/2);
  std::cout << "voxel: " << voxel.transpose() << std::endl;
  
  for (size_t i=0; i<3; i++){
    input.index(i) = voxel(i);
    // interp_cubic.index(i) = voxel(i);
    // interp_cubic_value.index(i) = voxel(i);
    // interp_linear.index(i) = voxel(i);
  }

  VAR(input);
  std::cout << input.transform().matrix() << std::endl;
  // std::cout << interp_cubic << std::endl;
  // std::cout << interp_linear << std::endl;

  // VAR(interp_cubic_value.value());
  // interp_cubic_value.voxel(voxel);
  // VAR(interp_cubic_value.value());
  // std::cout << interp_cubic_value << std::endl;
  float value = 0;
  Eigen::Matrix<float,1,3> gradient;
  interp_cubic.voxel(voxel);
  VAR(interp_cubic);
  interp_cubic.value_and_gradient(value, gradient);
  interp_cubic.voxel(voxel);
  VAR(interp_cubic);
  VAR(value);
  VAR(interp_cubic.value());
  VAR(input.value());

  VAR(gradient);
  // VAR(interp_cubic.value());
  
  
  // CONSOLE("linear:");
  // interp_linear.voxel(voxel);
  // std::cout << interp_linear << std::endl;
  // VAR(interp_linear.value());
  // VAR(interp_linear.gradient());

 auto output = Image<float>::create ("cubic_value.mif", input.header());
 for (auto i = Loop() (output); i ;++i){
    voxel << output.index(0), output.index(1), output.index(2);
    // interp_cubic.voxel(voxel);
    interp_cubic.index(0) = output.index(0);
    interp_cubic.index(1) = output.index(1);
    interp_cubic.index(2) = output.index(2);
    output.value() = interp_cubic.value();
 }

 auto output2 = Image<float>::create ("cubic_value_and_gradient.mif", input.header());
 for (auto i = Loop() (output2); i ;++i){
    voxel << output2.index(0), output2.index(1), output2.index(2);
    interp_cubic.voxel(voxel);
    interp_cubic.value_and_gradient(value, gradient);
    output2.value() = value; 
 }

}
