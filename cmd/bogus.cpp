#include "command.h"
#include "debug.h"
#include "image.h"
#include "registration/transform/compose.h"

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
  + Argument ("in", "the input image.").type_image_in ()
  + Argument ("transform", "the transformation.").type_file_in ()
  + Argument ("out", "the output image.").type_image_out ();
}


void run ()
{
//  auto input = Image<float>::open (argument[0]).with_direct_io (Stride::contiguous_along_axis(3));
  transform_type linear_transform;

  std::cout << linear_transform.matrix() << std::endl;

//  auto output = Image<float>::create (argument[2], input.original_header());
//  Registration::Transform::compose (linear_transform, input, output);


//  copy (input, output);
}
