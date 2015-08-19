#include "command.h"
#include "debug.h"
#include "image.h"
#include "interp/cubic.h"

namespace MR {
}


using namespace MR;
using namespace App;


void usage ()
{
  AUTHOR = "Joe Bloggs (joe.bloggs@acme.org)";

  DESCRIPTION
  + "test vector adapter";

  ARGUMENTS
  + Argument ("in", "the input image.").type_image_in ();
}


void run ()
{
  auto input = Image<float>::open (argument[0]).with_direct_io (Stride::contiguous_along_axis(3));
  Interp::SplineInterp<Image<float>, Math::UniformBSpline<float>, Math::SplineProcessingType::ValueAndGradient> interp (input, 0.0, false  );
  Eigen::Vector3 voxel(46,41,29);
  interp.voxel(voxel);
  float value = 0;
  Eigen::Matrix<float,1,3> gradient;


//  std::cout << interp.gradient() << std::endl;

  interp.value_and_gradient(value, gradient);

  std::cout << value << std::endl;

  std::cout << gradient << std::endl;




//  auto output = Image<float>::create (argument[2], input.header());
//  Registration::Transform::compose (linear_transform, input, output);


//  copy (input, output);
}
